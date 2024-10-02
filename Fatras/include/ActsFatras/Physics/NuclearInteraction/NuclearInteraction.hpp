// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/EventData/ProcessType.hpp"
#include "ActsFatras/Physics/NuclearInteraction/NuclearInteractionParameters.hpp"

#include <any>
#include <cmath>
#include <iterator>
#include <limits>
#include <optional>
#include <random>
#include <utility>
#include <vector>

namespace ActsFatras {

/// This class provides a parametrised nuclear interaction. The thereby
/// required parametrisation needs to be set and is not provided by default.
///
/// @note This class differs between two different processes labelled as nuclear
/// interaction. Either the initial particle survives (soft) or it gets
/// destroyed (hard) by this process.
struct NuclearInteraction {
  using Scalar = Particle::Scalar;
  /// The storage of the parameterisation
  detail::MultiParticleNuclearInteractionParametrisation
      multiParticleParameterisation;
  /// The number of trials to match momenta and inveriant masses
  //~ unsigned int nMatchingTrials = std::numeric_limits<unsigned int>::max();
  unsigned int nMatchingTrials = 100;
  unsigned int nMatchingTrialsTotal = 1000;

  /// This method evaluates the nuclear interaction length L0.
  ///
  /// @tparam generator_t The random number generator type
  /// @param [in, out] generator The random number generator
  /// @param [in] particle The ingoing particle
  ///
  /// @return valid X0 limit and no limit on L0
  template <typename generator_t>
  std::pair<Scalar, Scalar> generatePathLimits(generator_t& generator,
                                               const Particle& particle) const {
    // Fast exit: No paramtrization provided
    if (multiParticleParameterisation.empty()) {
      return std::make_pair(std::numeric_limits<Scalar>::infinity(),
                            std::numeric_limits<Scalar>::infinity());
    }
    // Find the parametrisation that corresponds to the particle type
    for (const auto& particleParametrisation : multiParticleParameterisation) {
      if (particleParametrisation.first == particle.pdg()) {
        std::uniform_real_distribution<double> uniformDistribution{0., 1.};

        // Get the parameters
        const detail::NuclearInteractionParametrisation&
            singleParticleParametrisation = particleParametrisation.second;
        const detail::NuclearInteractionParameters& parametrisation =
            findParameters(uniformDistribution(generator),
                           singleParticleParametrisation,
                           particle.absoluteMomentum());

        // Set the L0 limit if not done already
        const auto& distribution =
            parametrisation.nuclearInteractionProbability;
        auto limits =
            std::make_pair(std::numeric_limits<Scalar>::infinity(),
                           sampleContinuousValues(
                               uniformDistribution(generator), distribution));
        return limits;
      }
    }
    return std::make_pair(std::numeric_limits<Scalar>::infinity(),
                          std::numeric_limits<Scalar>::infinity());
  }

