// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/ActorList.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Kernel/SimulationResult.hpp"
#include "ActsFatras/Kernel/detail/SimulationActor.hpp"
#include "ActsFatras/Kernel/detail/SimulationError.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <vector>

namespace ActsFatras {

/// Single particle simulation with fixed propagator, interactions, and decay.
///
/// @tparam generator_t random number generator
/// @tparam interactions_t interaction list
/// @tparam hit_surface_selector_t selector for hit surfaces
/// @tparam decay_t decay module
template <typename propagator_t, typename interactions_t,
          typename hit_surface_selector_t, typename decay_t>
struct SingleParticleSimulation {
  /// How and within which geometry to propagate the particle.
  propagator_t propagator;
  /// Absolute maximum step size
  double maxStepSize = std::numeric_limits<double>::max();
  /// Absolute maximum path length
  double pathLimit = std::numeric_limits<double>::max();
  /// Decay module.
  decay_t decay;
  /// Interaction list containing the simulated interactions.
  interactions_t interactions;
  /// Selector for surfaces that should generate hits.
  hit_surface_selector_t selectHitSurface;
  /// Logger for debug output.
  std::unique_ptr<const Acts::Logger> logger;

  /// Alternatively construct the simulator with an external logger.
  SingleParticleSimulation(propagator_t &&propagator_,
                           std::unique_ptr<const Acts::Logger> _logger)
      : propagator(propagator_), logger(std::move(_logger)) {}

  /// Simulate a single particle without secondaries.
  ///
  /// @tparam generator_t is the type of the random number generator
  ///
  /// @param geoCtx is the geometry context to access surface geometries
  /// @param magCtx is the magnetic field context to access field values
  /// @param generator is the random number generator
  /// @param particle is the initial particle state
  /// @returns Simulated particle state, hits, and generated particles.
  template <typename generator_t>
  Acts::Result<SimulationResult> simulate(
      const Acts::GeometryContext &geoCtx,
      const Acts::MagneticFieldContext &magCtx, generator_t &generator,
      const Particle &particle) const {
    // propagator-related additional types
    using Actor = detail::SimulationActor<generator_t, decay_t, interactions_t,
                                          hit_surface_selector_t>;
    using Result = typename Actor::result_type;
    using ActorList = Acts::ActorList<Actor>;
    using PropagatorOptions =
        typename propagator_t::template Options<ActorList>;

    // Construct per-call options.
    PropagatorOptions options(geoCtx, magCtx);
    options.stepping.maxStepSize = maxStepSize;
    options.pathLimit = pathLimit;
    // setup the interactor as part of the propagator options
    auto &actor = options.actorList.template get<Actor>();
    actor.generator = &generator;
    actor.decay = decay;
    actor.interactions = interactions;
    actor.selectHitSurface = selectHitSurface;
    actor.initialParticle = particle;

    if (particle.hasReferenceSurface()) {
      auto result = propagator.propagate(
          particle.boundParameters(geoCtx).value(), options);
      if (!result.ok()) {
        return result.error();
      }
      auto &value = result.value().template get<Result>();
      return std::move(value);
    }

    auto result =
        propagator.propagate(particle.curvilinearParameters(), options);
    if (!result.ok()) {
      return result.error();
    }
    auto &value = result.value().template get<Result>();
    return std::move(value);
  }
};

/// A particle that failed to simulate.
struct FailedParticle {
  /// Initial particle state of the failed particle.
  ///
  /// This must store the full particle state to be able to handle secondaries
  /// that are not in the input particle list. Otherwise they could not be
  /// referenced.
  Particle particle;
  /// The associated error code for this particular failure case.
  std::error_code error;
};

/// Multi-particle/event simulation.
///
/// @tparam charged_selector_t Callable selector type for charged particles
/// @tparam charged_simulator_t Single particle simulator for charged particles
/// @tparam neutral_selector_t Callable selector type for neutral particles
/// @tparam neutral_simulator_t Single particle simulator for neutral particles
///
/// The selector types for charged and neutral particles **do not** need to
/// check for the particle charge. This is done automatically by the simulator
/// to ensure consistency.
template <typename charged_selector_t, typename charged_simulator_t,
          typename neutral_selector_t, typename neutral_simulator_t>
struct Simulation {
  charged_selector_t selectCharged;
  neutral_selector_t selectNeutral;
  charged_simulator_t charged;
  neutral_simulator_t neutral;

