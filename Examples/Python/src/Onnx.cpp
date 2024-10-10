// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/TrackFindingML/AmbiguityResolutionMLAlgorithm.hpp"
#include "ActsExamples/TrackFindingML/AmbiguityResolutionMLDBScanAlgorithm.hpp"
#include "ActsExamples/TrackFindingML/SeedFilterMLAlgorithm.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace ActsExamples;
using namespace Acts;

namespace Acts::Python {

void addOnnx(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");
  auto onnx = mex.def_submodule("_onnx");
  ctx.modules["onnx"] = onnx;

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::AmbiguityResolutionMLAlgorithm,
                                onnx, "AmbiguityResolutionMLAlgorithm",
                                inputTracks, inputDuplicateNN, outputTracks,
                                nMeasurementsMin);

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::AmbiguityResolutionMLDBScanAlgorithm, onnx,
      "AmbiguityResolutionMLDBScanAlgorithm", inputTracks, inputDuplicateNN,
      outputTracks, nMeasurementsMin, epsilonDBScan, minPointsDBScan);

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::SeedFilterMLAlgorithm, onnx,
                                "SeedFilterMLAlgorithm", inputTrackParameters,
                                inputSimSeeds, inputSeedFilterNN,
                                outputTrackParameters, outputSimSeeds,
                                epsilonDBScan, minPointsDBScan, minSeedScore);
}
}  // namespace Acts::Python
