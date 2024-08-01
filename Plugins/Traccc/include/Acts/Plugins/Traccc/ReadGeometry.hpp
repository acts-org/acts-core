// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Traccc include(s).
#include "traccc/geometry/geometry.hpp"

// Detray include(s).
#include "detray/geometry/tracking_surface.hpp"

namespace Acts::TracccPlugin {

/// Read in the detector geometry description from a detector object
template <typename detector_t>
traccc::geometry altReadGeometry(const detector_t& det) {
  std::map<traccc::geometry_id, traccc::transform3> maps;
  using cxt_t = typename detector_t::geometry_context;
  const cxt_t ctx0{};

  for (const auto& surfaceDesc : det.surfaces()) {
    const detray::tracking_surface sf{det, surfaceDesc.barcode()};

    maps.insert({sf.barcode().value(), sf.transform(ctx0)});
  }

  return maps;
}

}  // namespace Acts::TracccPlugin
