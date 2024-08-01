// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/TypeTraits.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/Traccc/Host/TracccChainAlgorithm.hpp"

#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace Acts::Python {

void addTracccChainHost(Context& ctx) {
  auto m = ctx.get("examples");

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::Traccc::Host::TracccChainAlgorithm, m,
      "TracccChainAlgorithmHost", inputCells, inputMeasurements, outputTracks,
      trackingGeometry, field, digitizationConfigs, chainConfig);
}

}  // namespace Acts::Python
