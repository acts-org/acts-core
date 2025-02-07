// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"

#include <array>
#include <memory>
#include <vector>

namespace Acts {
class TrackingGeometry;
}  // namespace Acts

namespace ActsExamples {

/// The telescope detector surface type
enum class TelescopeSurfaceType {
  Plane = 0,
  Disc = 1,
};

/// Global method to build the telescope tracking geometry
///
/// @param gctx is the detector element dependent geometry context
/// @param detectorStore is the store for the detector element
/// @param positions are the positions of different layers in the longitudinal
///                  direction
/// @param stereoAngles are the stereo angles of different layers, which are
///                     rotation angles around the longitudinal (normal)
///                     direction
/// @param offsets is the offset (u, v) of the layers in the transverse plane
/// @param bounds is the surface bound values, i.e. halfX and halfY if plane
///               surface, and minR and maxR if disc surface
/// @param thickness is the material thickness of each layer
/// @param surfaceType is the detector surface type
/// @param binValue indicates which axis the detector surface normals are
/// parallel to
std::unique_ptr<const Acts::TrackingGeometry> buildTelescopeDetector(
    const Acts::GeometryContext& gctx,
    std::vector<std::shared_ptr<const Acts::DetectorElementBase>>&
        detectorStore,
    const std::vector<double>& positions,
    const std::vector<double>& stereoAngles,
    const std::array<double, 2>& offsets, const std::array<double, 2>& bounds,
    double thickness, TelescopeSurfaceType surfaceType,
    Acts::AxisDirection binValue = Acts::AxisDirection::AxisZ);

}  // namespace ActsExamples
