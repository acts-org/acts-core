// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
      auto range = sExtent.range(b.axisDir);
      fBinning = ProtoBinning(b.axisDir, b.boundaryType, range.min(),
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
