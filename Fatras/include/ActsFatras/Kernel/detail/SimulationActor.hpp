// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Propagator/ConstrainedStep.hpp"
#include "Acts/Propagator/PropagatorState.hpp"
#include "Acts/Propagator/StandardAborters.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Kernel/SimulationResult.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace ActsFatras::detail {

/// Fatras simulation actor for the Acts propagator.
///
/// This actor must be added to the action list of the propagator and is the
/// equivalent to the `MaterialInteractor` for the reconstruction. This
/// implements surface-based simulation of particle interactions with matter
/// using a configurable interaction list as well as the decay simulation. The
/// interactions are simulated for every surface with valid material.
///
/// @tparam generator_t random number generator
/// @tparam decay_t decay module
/// @tparam interactions_t interaction list
/// @tparam hit_surface_selector_t selector for hit surfaces
template <typename generator_t, typename decay_t, typename interactions_t,
          typename hit_surface_selector_t>
struct SimulationActor {
  using result_type = SimulationResult;

  /// Random number generator used for the simulation.
  generator_t *generator = nullptr;
  /// Decay module.
  decay_t decay;
  /// Interaction list containing the simulated interactions.
  interactions_t interactions;
  /// Selector for surfaces that should generate hits.
  hit_surface_selector_t selectHitSurface;
  /// Initial particle state.
  Particle initialParticle;

  /// Relative tolerance of the particles proper time limit
  double properTimeRelativeTolerance = 1e-3;

  /// Simulate the interaction with a single surface.
  ///
  /// @tparam propagator_state_t is propagator state
  /// @tparam stepper_t is the stepper instance
  ///
  /// @param state is the mutable propagator state object
  /// @param stepper is the propagation stepper object
  /// @param result is the mutable result/cache object
  /// @param logger a logger instance
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  void act(propagator_state_t &state, stepper_t &stepper,
           navigator_t &navigator, result_type &result,
           const Acts::Logger &logger) const {
    assert(generator != nullptr && "The generator pointer must be valid");

    if (state.stage == Acts::PropagatorStage::prePropagation) {
      // first step is special: there is no previous state and we need to arm
      // the decay simulation for all future steps.
      result.particle =
          makeParticle(initialParticle, state, stepper, navigator);
      result.properTimeLimit =
          decay.generateProperTimeLimit(*generator, initialParticle);
      return;
    }

    // actors are called once more after the propagation terminated
    if (!result.isAlive) {
      return;
    }

    if (Acts::EndOfWorldReached{}.checkAbort(state, stepper, navigator,
                                             logger)) {
      result.isAlive = false;
      return;
    }

    // update the particle state first. this also computes the proper time which
    // needs the particle state from the previous step for reference. that means
    // this must happen for every step (not just on surface) and before
    // everything, e.g. any interactions that could modify the state.
    result.particle = makeParticle(result.particle, state, stepper, navigator);

    // decay check. needs to happen at every step, not just on surfaces.
    if (std::isfinite(result.properTimeLimit) &&
        (result.properTimeLimit - result.particle.properTime() <
         result.properTimeLimit * properTimeRelativeTolerance)) {
      auto descendants = decay.run(generator, result.particle);
      for (const auto &descendant : descendants) {
        result.generatedParticles.emplace_back(descendant);
      }
      result.isAlive = false;
      return;
    }

    // Regulate the step size
    if (std::isfinite(result.properTimeLimit)) {
      assert(result.particle.mass() > 0.0 && "Particle must have mass");
      //    beta² = p²/E²
      //    gamma = 1 / sqrt(1 - beta²) = sqrt(m² + p²) / m = E / m
      //     time = proper-time * gamma
      // ds = beta * dt = (p/E) dt (E/m) = (p/m) proper-time
      const auto properTimeDiff =
          result.properTimeLimit - result.particle.properTime();
      // Evaluate the step size for massive particle, assuming massless
      // particles to be stable
      const auto stepSize = properTimeDiff *
                            result.particle.absoluteMomentum() /
                            result.particle.mass();
      stepper.releaseStepSize(state.stepping,
                              Acts::ConstrainedStep::Type::User);
      stepper.updateStepSize(state.stepping, stepSize,
                             Acts::ConstrainedStep::Type::User);
    }

    // arm the point-like interaction limits in the first step
    if (std::isnan(result.x0Limit) || std::isnan(result.l0Limit)) {
      armPointLikeInteractions(initialParticle, result);
    }

    // If we are on target, everything should have been done
    if (state.stage == Acts::PropagatorStage::postPropagation) {
      return;
    }
    // If we are not on a surface, there is nothing further for us to do
    if (!navigator.currentSurface(state.navigation)) {
      return;
    }
    const Acts::Surface &surface = *navigator.currentSurface(state.navigation);

    // we need the particle state before and after the interaction for the hit
    // creation. create a copy since the particle will be modified in-place.
    const Particle before = result.particle;

    // interactions only make sense if there is material to interact with.
    if (surface.surfaceMaterial()) {
      // TODO is this the right thing to do when globalToLocal fails?
      //   it should in principle never happen, so probably it would be best
      //   to change to a model using transform() directly
      auto lpResult = surface.globalToLocal(state.geoContext, before.position(),
                                            before.direction());
      if (lpResult.ok()) {
        Acts::Vector2 local = lpResult.value();
        Acts::MaterialSlab slab =
            surface.surfaceMaterial()->materialSlab(local);
        // again: interact only if there is valid material to interact with
        if (!slab.isVacuum()) {
          // adapt material for non-zero incidence
          auto normal = surface.normal(state.geoContext, before.position(),
                                       before.direction());
          // dot-product(unit normal, direction) = cos(incidence angle)
          // particle direction is normalized, not sure about surface normal
          auto cosIncidenceInv = normal.norm() / normal.dot(before.direction());
          // apply abs in case `normal` and `before` produce an angle > 90°
          slab.scaleThickness(std::abs(cosIncidenceInv));
          // run the interaction simulation
          interact(slab, result);  // MARK: fpeMask(FLTUND, 1, #2346)
        }
      }
    }
    Particle &after = result.particle;

    // store results of this interaction step, including potential hits
    if (selectHitSurface(surface)) {
      result.hits.emplace_back(
          surface.geometryId(), before.particleId(),
          // the interaction could potentially modify the particle position
          0.5 * (before.fourPosition() + after.fourPosition()),
          before.fourMomentum(), after.fourMomentum(), result.hits.size());

      after.setNumberOfHits(result.hits.size());
    }

    if (after.absoluteMomentum() == 0.0) {
      result.isAlive = false;
      return;
    }

    // continue the propagation with the modified parameters
    stepper.update(state.stepping, after.position(), after.direction(),
                   after.qOverP(), after.time());
  }

  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  bool checkAbort(propagator_state_t & /*state*/, const stepper_t & /*stepper*/,
                  const navigator_t & /*navigator*/, const result_type &result,
                  const Acts::Logger & /*logger*/) const {
    // must return true if the propagation should abort
    return !result.isAlive;
  }

