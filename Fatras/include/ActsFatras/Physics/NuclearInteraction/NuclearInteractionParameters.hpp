// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Common.hpp"

#include <vector>

namespace ActsFatras::detail {

/// @brief Data storage of the parametrized nuclear interaction
struct NuclearInteractionParameters {
  using CumulativeDistribution =
      std::pair<std::vector<float>, std::vector<std::uint32_t>>;
  using Distributions = std::vector<CumulativeDistribution>;
  using PdgMap =
      std::vector<std::pair<int, std::vector<std::pair<int, float>>>>;

  /// @brief Nested struct for the storage of the kinematic parametrisations for
  /// a given final state multiplicity
  struct ParametersWithFixedMultiplicity {
    ParametersWithFixedMultiplicity() = default;

    ParametersWithFixedMultiplicity(
        Distributions& momenta, Acts::ActsDynamicVector& eValMom,
        Acts::ActsDynamicVector& eVecMom, Acts::ActsDynamicVector& meanMom,
        Distributions& invariantMasses, Acts::ActsDynamicVector& eValIM,
        Acts::ActsDynamicVector& eVecIM, Acts::ActsDynamicVector& meanIM)
        : validParametrisation(true),
          momentumDistributions(momenta),
          eigenvaluesMomentum(eValMom),
          meanMomentum(meanMom),
          invariantMassDistributions(invariantMasses),
          eigenvaluesInvariantMass(eValIM),
          meanInvariantMass(meanIM) {
      const unsigned int sizeMom = eigenvaluesMomentum.size();
      eigenvectorsMomentum.resize(sizeMom, sizeMom);
      for (unsigned int i = 0; i < sizeMom; i++) {
        for (unsigned int j = 0; j < sizeMom; j++) {
          eigenvectorsMomentum(i, j) = eVecMom[i * sizeMom + j];
        }
      }

      const unsigned int sizeInvMass = eigenvaluesInvariantMass.size();
      eigenvectorsInvariantMass.resize(sizeInvMass, sizeInvMass);
      for (unsigned int i = 0; i < sizeInvMass; i++) {
        for (unsigned int j = 0; j < sizeInvMass; j++) {
          eigenvectorsInvariantMass(i, j) = eVecIM[i * sizeInvMass + j];
        }
      }
    }

    bool validParametrisation = false;

    /// Momentum parameters
    /// Generation-wise distributions
    Distributions momentumDistributions;
    /// Eigenvalues
    Acts::ActsDynamicVector eigenvaluesMomentum;
    /// Eigenvectors
    Acts::ActsDynamicMatrix eigenvectorsMomentum;
    /// Mean in eigenspace
    Acts::ActsDynamicVector meanMomentum;

    /// Invariant mass parameters
    /// Generation-wise distributions
    Distributions invariantMassDistributions;
    /// Eigenvalues
    Acts::ActsDynamicVector eigenvaluesInvariantMass;
    /// Eigenvectors
    Acts::ActsDynamicMatrix eigenvectorsInvariantMass;
    /// Mean in eigenspace
    Acts::ActsDynamicVector meanInvariantMass;
  };

  /// Initial momentum
  float momentum = 0;
  /// Probability for soft nuclear interacion
  float softInteractionProbability = 0;
  /// PDG ID based branching probabilities
  PdgMap pdgMap;
  /// Probability for nuclear interaction
  CumulativeDistribution nuclearInteractionProbability;
  /// Multiplicity in soft interactions
  CumulativeDistribution softMultiplicity;
  /// Multiplicity in hard interactions
  CumulativeDistribution hardMultiplicity;
  /// Kinematic distributions in soft interactions
  std::vector<ParametersWithFixedMultiplicity> softKinematicParameters;
  /// Kinematic distributions in hard interactions
  std::vector<ParametersWithFixedMultiplicity> hardKinematicParameters;
};

/// Parametrisation of a single particle
using NuclearInteractionParametrisation =
    std::vector<std::pair<float, NuclearInteractionParameters>>;
/// Parametrisation of multiple particles
using MultiParticleNuclearInteractionParametrisation =
    std::vector<std::pair<int, NuclearInteractionParametrisation>>;
}  // namespace ActsFatras::detail
