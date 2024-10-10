// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Hashing/HashingAlgorithmConfig.hpp"
#include "Acts/Plugins/Hashing/HashingTrainingConfig.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/TrackFinding/SeedingAlgorithmHashing.hpp"

#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace ActsExamples;
using namespace Acts;

namespace Acts::Python {

void addHashing(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  auto hashingModule = m.def_submodule("hashing");
  auto hashingExampleModule = mex.def_submodule("_hashing");

  {
    using Config = Acts::HashingAlgorithmConfig;
    auto c = py::class_<Config>(hashingModule, "HashingAlgorithmConfig")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(bucketSize);
    ACTS_PYTHON_MEMBER(zBins);
    ACTS_PYTHON_MEMBER(phiBins);
    ACTS_PYTHON_STRUCT_END();
    patchKwargsConstructor(c);
  }

  {
    using Config = Acts::HashingTrainingConfig;
    auto c = py::class_<Config>(hashingModule, "HashingTrainingConfig")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(annoySeed);
    ACTS_PYTHON_MEMBER(f);
    ACTS_PYTHON_STRUCT_END();
    patchKwargsConstructor(c);
  }

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::SeedingAlgorithmHashing, hashingExampleModule,
      "SeedingAlgorithmHashing", inputSpacePoints, outputSeeds, outputBuckets,
      seedFilterConfig, seedFinderConfig, seedFinderOptions, gridConfig,
      gridOptions, allowSeparateRMax, zBinNeighborsTop, zBinNeighborsBottom,
      numPhiNeighbors, hashingConfig, hashingTrainingConfig, useExtraCuts);
}

}  // namespace Acts::Python