  /// Construct the current particle state from the propagation state.
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  Particle makeParticle(const Particle &previous, propagator_state_t &state,
                        stepper_t &stepper, navigator_t &navigator) const {
    // a particle can lose energy and thus its gamma factor is not a constant
    // of motion. since the stepper provides only the lab time, we need to
    // compute the change in proper time for each step separately. this assumes
    // that the gamma factor is constant over one stepper step.
    const auto deltaLabTime = stepper.time(state.stepping) - previous.time();
    // proper-time = time / gamma = (1/gamma) * time
    //       beta² = p²/E²
    //       gamma = 1 / sqrt(1 - beta²) = sqrt(m² + p²) / m
    //     1/gamma = m / sqrt(m² + p²) = m / E
    const auto gammaInv = previous.mass() / previous.energy();
    const auto properTime = previous.properTime() + gammaInv * deltaLabTime;
    const Acts::Surface *currentSurface = nullptr;
    if (navigator.currentSurface(state.navigation) != nullptr) {
      currentSurface = navigator.currentSurface(state.navigation);
    }
    // copy all properties and update kinematic state from stepper
    return Particle(previous)
        .setPosition4(stepper.position(state.stepping),
                      stepper.time(state.stepping))
        .setDirection(stepper.direction(state.stepping))
        .setAbsoluteMomentum(stepper.absoluteMomentum(state.stepping))
        .setProperTime(properTime)
        .setReferenceSurface(currentSurface);
  }

  /// Prepare limits and process selection for the next point-like interaction.
  void armPointLikeInteractions(const Particle &particle,
                                result_type &result) const {
    auto selection = interactions.armPointLike(*generator, particle);
    result.x0Limit = selection.x0Limit;
    result.l0Limit = selection.l0Limit;
    result.x0Process = selection.x0Process;
    result.l0Process = selection.l0Process;
  }

  /// Run the interaction simulation for the given material.
  ///
  /// Simulate all continuous processes and at most one point-like process
  /// within the material.
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
      if (interactions.runContinuous(*(this->generator), partialSlab,
                                     result.particle,
                                     result.generatedParticles)) {
        result.isAlive = false;
        retval = true;
      }
      // the SimulationActor is in charge of keeping track of the material.
      // since the accumulated material is stored in the particle it could (but
      // should not) be modified by a physics process. to avoid issues, the
      // material is updated only after process simulation has occurred. this
      // intentionally overwrites any material updates made by the process.
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
    //   occurred before reaching the material. not sure how this could happen,
    //   but in such a case the point-like interaction happens immediately.
    // 1 < fraction:
    //   the next point-like interaction does not occur within the current
    //   material. simulation is limited to the continuous processes.
    //
    // `clamp` ensures a valid range in all cases.
    const float fracX0 =
        std::clamp(static_cast<float>(x0Dist / slabX0), 0.0f, 1.0f);
    const float fracL0 =
        std::clamp(static_cast<float>(l0Dist / slabL0), 0.0f, 1.0f);
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
      const std::size_t process =
          (fracX0 < fracL0) ? result.x0Process : result.l0Process;
      // simulate the selected point-like process
      if (interactions.runPointLike(*generator, process, result.particle,
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

}  // namespace ActsFatras::detail
