// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/TrackFindingML/AmbiguityResolutionMLAlgorithm.hpp"
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

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::SeedFilterMLAlgorithm, onnx,
                                "SeedFilterMLAlgorithm", inputTrackParameters,
                                inputSimSeeds, inputSeedFilterNN,
                                outputTrackParameters, outputSimSeeds,
                                epsilonDBScan, minPointsDBScan, minSeedScore);
}
}  // namespace Acts::Python
