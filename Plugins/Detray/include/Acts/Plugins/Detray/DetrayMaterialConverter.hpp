// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
/// @param cCache object to have the link association from the geometry building
/// @param detector the detector object
/// @param logger the logger object for screen output
///
/// @return the volume_payload for portals and volumes by @param volume acts object
detray::io::detector_homogeneous_material_payload
convertHomogeneousSurfaceMaterial(const DetrayConversionUtils::Cache& cCache,
                                  const Experimental::Detector& detector,
                                  const Logger& logger);

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
/// @param cCache object to have the link association from the geometry building
/// @param detector the detector object
/// @param logger the logger object for screen output
///
/// @return the volume_payload for portals and volumes by @param volume acts object
detray::io::detector_grids_payload<detray::io::material_slab_payload,
                                   detray::io::material_id>
convertGridSurfaceMaterial(const DetrayConversionUtils::Cache& cCache,
                           const Experimental::Detector& detector,
                           const Logger& logger);

}  // namespace DetrayMaterialConverter

}  // namespace Acts
