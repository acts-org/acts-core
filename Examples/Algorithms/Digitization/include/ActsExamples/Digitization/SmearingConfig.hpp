// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
