// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Detray/DetrayConversionUtils.hpp"
#include "Acts/Plugins/Detray/DetrayConverter.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/Propagation/PropagatorInterface.hpp"
#include "ActsExamples/Traccc/DetrayPropagator.hpp"
#include "ActsExamples/Traccc/DetrayStore.hpp"

#include <detray/propagator/line_stepper.hpp>
#include <pybind11/pybind11.h>
#include <vecmem/memory/host_memory_resource.hpp>
#include <vecmem/memory/memory_resource.hpp>

namespace py = pybind11;
using namespace pybind11::literals;

using namespace Acts;
using namespace ActsExamples;

namespace Acts::Python {

void addTraccc(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  auto traccc = mex.def_submodule("traccc");

  /// Define host detray store
  {
    py::class_<DetrayHostStore, std::shared_ptr<DetrayHostStore>>(
        traccc, "DetrayHostStore");

    /// Convert the detector and create a DetrayHostStore
    ///
    /// @param gctx the geometry context
    /// @param detector the detector to be converted
    /// @param options the conversion options
    traccc.def("convertDetectorHost", [](const GeometryContext& gctx,
                                         const Experimental::Detector& detector,
                                         DetrayConverter::Options options) {
      return DetrayHostStore::create(gctx, detector, options);
    });
  }

  /// Define the DetrayPropagator
  {
    traccc.def(
        "createSlPropagatorHost",
        [](std::shared_ptr<const DetrayHostStore> detrayStore,
           bool sterile = false) {
          std::shared_ptr<PropagatorInterface> detrayProagator = nullptr;

          using DetrayLineStepper =
              detray::line_stepper<typename DetrayHostDetector::algebra_type>;

          using DetrayPropagator =
              DetrayPropagator<DetrayLineStepper, DetrayHostStore>;

          DetrayPropagator::Config cfg{detrayStore, sterile};
          detrayProagator = std::make_shared<DetrayPropagator>(cfg);
          return detrayProagator;
        });
  }
}
}  // namespace Acts::Python