  /// This method performs a nuclear interaction.
  ///
  /// @tparam generator_t The random number generator type
  /// @param [in, out] generator The random number generator
  /// @param [in, out] particle The ingoing particle
  /// @param [out] generated Additional generated particles
  ///
  /// @return True if the particle was killed, false otherwise
  template <typename generator_t>
  bool run(generator_t& generator, Particle& particle,
           std::vector<Particle>& generated) const {
    // Fast exit: No paramtrization provided
    if (multiParticleParameterisation.empty()) {
      return false;
    }

    // Find the parametrisation that corresponds to the particle type
    for (const auto& particleParametrisation : multiParticleParameterisation) {
      if (particleParametrisation.first == particle.pdg()) {
        std::uniform_real_distribution<double> uniformDistribution{0., 1.};

        // Get the parameters
        const detail::NuclearInteractionParametrisation&
            singleParticleParametrisation = particleParametrisation.second;
        const detail::NuclearInteractionParameters& parametrisation =
            findParameters(uniformDistribution(generator),
                           singleParticleParametrisation,
                           particle.absoluteMomentum());

        std::normal_distribution<double> normalDistribution{0., 1.};
        // Dice the interaction type
        const bool interactSoft =
            softInteraction(normalDistribution(generator),
                            parametrisation.softInteractionProbability);

        // Get the final state multiplicity
        const unsigned int multiplicity = finalStateMultiplicity(
            uniformDistribution(generator),
            interactSoft ? parametrisation.softMultiplicity
                         : parametrisation.hardMultiplicity);

        // Get the parameters for the kinematics
        const std::vector<detail::NuclearInteractionParameters::
                              ParametersWithFixedMultiplicity>&
            parametrisationOfType =
                interactSoft ? parametrisation.softKinematicParameters
                             : parametrisation.hardKinematicParameters;
        const detail::NuclearInteractionParameters::
            ParametersWithFixedMultiplicity& parametrisationOfMultiplicity =
                parametrisationOfType[multiplicity];
        if (!parametrisationOfMultiplicity.validParametrisation) {
          return false;
        }

        // Get the kinematics
        const auto kinematics = sampleKinematics(
            generator, parametrisationOfMultiplicity, parametrisation.momentum);
        if (!kinematics.has_value()) {
          return run(generator, particle, generated);
        }

        // Get the particle types
        const std::vector<int> pdgIds =
            samplePdgIds(generator, parametrisation.pdgMap, multiplicity,
                         particle.pdg(), interactSoft);

        // Construct the particles
        const auto particles = convertParametersToParticles(
            generator, pdgIds, kinematics->first, kinematics->second, particle,
            parametrisation.momentum, interactSoft);

        // Kill the particle in a hard process
        if (!interactSoft) {
          particle.setAbsoluteMomentum(0);
        }

        generated.insert(generated.end(), particles.begin(), particles.end());
        return !interactSoft;
      }
    }
    // Fast exit if no parametrisation for the particle was provided
    return false;
  }

 private:
  /// Retrieves the parametrisation for the particle
  ///
  /// @param [in] rnd A random number
  /// @param [in] parametrisation The storage of parametrisations
  /// @param [in] particleMomentum The particles momentum
  ///
  /// @return The parametrisation
  const detail::NuclearInteractionParameters& findParameters(
      double rnd,
      const detail::NuclearInteractionParametrisation& parametrisation,
      float particleMomentum) const;

  /// Estimates the interaction type
  ///
  /// @param [in] rnd Random number
  /// @param [in] probability The probability for a soft interaction
  ///
  /// @return True if a soft interaction occurs
  inline bool softInteraction(double rnd, float probability) const {
    return rnd <= probability;
  }

  /// Evaluates the multiplicity of the final state
  ///
  /// @param [in] rnd Random number
  /// @param [in] distribution The multiplicity distribution
  ///
  /// @return The final state multiplicity
  unsigned int finalStateMultiplicity(
      double rnd,
      const detail::NuclearInteractionParameters::CumulativeDistribution&
          distribution) const;

  /// Evaluates the final state PDG IDs
  ///
  /// @tparam generator_t The random number generator type
  /// @param [in, out] generator The random number generator
  /// @param [in] pdgMap The branching probability map
  /// @param [in] multiplicity The final state multiplicity
  /// @param [in] particlePdg The PDG ID of the initial particle
  /// @param [in] soft Treat it as soft or hard nuclear interaction
  ///
  /// @return Vector containing the PDG IDs
  template <typename generator_t>
  std::vector<int> samplePdgIds(
      generator_t& generator,
      const detail::NuclearInteractionParameters::PdgMap& pdgMap,
      unsigned int multiplicity, int particlePdg, bool soft) const;

  /// Evaluates the final state invariant masses
  ///
  /// @tparam generator_t The random number generator type
  /// @param [in, out] generator The random number generator
  /// @param [in] parametrisation Parametrisation of kinematic properties
  ///
  /// @return Vector containing the invariant masses
  template <typename generator_t>
  Acts::ActsDynamicVector sampleInvariantMasses(
      generator_t& generator,
      const detail::NuclearInteractionParameters::
          ParametersWithFixedMultiplicity& parametrisation) const;

