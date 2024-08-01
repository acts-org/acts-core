// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <vector>

#include "detray/builders/detector_builder.hpp"
#include "detray/core/detector.hpp"
#include "detray/definitions/geometry.hpp"
#include "detray/io/common/geometry_reader.hpp"
#include "detray/io/frontend/detector_writer.hpp"
#include "detray/io/frontend/payloads.hpp"
#include "detray/utils/consistency_checker.hpp"

namespace Acts {

namespace DetrayConverter {

using DetrayDetector = detray::detector<detray::default_metadata>;

/// Write the detector to json output
///
/// @param dDetector is the detray detector (converted)
/// @param names a name map for the detector volumes
/// @param writer_cfg the writer configuration
void writeToJson(const DetrayDetector& dDetector,
                 const typename DetrayDetector::name_map& names = {},
                 detray::io::detector_writer_config writer_cfg = {});

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
/// @param gctx the geometry context
/// @param portal the portal to be converted
/// @param ip the portal index
/// @param volume the volume to which the portal belongs
/// @param orientedSurfaces the oriented surfaces of the portal
/// @param detectorVolumes the detector volumes for the link lookup
///
/// @note due to portal splitting this can add up in N portals for one initial one
///
/// @brief convert the acts portal to detray surface payload and populate the payload
std::vector<detray::io::surface_payload> convertPortal(
    const GeometryContext& gctx, const Experimental::Portal& portal,
    std::size_t ip, const Experimental::DetectorVolume& volume,
    const std::vector<OrientedSurface>& orientedSurfaces,
    const std::vector<const Experimental::DetectorVolume*>& detectorVolumes);

/// Conversion method for volume objects to detray::volume payloads
///
/// @param gctx the geometry context
/// @param volume the volume to be converted
/// @param detectorVolumes the detector volumes for the link lookup
///
/// @return the volume_payload for portals and volumes by @param volume acts object
detray::io::volume_payload convertVolume(
    const GeometryContext& gctx, const Experimental::DetectorVolume& volume,
    const std::vector<const Experimental::DetectorVolume*>& detectorVolumes);

/// Conversion method for (common) header payload
///
/// @param detector is the detector to be converted
///
/// @return a geometry header payload
detray::io::geo_header_payload convertHead(
    const Acts::Experimental::Detector& detector);

/// Convert an Acts::Experimental::Detector to a detray::detector object
///
/// @param gctx the geometry context
/// @param detector the detector to be converted
/// @param mr the memory resource to be used
///
/// @returns a detector of requested return type
template <typename detector_t = DetrayDetector>
std::tuple<detector_t, vecmem::memory_resource&> convertDetector(
    const Acts::GeometryContext& gctx,
    const Acts::Experimental::Detector& detector, vecmem::memory_resource& mr) {
  detray::io::detector_payload detectorPayload;
  for (const auto volume : detector.volumes()) {
    detectorPayload.volumes.push_back(
        convertVolume(gctx, *volume, detector.volumes()));
  }
  typename detector_t::name_map names = {{0u, detector.name()}};

  // build detector
  detray::detector_builder<detray::default_metadata> detectorBuilder{};
  detray::io::geometry_reader::convert<detector_t>(detectorBuilder, names,
                                                   detectorPayload);
  detector_t detrayDetector(detectorBuilder.build(mr));

  // checks and print
  detray::detail::check_consistency(detrayDetector);
  converterPrint(detrayDetector, names);

  return {std::move(detrayDetector), mr};
}

}  // namespace DetrayConverter
}  // namespace Acts
