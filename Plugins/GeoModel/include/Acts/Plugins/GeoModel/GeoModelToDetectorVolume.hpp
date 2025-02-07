// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"

#include "GeoModelKernel/GeoDefinitions.h"
class GeoShape;

namespace Acts::GeoModel {

// function converts GeoShape to Acts::Volume
Volume convertVolume(const Transform3& trf, const GeoShape& shape);

/// @brief Convert a GeoModel shape to a DetectorVolume
///
/// @param shape the GeoModel shape
/// @param transform the transform to be applied
/// @return the DetectorVolume
std::shared_ptr<Experimental::DetectorVolume> convertDetectorVolume(
    const GeometryContext& context, const GeoShape& shape,
    const std::string& name, const GeoTrf::Transform3D& transform,
    const std::vector<GeoModelSensitiveSurface>& sensitives);

}  // namespace Acts::GeoModel