  /// Evaluates the final state momenta
  ///
  /// @tparam generator_t The random number generator type
  /// @param [in, out] generator The random number generator
  /// @param [in] parametrisation Parametrisation of kinematic properties
  /// @param [in] initialMomentum The initial momentum
  ///
  /// @return Vector containing the momenta
  template <typename generator_t>
  Acts::ActsDynamicVector sampleMomenta(
      generator_t& generator,
      const detail::NuclearInteractionParameters::
          ParametersWithFixedMultiplicity& parametrisation,
      float initialMomentum) const;

  /// Tests whether the final state momenta and invariant masses are
  /// matching to each other to allow the evaluation of particle directions.
  ///
  /// @param [in] momenta The final state momenta
  /// @param [in] invariantMasses The final state invariant masses
  /// @param [in] parametrizedMomentum The momentum of the parametrized particle
  ///
  /// @return Decision whether the parameters can be matched to each other or
  /// not.
  bool match(const Acts::ActsDynamicVector& momenta,
             const Acts::ActsDynamicVector& invariantMasses,
             float parametrizedMomentum) const;

  /// This method samples the kinematics of the final state particles
  ///
  /// @tparam generator_t The random number generator type
  /// @param [in, out] generator The random number generator
  /// @param [in] parameters The parametrisation
  /// @param [in] momentum The momentum of the parametrisation
  ///
  /// @return The final state momenta and invariant masses
  template <typename generator_t>
  std::optional<std::pair<Acts::ActsDynamicVector, Acts::ActsDynamicVector>>
  sampleKinematics(generator_t& generator,
                   const detail::NuclearInteractionParameters::
                       ParametersWithFixedMultiplicity& parameters,
                   float momentum) const;

  /// Converts relative angles to absolute angles wrt the global
  /// coordinate system.
  /// @note It is assumed that the angles of the first particle are provided in
  /// the context of the global coordinate system whereas the angles of the
  /// second particle are provided relatively to the first particle.
  ///
  /// @param [in] phi1 The azimuthal angle of the first particle
  /// @param [in] theta1 The polar angle of the first particle
  /// @param [in] phi2 The azimuthal angle of the second particle
  /// @param [in] theta2 The polar angle of the second particle
  ///
  /// @return Azimuthal and polar angle of the second particle in the global
  /// coordinate system
  std::pair<ActsFatras::Particle::Scalar, ActsFatras::Particle::Scalar>
  globalAngle(ActsFatras::Particle::Scalar phi1,
              ActsFatras::Particle::Scalar theta1, float phi2,
              float theta2) const;

  /// Converter from sampled numbers to a vector of particles
  ///
  /// @tparam generator_t The random number generator type
  /// @param [in, out] generator The random number generator
  /// @param [in] pdgId The PDG IDs
  /// @param [in] momenta The momenta
  /// @param [in] invariantMasses The invariant masses
  /// @param [in] initialParticle The initial particle
  /// @param [in] parametrizedMomentum Momentum of the parametrisation
  /// @param [in] soft Treat it as soft or hard nuclear interaction
  ///
  /// @return Vector containing the final state particles
  template <typename generator_t>
  std::vector<Particle> convertParametersToParticles(
      generator_t& generator, const std::vector<int>& pdgId,
      const Acts::ActsDynamicVector& momenta,
      const Acts::ActsDynamicVector& invariantMasses, Particle& initialParticle,
      float parametrizedMomentum, bool soft) const;

  /// This function performs an inverse sampling to provide a discrete
  /// value from a distribution.
  ///
  /// @param [in] rnd A random number in [0,1]
  /// @param [in] distribution The distribution to sample from
  ///
  /// @return The sampled value
  unsigned int sampleDiscreteValues(
      double rnd,
      const detail::NuclearInteractionParameters::CumulativeDistribution&
          distribution) const;

