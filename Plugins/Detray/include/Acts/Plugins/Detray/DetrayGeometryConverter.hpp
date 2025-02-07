// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Plugins/Detray/DetrayConversionUtils.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <vector>

#include <detray/builders/detector_builder.hpp>
#include <detray/definitions/geometry.hpp>
#include <detray/io/frontend/detector_writer.hpp>
#include <detray/io/frontend/payloads.hpp>

namespace Acts {

class Surface;
class SurfaceBounds;

namespace Experimental {
class DetectorVolume;
class Detector;
class Portal;
}  //  namespace Experimental

namespace DetrayGeometryConverter {

/// Conversion method for transform objects to detray::transform payloads
///
/// @param t the transform to be converted
///
/// @return the transform_payload(translation, rotation)
detray::io::transform_payload convertTransform(const Transform3& t);

/// Conversion method for surface bounds to detray::mask payloads
///
/// @param bounds the bounds object to be converted
/// @param portal the flag for conversion into detray portal format
///
/// @return the mask_payload representing the bounds
detray::io::mask_payload convertMask(const SurfaceBounds& bounds,
                                     bool portal = false);

/// Conversion method for surface objects to detray::surface payloads
///
/// @param gctx the geometry context
/// @param surface the surface to be converted
/// @param portal the flag for conversion into detray portal format
///
/// @return the surface_payload for portals and volumes by @param Surface acts object
detray::io::surface_payload convertSurface(const GeometryContext& gctx,
                                           const Surface& surface,
                                           bool portal = false);

/// Conversion method for Portal object to detray::portal payloads
///
/// @param cCache [in, out] object
/// @param gctx the geometry context
/// @param portal the portal to be converted
/// @param ip the portal index
/// @param volume the volume to which the portal belongs
/// @param orientedSurfaces the oriented surfaces of the portal
///
/// @note due to portal splitting this can add up in N portals for one initial one
///
/// @brief convert the acts portal to detray surface payload and populate the payload
std::vector<detray::io::surface_payload> convertPortal(
    DetrayConversionUtils::Cache& cCache, const GeometryContext& gctx,
    const Experimental::Portal& portal, std::size_t ip,
    const Experimental::DetectorVolume& volume,
    const std::vector<OrientedSurface>& orientedSurfaces);

/// Conversion method for volume objects to detray::volume payloads
///
/// @param cCache [in, out] object
/// @param gctx the geometry context
/// @param volume the volume to be converted
/// @param detectorVolumes the detector volumes for the link lookup
/// @param logger the logger object for screen output
///
/// @return the volume_payload for portals and volumes by @param volume acts object
detray::io::volume_payload convertVolume(
    DetrayConversionUtils::Cache& cCache, const GeometryContext& gctx,
    const Experimental::DetectorVolume& volume, const Acts::Logger& logger);

/// Conversion method for detector objects to detray::detector payload
///
/// @param cCache [in, out] object
/// @param gctx the geometry context
/// @param detector the detector to be converted
/// @param logger the logger object for screen output
///
/// @return the detector_payload for portals and volumes by @param detector acts object
detray::io::detector_payload convertDetector(
    DetrayConversionUtils::Cache& cCache, const GeometryContext& gctx,
    const Experimental::Detector& detector, const Acts::Logger& logger);

}  // namespace DetrayGeometryConverter
}  // namespace Acts
