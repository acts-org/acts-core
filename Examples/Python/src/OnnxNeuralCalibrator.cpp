// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Python/Utilities.hpp"
#include <ActsExamples/EventData/NeuralCalibrator.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace ActsExamples;
using namespace Acts;

namespace Acts::Python {

void addOnnxNeuralCalibrator(Context &ctx) {
  auto [m, mex, onnx] = ctx.get("main", "examples", "onnx");

  onnx.def(
      "makeNeuralCalibrator",
      [](const char *modelPath, std::size_t nComp,
         std::vector<std::size_t> volumeIds)
          -> std::shared_ptr<MeasurementCalibrator> {
        return std::make_shared<NeuralCalibrator>(modelPath, nComp, volumeIds);
      },
      py::arg("modelPath"), py::arg("nComp") = 1,
      py::arg("volumeIds") = std::vector<std::size_t>({7, 8, 9}));
}
}  // namespace Acts::Python
