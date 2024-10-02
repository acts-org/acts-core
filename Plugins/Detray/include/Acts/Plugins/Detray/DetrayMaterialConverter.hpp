// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Plugins/Detray/DetrayConversionUtils.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <detray/io/frontend/payloads.hpp>

namespace Acts {

namespace Experimental {
class Detector;
}

namespace DetrayMaterialConverter {

/// Conversion method for material slab
///
/// @param materialSlab the material slab object
///
/// @return a material slab payload
detray::io::material_slab_payload convertMaterialSlab(
    const MaterialSlab& materialSlab);

/// Conversion method for homogeneous material
///
/// @param geoIdCache object to have the link association from the geometry building
/// @param detector the detector object
/// @param logger the logger object for screen output
///
/// @return the volume_payload for portals and volumes by @param volume acts object
detray::io::detector_homogeneous_material_payload
convertHomogeneousSurfaceMaterial(
    const DetrayConversionUtils::GeometryIdCache& geoIdCache,
    const Experimental::Detector& detector, const Logger& logger);

/// Conversion method for grid based surface material
///
/// @param material the material (Binned/Grid)
/// @param logger the logger object for screen output
///
/// @return a surface material
detray::io::grid_payload<detray::io::material_slab_payload,
                         detray::io::material_id>
convertGridSurfaceMaterial(const ISurfaceMaterial& material,
                           const Acts::Logger& logger);

/// Conversion method for material grids
///
/// @param geoIdCache object to have the link association from the geometry building
/// @param detector the detector object
/// @param logger the logger object for screen output
///
/// @return the volume_payload for portals and volumes by @param volume acts object
detray::io::detector_grids_payload<detray::io::material_slab_payload,
                                   detray::io::material_id>
convertGridSurfaceMaterial(
    const DetrayConversionUtils::GeometryIdCache& geoIdCache,
    const Experimental::Detector& detector, const Logger& logger);

}  // namespace DetrayMaterialConverter

}  // namespace Acts
