// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Propagator/MaterialInteractor.hpp"
#include "Acts/Propagator/PropagatorStatistics.hpp"
#include "Acts/Propagator/detail/SteppingLogger.hpp"

#include <vector>

namespace ActsExamples {

struct PropagationSummary {
  explicit PropagationSummary(Acts::BoundTrackParameters startParameters_)
      : startParameters(std::move(startParameters_)) {}

  /// The start parameters
  Acts::BoundTrackParameters startParameters;

  /// Totoal number of successful steps
  std::size_t nSteps = 0;

  /// Totoal number of attempted steps
  std::size_t nStepTrials = 0;

  /// Path length
  double pathLength = 0;

  /// Steps
  std::vector<Acts::detail::Step> steps;

  /// Propagation statistics
  Acts::PropagatorStatistics statistics;
};

using PropagationSummaries = std::vector<PropagationSummary>;

/// Using some short hands for Recorded Material
using RecordedMaterial = Acts::MaterialInteractor::result_type;

/// And recorded material track
/// - this is start:  position, start momentum
///   and the Recorded material
using RecordedMaterialTrack =
    std::pair<std::pair<Acts::Vector3, Acts::Vector3>, RecordedMaterial>;

/// Finally the output of the propagation test
using PropagationOutput = std::pair<PropagationSummary, RecordedMaterial>;

}  // namespace ActsExamples