  /// Construct from the single charged/neutral particle simulators.
  Simulation(charged_simulator_t &&charged_, neutral_simulator_t &&neutral_)
      : charged(std::move(charged_)), neutral(std::move(neutral_)) {}

  /// Simulate multiple particles and generated secondaries.
  ///
  /// @param geoCtx is the geometry context to access surface geometries
  /// @param magCtx is the magnetic field context to access field values
  /// @param generator is the random number generator
  /// @param inputParticles contains all particles that should be simulated
  /// @param simulatedParticlesInitial contains initial particle states
  /// @param simulatedParticlesFinal contains final particle states
  /// @param hits contains all generated hits
  /// @retval Acts::Result::Error if there is a fundamental issue
  /// @retval Acts::Result::Success with all particles that failed to simulate
  ///
  /// @warning Particle-hit association is based on particle ids generated
  ///          during the simulation. This requires that all input particles
  ///          **must** have generation and sub-particle number set to zero.
  /// @note Parameter edge-cases can lead to errors in the underlying propagator
  ///       and thus to particles that fail to simulate. Here, full events are
  ///       simulated and the failure to simulate one particle should not be
  ///       considered a general failure of the simulator. Instead, a list of
  ///       particles that fail to simulate is provided to the user. It is the
  ///       users responsibility to handle them.
  /// @note Failed particles are removed from the regular output, i.e. they do
  ///       not appear in the simulated particles containers nor do they
  ///       generate hits.
  ///
  /// This takes all input particles and simulates those passing the selection
  /// using the appropriate simulator. All selected particle states including
  /// additional ones generated from interactions are stored in separate output
  /// containers; both the initial state at the production vertex and the final
  /// state after propagation are stored. Hits generated from selected input and
  /// generated particles are stored in the hit container.
  ///
  /// @tparam generator_t is the type of the random number generator
  /// @tparam input_particles_t is a Container for particles
  /// @tparam output_particles_t is a SequenceContainer for particles
  /// @tparam hits_t is a SequenceContainer for hits
  template <typename generator_t, typename input_particles_t,
            typename output_particles_t, typename hits_t>
  Acts::Result<std::vector<FailedParticle>> simulate(
      const Acts::GeometryContext &geoCtx,
      const Acts::MagneticFieldContext &magCtx, generator_t &generator,
      const input_particles_t &inputParticles,
      output_particles_t &simulatedParticlesInitial,
      output_particles_t &simulatedParticlesFinal, hits_t &hits) const {
    assert(
        (simulatedParticlesInitial.size() == simulatedParticlesFinal.size()) &&
        "Inconsistent initial sizes of the simulated particle containers");

    using SingleParticleSimulationResult = Acts::Result<SimulationResult>;

    std::vector<FailedParticle> failedParticles;

    for (const Particle &inputParticle : inputParticles) {
      // only consider simulatable particles
      if (!selectParticle(inputParticle)) {
        continue;
      }
      // required to allow correct particle id numbering for secondaries later
      if ((inputParticle.particleId().generation() != 0u) ||
          (inputParticle.particleId().subParticle() != 0u)) {
        return detail::SimulationError::eInvalidInputParticleId;
      }

      // Do a *depth-first* simulation of the particle and its secondaries,
      // i.e. we simulate all secondaries, tertiaries, ... before simulating
      // the next primary particle. Use the end of the output container as
      // a queue to store particles that should be simulated.
      //
      // WARNING the initial particle state output container will be modified
      //         during iteration. New secondaries are added to and failed
      //         particles might be removed. To avoid issues, access must always
      //         occur via indices.
      auto iinitial = simulatedParticlesInitial.size();
      simulatedParticlesInitial.push_back(inputParticle);
      for (; iinitial < simulatedParticlesInitial.size(); ++iinitial) {
        const auto &initialParticle = simulatedParticlesInitial[iinitial];

        // only simulatable particles are pushed to the container and here we
        // only need to switch between charged/neutral.
        SingleParticleSimulationResult result =
            SingleParticleSimulationResult::success({});
        if (initialParticle.charge() != 0.) {
          result = charged.simulate(geoCtx, magCtx, generator, initialParticle);
        } else {
          result = neutral.simulate(geoCtx, magCtx, generator, initialParticle);
        }

        if (!result.ok()) {
          // remove particle from output container since it was not simulated.
          simulatedParticlesInitial.erase(
              std::next(simulatedParticlesInitial.begin(), iinitial));
          // record the particle as failed
          failedParticles.push_back({initialParticle, result.error()});
          continue;
        }

        assert(result->particle.particleId() == initialParticle.particleId() &&
               "Particle id must not change during simulation");

        copyOutputs(result.value(), simulatedParticlesInitial,
                    simulatedParticlesFinal, hits);
        // since physics processes are independent, there can be particle id
        // collisions within the generated secondaries. they can be resolved by
        // renumbering within each sub-particle generation. this must happen
        // before the particle is simulated since the particle id is used to
        // associate generated hits back to the particle.
        renumberTailParticleIds(simulatedParticlesInitial, iinitial);
      }
    }

    assert(
        (simulatedParticlesInitial.size() == simulatedParticlesFinal.size()) &&
        "Inconsistent final sizes of the simulated particle containers");

    // the overall function call succeeded, i.e. no fatal errors occurred.
    // yet, there might have been some particle for which the propagation
    // failed. thus, the successful result contains a list of failed particles.
    // sounds a bit weird, but that is the way it is.
    return failedParticles;
  }

