// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
