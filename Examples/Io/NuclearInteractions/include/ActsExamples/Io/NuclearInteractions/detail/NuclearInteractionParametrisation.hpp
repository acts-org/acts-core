// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "ActsExamples/EventData/ExtractedSimulationProcess.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <TH1F.h>
#include <TVectorFfwd.h>

namespace ActsExamples::detail::NuclearInteractionParametrisation {

/// This struct stores a fraction of an event around a nuclear
/// interaction.
struct EventFraction {
  EventFraction() = default;

  /// @brief Constructor
  ///
  /// @param [in] event Tuple containing the initial particle, the particle
  /// before the interaction and all final state particles after the interaction
  EventFraction(const ActsExamples::ExtractedSimulationProcess& event)
      : initialParticle(event.initial),
        interactingParticle(event.before),
        finalParticles(event.after) {}

  /// The initial particle
  ActsExamples::SimParticle initialParticle;
  /// The particle before the interaction
  ActsExamples::SimParticle interactingParticle;
  /// All particles after the interaction occurred
  std::vector<ActsExamples::SimParticle> finalParticles;

  /// Label whether it was a soft interaction or a hard one
  bool soft = false;
  /// The final state multiplicity
  unsigned int multiplicity = 0;
  /// The initial momentum of the particle
  float initialMomentum = 0.;
};

static constexpr std::uint32_t s_MaxValue =
    std::numeric_limits<std::uint32_t>::max();
using EventCollection = std::vector<EventFraction>;
using EventProperties = std::vector<std::vector<float>>;
using ProbabilityDistributions = std::vector<TH1F*>;
using CumulativeDistribution = TH1F*;
using Vector = Acts::ActsDynamicVector;
using Matrix = Acts::ActsDynamicMatrix;
using EigenspaceComponents = std::tuple<Vector, Matrix, Vector>;
using Parametrisation =
    std::pair<EigenspaceComponents, std::vector<CumulativeDistribution>>;

/// @brief This method scales the final state momenta by the initial momentum
///
/// @param [in] events The event storage
/// @param [in] multiplicity The target multiplicity
/// @param [in] soft Decision whether soft interactions should be considered
///
/// @return Vector containing the scaled final states
EventProperties prepareMomenta(const EventCollection& events,
                               unsigned int multiplicity, bool soft);

/// @brief This method builds the distributions of each generation of a final
/// state
///
/// @param [in] events The storage of a property with fixed multiplicity and
/// interaction type
/// @param [in] nBins Number of bins in the histogram
///
/// @return Vector containing the distributions for each generation
ProbabilityDistributions buildMomPerMult(const EventProperties& events,
                                         unsigned int nBins);

/// @brief Transform probability distribution entries to standard normal
/// distribution entries
///
/// @param [in] histos The probability distributions
/// @param [in] events The storage of a property with fixed multiplicity and
/// interaction type
///
/// @return The transformed values of @p events
EventProperties convertEventToGaussian(const ProbabilityDistributions& histos,
                                       const EventProperties& events);

/// @brief Calculate the mean and the covariance matrix of a final state
/// property
///
/// @param [in] events The storage of a property with fixed multiplicity and
/// interaction type
///
/// @return Pair containing the mean and the covariance matrix
std::pair<Vector, Matrix> calculateMeanAndCovariance(
    unsigned int multiplicity, const EventProperties& events);

/// @brief Calculate the eigenvalues, eigenvectors and the mean in eigenspace
///
/// @param [in] mean The mean of the normal distribution
/// @param [in] covariance The covariance matrix of the normal distribution
///
/// @return Tuple containing the eigenvalues, eigenvectors and the mean in
/// eigenspace
EigenspaceComponents calculateEigenspace(const Vector& mean,
                                         const Matrix& covariance);

/// @brief This function calculates all components required for simulating final
/// state momenta
///
/// @param [in] events The event storage
/// @param [in] soft Decision whether soft interactions should be considered
/// @param [in] nBins The number of bins in the histograms
///
/// @return Pair storing all components
Parametrisation buildMomentumParameters(const EventCollection& events,
                                        unsigned int multiplicity, bool soft,
                                        unsigned int nBins);

/// @brief This method calculates the final state particles invariant masses
///
/// @param [in] events The event storage
/// @param [in] multiplicity The target multiplicity
/// @param [in] soft Decision whether soft interactions should be considered
///
/// @return Vector containing the final state invariant masses
EventProperties prepareInvariantMasses(const EventCollection& events,
                                       unsigned int multiplicity, bool soft);

/// @brief This function calculates all components required for simulating final
/// state invariant masses
///
/// @param [in] events The event storage
/// @param [in] soft Decision whether soft interactions should be considered
/// @param [in] nBins The number of bins in the histograms
///
/// @return Pair storing all components
Parametrisation buildInvariantMassParameters(const EventCollection& events,
                                             unsigned int multiplicity,
                                             bool soft, unsigned int nBins);

/// @brief This method evaluates the cumulative probabilities for a given
/// particle type to produce a particle type
///
/// @param [in] events The event storage
///
/// @return Map containing the cumulative branching probabilities
std::unordered_map<int, std::unordered_map<int, float>>
cumulativePDGprobability(const EventCollection& events);

/// @brief Evaluates the cumulative probabilities for a certain multiplicity of
/// in a soft or hard process
///
/// @param [in] events The event storage
///
/// @return Pair containing the distribution for soft and hard processes
std::pair<CumulativeDistribution, CumulativeDistribution>
cumulativeMultiplicityProbability(const EventCollection& events,
                                  unsigned int multiplicityMax);

/// @brief This method evaluates the probability that a nuclear interaction is a
/// soft interaction
///
/// @param [in] events The event storage
///
/// @return The probability for soft interactions
TVectorF softProbability(const EventCollection& events);

/// @brief This method calculates the cumulative probability for a nuclear
/// interaction as a function of L0
///
/// @param [in] events The event storage
/// @param [in] interactionProbabilityBins Number of bins used for the histogram
/// @note The number of events is used for the normalisation of the
/// distribution. Hence the result is not necessarily normalised to 1. This
/// allows to sample whether a nuclear interaction occurs up to a certain value
/// of L0 or not at all.
///
/// @return The cumulative distribution for the nuclear interaction
CumulativeDistribution cumulativeNuclearInteractionProbability(
    const EventCollection& events, unsigned int interactionProbabilityBins);
}  // namespace ActsExamples::detail::NuclearInteractionParametrisation
