// This file is part of the Acts project.
//
// Copyright (C) 2018-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "ActsFatras/EventData/Hit.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Kernel/SimulationResult.hpp"

#include <algorithm>
#include <cassert>
#include <limits>

namespace ActsFatras {
namespace detail {

/// Fatras simulator plugin for the Acts propagator.
///
/// This plugin must be added to the action list of the propagator and is the
/// equivalent to the `MaterialInteractor` for the reconstruction. This
/// implements surface-based simulation of particle interactions with matter
/// using a configurable physics lists as well as some parts of the decay
/// simulation. The physics lists is called for every surface with valid
/// material.
///
/// @tparam generator_t random number generator
/// @tparam decay_t decay module
/// @tparam continuous_physics_t physics lists for continuous interactions
/// @tparam pointlike_physics_t physics lists for point-like interactions
/// @tparam hit_surface_selector_t sensitive hit surfaces selector
template <typename generator_t, typename decay_t, typename continuous_physics_t,
          typename pointlike_physics_t, typename hit_surface_selector_t>
struct Interactor {
  using result_type = SimulationResult;

  /// Abort if the particle was killed during a previous interaction.
  struct ParticleNotAlive {
    // This references the Interactor to automatically access its result type.
    using action_type = Interactor;

    template <typename propagator_state_t, typename stepper_t>
    constexpr bool operator()(propagator_state_t &, const stepper_t &,
                              const result_type &result) const {
      // must return true if the propagation should abort
      return not result.isAlive;
    }
  };

  /// Random number generator used for the simulation.
  generator_t *generator = nullptr;
  /// Decay module.
  decay_t decay;
  /// Physics list detailing the simulated continuous interactions.
  continuous_physics_t continuous;
  /// Physics list detailing the simulated point-like interactions.
  pointlike_physics_t pointlike;
  /// Selector for surfaces that should generate hits.
  hit_surface_selector_t selectHitSurface;
  /// Initial particle state.
  Particle initialParticle;

  /// Simulate the interaction with a single surface.
  ///
  /// @tparam propagator_state_t is propagator state
  /// @tparam stepper_t is the stepper instance
  ///
  /// @param state is the mutable propagator state object
  /// @param stepper is the propagation stepper object
  /// @param result is the mutable result/cache object
  template <typename propagator_state_t, typename stepper_t>
  void operator()(propagator_state_t &state, stepper_t &stepper,
                  result_type &result) const {
    assert(generator and "The generator pointer must be valid");

    // update the particle state first. this also computes the proper time which
    // needs the particle state from the previous step for reference. that means
    // this must happen for every step (not just on surface) and before
    // everything, e.g. any interactions that could modify the state.
    if (std::isnan(result.properTimeLimit)) {
      // first step is special: there is no previous state and we need to arm
      // the decay simulation for all future steps.
      result.particle = makeParticle(initialParticle, stepper, state.stepping);
      // limit is relative to the initial state
      result.properTimeLimit =
          initialParticle.properTime() +
          decay.generateProperTimeLimit(*generator, initialParticle);
    } else {
      result.particle = makeParticle(result.particle, stepper, state.stepping);
    }

    // decay check. needs to happen at every step, not just on surfaces.
    // TODO limit the stepsize when close to the lifetime limit to avoid
    //   overstepping and decaying the particle systematically too late
    if (result.properTimeLimit < result.particle.properTime()) {
      auto descendants = decay.run(generator, result.particle);
      for (auto &&descendant : descendants) {
        result.generatedParticles.emplace_back(std::move(descendant));
      }
      result.isAlive = false;
      return;
    }

    // arm the point-like interaction limits in the first step
    if (std::isnan(result.x0Limit) or std::isnan(result.l0Limit)) {
      armPointLikeInteractions(initialParticle, result);
    }

    // If we are on target, everything should have been done
    if (state.navigation.targetReached) {
      return;
    }
    // If we are not on a surface, there is nothing further for us to do
    if (not state.navigation.currentSurface) {
      return;
    }
    const Acts::Surface &surface = *state.navigation.currentSurface;

    // we need the particle state before and after the interaction for the hit
    // creation. create a copy since the particle will be modified in-place.
    const Particle before = result.particle;

    // interactions only make sense if there is material to interact with.
    if (surface.surfaceMaterial()) {
      // TODO is this the right thing to do when globalToLocal fails?
      //   it should in principle never happen, so probably it would be best
      //   to change to a model using transform() directly
      auto lpResult = surface.globalToLocal(state.geoContext, before.position(),
                                            before.unitDirection());
      if (lpResult.ok()) {
        Acts::Vector2 local = lpResult.value();
        Acts::MaterialSlab slab =
            surface.surfaceMaterial()->materialSlab(local);
        // again: interact only if there is valid material to interact with
        if (slab) {
          // adapt material for non-zero incidence
          auto normal = surface.normal(state.geoContext, local);
          // dot-product(unit normal, direction) = cos(incidence angle)
          // particle direction is normalized, not sure about surface normal
          auto cosIncidenceInv =
              normal.norm() / normal.dot(before.unitDirection());
          slab.scaleThickness(cosIncidenceInv);
          // run the interaction simulation
          interact(slab, result);
        }
      }
    }
    const Particle &after = result.particle;

    // store results of this interaction step, including potential hits
    if (selectHitSurface(surface)) {
      result.hits.emplace_back(
          surface.geometryId(), before.particleId(),
          // the interaction could potentially modify the particle position
          Hit::Scalar(0.5) * (before.fourPosition() + after.fourPosition()),
          before.fourMomentum(), after.fourMomentum(), result.hits.size());
    }

    // continue the propagation with the modified parameters
    stepper.update(state.stepping, after.position(), after.unitDirection(),
                   after.absoluteMomentum(), after.time());
  }