 private:
  /// Select if the particle should be simulated at all.
  bool selectParticle(const Particle &particle) const {
    if (particle.charge() != 0.) {
      return selectCharged(particle);
    } else {
      return selectNeutral(particle);
    }
  }

  /// Copy results to output containers.
  ///
  /// @tparam particles_t is a SequenceContainer for particles
  /// @tparam hits_t is a SequenceContainer for hits
  template <typename particles_t, typename hits_t>
  void copyOutputs(const SimulationResult &result,
                   particles_t &particlesInitial, particles_t &particlesFinal,
                   hits_t &hits) const {
    // initial particle state was already pushed to the container before
    // store final particle state at the end of the simulation
    particlesFinal.push_back(result.particle);
    std::copy(result.hits.begin(), result.hits.end(), std::back_inserter(hits));

    // move generated secondaries that should be simulated to the output
    std::copy_if(
        result.generatedParticles.begin(), result.generatedParticles.end(),
        std::back_inserter(particlesInitial),
        [this](const Particle &particle) { return selectParticle(particle); });
  }

  /// Renumber particle ids in the tail of the container.
  ///
  /// Ensures particle ids are unique by modifying the sub-particle number
  /// within each generation.
  ///
  /// @param particles particle container in which particles are renumbered
  /// @param lastValid index of the last particle with a valid particle id
  ///
  /// @tparam particles_t is a SequenceContainer for particles
  ///
  /// @note This function assumes that all particles in the tail have the same
  ///       vertex numbers (primary/secondary) and particle number and are
  ///       ordered according to their generation number.
  ///
  template <typename particles_t>
  static void renumberTailParticleIds(particles_t &particles,
                                      std::size_t lastValid) {
    // iterate over adjacent pairs; potentially modify the second element.
    // assume e.g. a primary particle 2 with generation=subparticle=0 that
    // generates two secondaries during simulation. we have the following
    // ids (decoded as vertex|particle|generation|subparticle)
    //
    //     v|2|0|0, v|2|1|0, v|2|1|0
    //
    // where v represents the vertex numbers. this will be renumbered to
    //
    //     v|2|0|0, v|2|1|0, v|2|1|1
    //
    // if each secondary then generates two tertiaries we could have e.g.
    //
    //     v|2|0|0, v|2|1|0, v|2|1|1, v|2|2|0, v|2|2|1, v|2|2|0, v|2|2|1
    //
    // which would then be renumbered to
    //
    //     v|2|0|0, v|2|1|0, v|2|1|1, v|2|2|0, v|2|2|1, v|2|2|2, v|2|2|3
    //
    for (auto j = lastValid; (j + 1u) < particles.size(); ++j) {
      const auto prevId = particles[j].particleId();
      auto currId = particles[j + 1u].particleId();
      // NOTE primary/secondary vertex and particle are assumed to be equal
      // only renumber within one generation
      if (prevId.generation() != currId.generation()) {
        continue;
      }
      // ensure the sub-particle is strictly monotonic within a generation
      if (prevId.subParticle() < currId.subParticle()) {
        continue;
      }
      // sub-particle numbering must be non-zero
      currId.setSubParticle(prevId.subParticle() + 1u);
      particles[j + 1u] = particles[j + 1u].withParticleId(currId);
    }
  }
};

}  // namespace ActsFatras
