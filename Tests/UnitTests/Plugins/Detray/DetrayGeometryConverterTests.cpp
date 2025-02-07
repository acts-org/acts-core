// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/Detray/DetrayGeometryConverter.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalDetector.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <numbers>
#include <vector>

#include <detray/io/frontend/payloads.hpp>

using namespace Acts;
using namespace Acts::Experimental;
using namespace Acts::Test;

GeometryContext tContext;

auto logger =
    Acts::getDefaultLogger("DetrayGeometryConverterTests", Acts::Logging::INFO);

BOOST_AUTO_TEST_SUITE(DetrayConversion)

BOOST_AUTO_TEST_CASE(DetrayTransformConversion) {
  auto transform = Transform3::Identity();
  transform.pretranslate(Vector3(1., 2., 3.));
  transform.rotate(Eigen::AngleAxisd(std::numbers::pi / 2., Vector3::UnitZ()));

  detray::io::transform_payload payload =
      DetrayGeometryConverter::convertTransform(transform);
  // Transform is correctly translated
  CHECK_CLOSE_ABS(payload.tr[0u], 1., std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.tr[1u], 2., std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.tr[2u], 3., std::numeric_limits<double>::epsilon());
  // Rotation is correctly translated
  RotationMatrix3 rotation = transform.rotation().transpose();
  CHECK_CLOSE_ABS(payload.rot[0u], rotation(0, 0),
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.rot[1u], rotation(0, 1),
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.rot[2u], rotation(0, 2),
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.rot[3u], rotation(1, 0),
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.rot[4u], rotation(1, 1),
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.rot[5u], rotation(1, 2),
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.rot[6u], rotation(2, 0),
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.rot[7u], rotation(2, 1),
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.rot[8u], rotation(2, 2),
                  std::numeric_limits<double>::epsilon());
}

BOOST_AUTO_TEST_CASE(DetrayMaskConversion) {
  // Placeholder, masks are tested for the moment through the DetrayJsonHelper,
  // this code will move over here when the "toJsonDetray" will become
  // deprecated
}

BOOST_AUTO_TEST_CASE(DetraySurfaceConversion) {
  // Translate a cylinder
  auto cylinderSurface = Acts::Surface::makeShared<CylinderSurface>(
      Transform3::Identity(), std::make_shared<CylinderBounds>(20., 100.));

  auto sgID = Acts::GeometryIdentifier().setSensitive(1);
  cylinderSurface->assignGeometryId(sgID);

  detray::io::surface_payload payload = DetrayGeometryConverter::convertSurface(
      tContext, *cylinderSurface, false);

  // Check the payload
  BOOST_CHECK(!payload.index_in_coll.has_value());
  BOOST_CHECK(payload.mask.shape == detray::io::shape_id::cylinder2);
  BOOST_CHECK_EQUAL(payload.source, sgID.value());
  BOOST_CHECK(payload.type == detray::surface_id::e_sensitive);
}

BOOST_AUTO_TEST_CASE(DetrayVolumeConversion) {
  auto beampipe = std::make_shared<
      CylindricalVolumeBuilder<CylinderSurface, CylinderBounds>>(
      Transform3::Identity(), CylinderVolumeBounds(0., 50., 400.),
      CylinderBounds(25., 380.), "BeamPipe");

  auto [volumes, portals, rootVolumes] = beampipe->construct(tContext);
  auto volume = volumes.front();

  std::vector<const Experimental::DetectorVolume*> dVolumes = {volume.get()};
  DetrayConversionUtils::Cache cCache(dVolumes);

  detray::io::volume_payload payload = DetrayGeometryConverter::convertVolume(
      cCache, tContext, *volume, *logger);

  // Check the volume payload
  BOOST_CHECK(payload.name == "BeamPipe");
  BOOST_CHECK(payload.type == detray::volume_id::e_cylinder);
  // 3 portals and 1 surface contained
  BOOST_CHECK_EQUAL(payload.surfaces.size(), 4u);
  // Let's count
  std::size_t nPortals = 0;
  for (auto& s : payload.surfaces) {
    if (s.type == detray::surface_id::e_portal) {
      nPortals++;
    }
  }
  BOOST_CHECK_EQUAL(nPortals, 3u);
  // No acceleration structure for the moment
  BOOST_CHECK(!payload.acc_links.has_value());
}

BOOST_AUTO_TEST_CASE(CylindricalDetector) {
  // Load the detector from the Test utilities
  auto detector = buildCylindricalDetector(tContext);

  // Convert the detector
  DetrayConversionUtils::Cache cCache(detector->volumes());

  detray::io::detector_payload payload =
      DetrayGeometryConverter::convertDetector(cCache, tContext, *detector,
                                               *logger);

  // Test the payload - we have six volumes
  BOOST_CHECK_EQUAL(payload.volumes.size(), 6u);

  // The first volume is the beam pipe volume
  BOOST_CHECK_EQUAL(payload.volumes[0].name, "BeamPipe");
  // The beam pipe should have 1 surface and 5 portals
  // the original cylinder cover is split into 3 portals
  BOOST_CHECK_EQUAL(payload.volumes[0].surfaces.size(), 6u);
  // The second volume should be the negative endcap
  BOOST_CHECK_EQUAL(payload.volumes[1].name, "NegativeEndcap");
  // The negative endcap should have 1 surface and 6 portals
  BOOST_CHECK_EQUAL(payload.volumes[1].surfaces.size(), 7u);
  // Barrel 0,1,2 follow
  BOOST_CHECK_EQUAL(payload.volumes[2].name, "Barrel0");
  BOOST_CHECK_EQUAL(payload.volumes[3].name, "Barrel1");
  BOOST_CHECK_EQUAL(payload.volumes[4].name, "Barrel2");
  // No portal splitting for those, hence we remain at 5 surfaces
  // i.e. 4 portals and one surface
  BOOST_CHECK_EQUAL(payload.volumes[2].surfaces.size(), 5u);
  BOOST_CHECK_EQUAL(payload.volumes[3].surfaces.size(), 5u);
  BOOST_CHECK_EQUAL(payload.volumes[4].surfaces.size(), 5u);
  // Finally the positive endcap
  BOOST_CHECK_EQUAL(payload.volumes[5].name, "PositiveEndcap");
  // The positive endcap should have again 1 surface and 6 portals
  BOOST_CHECK_EQUAL(payload.volumes[5].surfaces.size(), 7u);
}

BOOST_AUTO_TEST_SUITE_END()