  /// Pure observer interface. Does not apply to the Fatras simulator.
  template <typename propagator_state_t, typename stepper_t>
  void operator()(propagator_state_t &, stepper_t &) const {}

  /// Construct the current particle state from the stepper state.
  template <typename stepper_t>
  Particle makeParticle(const Particle &previous, const stepper_t &stepper,
                        const typename stepper_t::State &state) const {
    // a particle can loose energy and thus its gamma factor is not a constant
    // of motion. since the stepper provides only the lab time, we need to
    // compute the change in proper time for each step separately. this assumes
    // that the gamma factor is constant over one stepper step.
    const auto deltaLabTime = stepper.time(state) - previous.time();
    // proper-time = time / gamma = (1/gamma) * time
    //       beta² = p²/E²
    //       gamma = 1 / sqrt(1 - beta²) = sqrt(m² + p²) / m
    //     1/gamma = m / sqrt(m² + p²) = m / E
    const auto gammaInv = previous.mass() / previous.energy();
    const auto properTime = previous.properTime() + gammaInv * deltaLabTime;
    // copy all properties and update kinematic state from stepper
    return Particle(previous)
        .setPosition4(stepper.position(state), stepper.time(state))
        .setDirection(stepper.direction(state))
        .setAbsoluteMomentum(stepper.momentum(state))
        .setProperTime(properTime);
  }

  /// Prepare limits and process selection for the next point-like interaction.
  void armPointLikeInteractions(const Particle &particle,
                                result_type &result) const {
    auto selection = pointlike.arm(*generator, particle);
    // since interactions can occur multiple times, the generated limits must
    // always be considered relative to the already accumulated paths.
    result.x0Limit = particle.pathInX0() + selection.x0Limit;
    result.l0Limit = particle.pathInL0() + selection.l0Limit;
    result.x0Process = selection.x0Process;
    result.l0Process = selection.l0Process;
  }

  /// Run the interaction simulation for the given material.
  ///
  /// Simulate all continous processes and at most one point-like process within
  /// the material.
  void interact(const Acts::MaterialSlab &slab, result_type &result) const {
    // run the continuous processes over a fraction of the material. returns
    // true on break condition (same as the underlying physics lists).
    auto runContinuousPartial = [&, this](float fraction) {
      Acts::MaterialSlab partialSlab = slab;
      partialSlab.scaleThickness(fraction);
      // material after passing this slab
      const auto x0 = result.particle.pathInX0() + partialSlab.thicknessInX0();
      const auto l0 = result.particle.pathInX0() + partialSlab.thicknessInL0();
      bool retval = false;
      if (continuous.run(*(this->generator), partialSlab, result.particle,
                         result.generatedParticles)) {
        result.isAlive = false;
        retval = true;
      }
      // the Interactor is in charge of keeping track of the material. since the
      // accumulated material is stored in the particle it could (but should
      // not) be modified by a physics process. to avoid issues, the material is
      // updated only after process simulation has occured. this intentionally
      // overwrites any material updates made by the process.
      result.particle.setMaterialPassed(x0, l0);
      return retval;
    };

    // material thickness measured in radiation/interaction lengths
    const auto slabX0 = slab.thicknessInX0();
    const auto slabL0 = slab.thicknessInL0();
    // remaining radiation/interaction length to next point-like interaction
    // NOTE for limit=inf this should result in dist=inf
    const auto x0Dist = result.x0Limit - result.particle.pathInX0();
    const auto l0Dist = result.l0Limit - result.particle.pathInL0();

    // something point-like could happen within this material and we need to
    // select which process would come first. x0/l0 measures the propagated path
    // along different scales. to be able to check which one would happen first
    // they need to be translated to a common scale.

    // relative fraction within material where the interaction occurs.
    //
    // fraction < 0:
    //   this is an error case where the point-like interaction should have
    //   occured before reaching the material. not sure how this could happen,
    //   but in such a case the point-like interaction happens immediately.
    // 1 < fraction:
    //   the next point-like interaction does not occur within the current
    //   material. simulation is limited to the continuous processes.
    //
    // `clamp` ensures a valid range in all cases.
    const float fracX0 = std::clamp(float(x0Dist / slabX0), 0.0f, 1.0f);
    const float fracL0 = std::clamp(float(l0Dist / slabL0), 0.0f, 1.0f);
    // fraction of the material where the first point-like interaction occurs
    const float frac = std::min(fracX0, fracL0);

    // do not run if there is zero material before the point-like interaction
    if (0.0f < frac) {
      // simulate continuous processes before the point-like interaction
      if (runContinuousPartial(frac)) {
        return;
      }
    }
    // do not run if there is no point-like interaction
    if (frac < 1.0f) {
      // select which process to simulate
      const size_t process =
          (fracX0 < fracL0) ? result.x0Process : result.l0Process;
      // simulate the selected point-like process
      if (pointlike.run(*generator, process, result.particle,
                        result.generatedParticles)) {
        result.isAlive = false;
        return;
      }
      // simulate continuous processes after the point-like interaction
      if (runContinuousPartial(1.0 - frac)) {
        return;
      }

      // particle is still alive and point-like interactions can occur again.
      // in principle, the re-arming should occur directly after the point-like
      // process. this could lead to a situation where the next interaction
      // should already occur within the same material slab. thus, re-arming is
      // done after all processes are simulated to enforce the
      // one-interaction-per-slab rule.
      armPointLikeInteractions(result.particle, result);
    }
  }
};

}  // namespace detail
}  // namespace ActsFatras