  /// This function performs an inverse sampling to provide a continuous
  /// value from a distribition.
  ///
  /// @param [in] rnd A random number in [0,1]
  /// @param [in] distribution The distribution to sample from
  /// @param [in] interpolate Flag to steer whether an interpolation between
  /// neighbouring bins should be performed instead of a bin lookup
  ///
  /// @return The sampled value
  Scalar sampleContinuousValues(
      double rnd,
      const detail::NuclearInteractionParameters::CumulativeDistribution&
          distribution,
      bool interpolate = false) const;
};

template <typename generator_t>
std::vector<int> NuclearInteraction::samplePdgIds(
    generator_t& generator,
    const detail::NuclearInteractionParameters::PdgMap& pdgMap,
    unsigned int multiplicity, int particlePdg, bool soft) const {
  // Fast exit in case of no final state particles
  if (multiplicity == 0) {
    return {};
  }

  // The final state PDG IDs
  std::vector<int> pdgIds;
  pdgIds.reserve(multiplicity);

  std::uniform_real_distribution<float> uniformDistribution{0., 1.};

  // Find the producers probability distribution
  auto citProducer = pdgMap.cbegin();
  while (citProducer->first != particlePdg && citProducer != pdgMap.end()) {
    citProducer++;
  }

  const std::vector<std::pair<int, float>>& mapInitial = citProducer->second;
  // Set the first particle depending on the interaction type
  if (soft) {
    // Store the initial particle if the interaction is soft
    pdgIds.push_back(particlePdg);
  } else {
    // Otherwise dice the particle
    const float rndInitial = uniformDistribution(generator);

    pdgIds.push_back(
        std::lower_bound(mapInitial.begin(), mapInitial.end(), rndInitial,
                         [](const std::pair<int, float>& element,
                            float random) { return element.second < random; })
            ->first);
  }

  // Set the remaining particles
  for (unsigned int i = 1; i < multiplicity; i++) {
    // Find the producers probability distribution from the last produced
    // particle
    citProducer = pdgMap.cbegin();
    while (citProducer->first != pdgIds[i - 1] && citProducer != pdgMap.end()) {
      citProducer++;
    }

    // Set the next particle
    const std::vector<std::pair<int, float>>& map = citProducer->second;
    const float rnd = uniformDistribution(generator);
    pdgIds.push_back(
        std::lower_bound(map.begin(), map.end(), rnd,
                         [](const std::pair<int, float>& element,
                            float random) { return element.second < random; })
            ->first);
  }
  return pdgIds;
}

template <typename generator_t>
Acts::ActsDynamicVector NuclearInteraction::sampleInvariantMasses(
    generator_t& generator,
    const detail::NuclearInteractionParameters::ParametersWithFixedMultiplicity&
        parametrisation) const {
  // The resulting vector
  Acts::ActsDynamicVector parameters;
  const unsigned int size = parametrisation.eigenvaluesInvariantMass.size();
  parameters.resize(size);

  // Sample in the eigenspace
  for (unsigned int i = 0; i < size; i++) {
    float variance = parametrisation.eigenvaluesInvariantMass[i];
    std::normal_distribution<Acts::ActsScalar> dist{
        parametrisation.meanInvariantMass[i], sqrtf(variance)};
    parameters[i] = dist(generator);
  }
  // Transform to multivariate normal distribution
  parameters = parametrisation.eigenvectorsInvariantMass * parameters;

  // Perform the inverse sampling from the distributions
  for (unsigned int i = 0; i < size; i++) {
    const double cdf = (std::erff(parameters[i]) + 1) * 0.5;
    parameters[i] = sampleContinuousValues(
        cdf, parametrisation.invariantMassDistributions[i]);
  }
  return parameters;
}

template <typename generator_t>
Acts::ActsDynamicVector NuclearInteraction::sampleMomenta(
    generator_t& generator,
    const detail::NuclearInteractionParameters::ParametersWithFixedMultiplicity&
        parametrisation,
    float initialMomentum) const {
  // The resulting vector
  Acts::ActsDynamicVector parameters;
  const unsigned int size = parametrisation.eigenvaluesMomentum.size();
  parameters.resize(size);

  // Sample in the eigenspace
  for (unsigned int i = 0; i < size; i++) {
    float variance = parametrisation.eigenvaluesMomentum[i];
    std::normal_distribution<Acts::ActsScalar> dist{
        parametrisation.meanMomentum[i], sqrtf(variance)};
    parameters[i] = dist(generator);
  }

  // Transform to multivariate normal distribution
  parameters = parametrisation.eigenvectorsMomentum * parameters;

  // Perform the inverse sampling from the distributions
  for (unsigned int i = 0; i < size; i++) {
    const float cdf = (std::erff(parameters[i]) + 1) * 0.5;
    parameters[i] =
        sampleContinuousValues(cdf, parametrisation.momentumDistributions[i]);
  }

  // Scale the momenta
  Acts::ActsDynamicVector momenta = parameters.head(size - 1);
  const float sum = momenta.sum();
  const float scale = parameters.template tail<1>()(0, 0) / sum;
  momenta *= scale * initialMomentum;
  return momenta;
}

template <typename generator_t>
std::optional<std::pair<Acts::ActsDynamicVector, Acts::ActsDynamicVector>>
NuclearInteraction::sampleKinematics(
    generator_t& generator,
    const detail::NuclearInteractionParameters::ParametersWithFixedMultiplicity&
        parameters,
    float momentum) const {
  unsigned int trials = 0;
  Acts::ActsDynamicVector invariantMasses =
      sampleInvariantMasses(generator, parameters);
  Acts::ActsDynamicVector momenta =
      sampleMomenta(generator, parameters, momentum);
  // Repeat momentum evaluation until the parameters match
  while (!match(momenta, invariantMasses, momentum)) {
    if (trials == nMatchingTrialsTotal) {
      return std::nullopt;
    }
    // Re-sampole invariant masses if no fitting momenta were found
    if (trials++ % nMatchingTrials == 0) {
      invariantMasses = sampleInvariantMasses(generator, parameters);
    } else {
      momenta = sampleMomenta(generator, parameters, momentum);
    }
  }
  return std::make_pair(momenta, invariantMasses);
}

template <typename generator_t>
std::vector<Particle> NuclearInteraction::convertParametersToParticles(
    generator_t& generator, const std::vector<int>& pdgId,
    const Acts::ActsDynamicVector& momenta,
    const Acts::ActsDynamicVector& invariantMasses, Particle& initialParticle,
    float parametrizedMomentum, bool soft) const {
  std::uniform_real_distribution<double> uniformDistribution{0., 1.};
  const auto& initialDirection = initialParticle.direction();
  const double phi = Acts::VectorHelpers::phi(initialDirection);
  const double theta = Acts::VectorHelpers::theta(initialDirection);
  const unsigned int size = momenta.size();

  std::vector<Particle> result;
  result.reserve(size);

  // Build the particles
  for (unsigned int i = 0; i < size; i++) {
    const float momentum = momenta[i];
    const float invariantMass = invariantMasses[i];
    const float p1p2 = 2. * momentum * parametrizedMomentum;
    const float costheta = 1. - invariantMass * invariantMass / p1p2;

    const auto phiTheta =
        globalAngle(phi, theta, uniformDistribution(generator) * 2. * M_PI,
                    std::acos(costheta));
    const auto direction =
        Acts::makeDirectionFromPhiTheta(phiTheta.first, phiTheta.second);

    Particle p = Particle(initialParticle.particleId().makeDescendant(i),
                          static_cast<Acts::PdgParticle>(pdgId[i]));
    p.setProcess(ProcessType::eNuclearInteraction)
        .setPosition4(initialParticle.fourPosition())
        .setAbsoluteMomentum(momentum)
        .setDirection(direction)
        .setReferenceSurface(initialParticle.referenceSurface());

    // Store the particle
    if (i == 0 && soft) {
      initialParticle = p;
    } else {
      result.push_back(std::move(p));
    }
  }

  return result;
}
}  // namespace ActsFatras
