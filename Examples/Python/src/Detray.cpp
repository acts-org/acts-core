// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Detector/Detector.hpp"
#include "Acts/Plugins/Detray/DetrayConverter.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"

#include <memory>
#include <string>

#include <detray/builders/detector_builder.hpp>
#include <pybind11/pybind11.h>
#include <vecmem/memory/host_memory_resource.hpp>
#include <vecmem/memory/memory_resource.hpp>

namespace py = pybind11;
using namespace pybind11::literals;

using namespace Acts;
using namespace detray;
using namespace detray::io::detail;

namespace Acts::Python {

void addDetray(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  auto detray = m.def_submodule("detray");
  {
    py::class_<detector<default_metadata>,
               std::shared_ptr<detector<default_metadata>>>(detray,
                                                            "detray_detector");
  }

  {
    // This test function will convert an ACTS detector into a detray detector
    // and write it to the corresponding json files.
    //
    // The memory resource and the detector are destroyed after the function
    detray.def("writeToJson", [](const GeometryContext& gctx,
                                 const Experimental::Detector& detector) {
      auto memoryRessource = vecmem::host_memory_resource();

      DetrayConverter::Options options;
      options.writeToJson = true;
      options.convertMaterial = false;
      options.convertSurfaceGrids = true;
      auto detrayDetector =
          DetrayConverter().convert<>(gctx, detector, memoryRessource, options);
    });
  }
}
}  // namespace Acts::Python
