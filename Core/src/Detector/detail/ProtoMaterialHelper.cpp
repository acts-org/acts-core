// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/detail/ProtoMaterialHelper.hpp"

#include "Acts/Detector/ProtoBinning.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Surfaces/Surface.hpp"

Acts::Experimental::BinningDescription
Acts::Experimental::detail::ProtoMaterialHelper::attachProtoMaterial(
    const GeometryContext& gctx, Surface& surface,
    const BinningDescription& bDescription) {
  // The binning description, with eventually fixed range
  BinningDescription fbDescription;
  // Measure the surface
  Extent sExtent = surface.polyhedronRepresentation(gctx, 1).extent();
  for (const auto& b : bDescription.binning) {
    ProtoBinning fBinning = b;
    // Check if the binning needs to be fixed
    if (fBinning.autorange) {
      auto range = sExtent.range(b.binValue);
      fBinning = ProtoBinning(b.binValue, b.boundaryType, range.min(),
                              range.max(), b.bins(), b.expansion);
    }
    fbDescription.binning.push_back(fBinning);
  }
  // Create the new proto material description and assign it
  auto protoMaterial =
      std::make_shared<ProtoGridSurfaceMaterial>(fbDescription);
  surface.assignSurfaceMaterial(protoMaterial);
  // Return the (fixed) binning description
  return fbDescription;
}
