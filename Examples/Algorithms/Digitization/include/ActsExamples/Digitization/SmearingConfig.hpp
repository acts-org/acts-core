// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsFatras/Digitization/UncorrelatedHitSmearer.hpp"

#include <vector>

namespace ActsExamples {

struct ParameterSmearingConfig {
  /// Which parameter does this apply to.
  Acts::BoundIndices index = Acts::eBoundSize;
  /// The smearing function for this parameter.
  ActsFatras::SingleParameterSmearFunction<RandomEngine> smearFunction;
  /// A flag to return only positive smeared values
  bool forcePositiveValues = false;
};

// The configured indices must be unique, i.e. each one can only appear once
// in a smearer configuration.
using SmearingConfig = std::vector<ParameterSmearingConfig>;

}  // namespace ActsExamples
