// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/AmbiguityResolution/GreedyAmbiguityResolutionAlgorithm.hpp"
#include "ActsExamples/AmbiguityResolution/ScoreBasedAmbiguityResolutionAlgorithm.hpp"

#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace Acts;
using namespace ActsExamples;

namespace Acts::Python {

void addAmbiguityResolution(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::GreedyAmbiguityResolutionAlgorithm, mex,
      "GreedyAmbiguityResolutionAlgorithm", inputTracks, outputTracks,
      maximumSharedHits, maximumIterations, nMeasurementsMin);

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::ScoreBasedAmbiguityResolutionAlgorithm, mex,
      "ScoreBasedAmbiguityResolutionAlgorithm", inputTracks, configFile,
      outputTracks, minScore, minScoreSharedTracks, maxShared,
      maxSharedTracksPerMeasurement, pTMin, pTMax, phiMin, phiMax, etaMin,
      etaMax, useAmbiguityFunction);
}

}  // namespace Acts::Python
