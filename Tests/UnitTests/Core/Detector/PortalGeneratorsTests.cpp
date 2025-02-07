// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Navigation/NavigationState.hpp"

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

using namespace Acts::Experimental;

// A test context
Acts::GeometryContext tContext;

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(CylindricalPortalGenerator) {
  // Access Vectors, they should yield the detector volume
  // as a return on nextVolume(...) call
  Acts::Vector3 negPos(-200., 0., 0.);
  Acts::Vector3 negDir(0., 0., 1.);

  Acts::Vector3 posPos(200., 0., 0.);
  Acts::Vector3 posDir(0., 0., -1.);

  Acts::Vector3 outerPos(100., 0., 0.);
  Acts::Vector3 outerDir(-1., 0., 0.);

  Acts::Vector3 innerPos(10., 0., 0.);
  Acts::Vector3 innerDir(1., 0., 0.);

  // Filled Cylinder
  Acts::CylinderVolumeBounds cBar(0., 100, 200.);

  auto dTransform = Acts::Transform3::Identity();
  auto pGenerator = defaultPortalGenerator();
  auto dVolume = DetectorVolumeFactory::construct(
      pGenerator, tContext, "dummy", dTransform,
      std::make_unique<Acts::CuboidVolumeBounds>(1, 1, 1),
      tryAllPortalsAndSurfaces());

  auto cBarPortals = generatePortals(dTransform, cBar, dVolume);

  BOOST_CHECK_EQUAL(cBarPortals.size(), 3u);
  // Check they are not nullptrs
  for (const auto& p : cBarPortals) {
    BOOST_REQUIRE_NE(p, nullptr);
  }

  // Pointing inside the volume
  NavigationState nState;

  auto testDetectorVolumeUpdate =
      [&](const Acts::Experimental::Portal& portal,
          const Acts::Vector3& position, const Acts::Vector3& direction,
          const Acts::Experimental::DetectorVolume* expected) -> void {
    nState.position = position;
    nState.direction = direction;
    portal.updateDetectorVolume(tContext, nState);
    BOOST_CHECK_EQUAL(nState.currentVolume, expected);
  };

  testDetectorVolumeUpdate(*cBarPortals[0], negPos, negDir, dVolume.get());
  testDetectorVolumeUpdate(*cBarPortals[1], posPos, posDir, dVolume.get());
  testDetectorVolumeUpdate(*cBarPortals[2], outerPos, outerDir, dVolume.get());

  testDetectorVolumeUpdate(*cBarPortals[0], negPos, -negDir, nullptr);
  testDetectorVolumeUpdate(*cBarPortals[1], posPos, -posDir, nullptr);
  testDetectorVolumeUpdate(*cBarPortals[2], outerPos, -outerDir, nullptr);

  // Tube Cylinder
  Acts::CylinderVolumeBounds cTube(10., 100, 200.);
  auto cTubePortals = generatePortals(dTransform, cTube, dVolume);
  BOOST_CHECK_EQUAL(cTubePortals.size(), 4u);
  // Check they are not nullptrs
  for (const auto& p : cTubePortals) {
    BOOST_REQUIRE_NE(p, nullptr);
  }

  testDetectorVolumeUpdate(*cTubePortals[0], negPos, negDir, dVolume.get());
  testDetectorVolumeUpdate(*cTubePortals[1], posPos, posDir, dVolume.get());
  testDetectorVolumeUpdate(*cTubePortals[2], outerPos, outerDir, dVolume.get());
  testDetectorVolumeUpdate(*cTubePortals[3], innerPos, innerDir, dVolume.get());

  testDetectorVolumeUpdate(*cTubePortals[0], negPos, -negDir, nullptr);
  testDetectorVolumeUpdate(*cTubePortals[1], posPos, -posDir, nullptr);
  testDetectorVolumeUpdate(*cTubePortals[2], outerPos, -outerDir, nullptr);
  testDetectorVolumeUpdate(*cTubePortals[3], innerPos, -innerDir, nullptr);

  // Sectoral tube cylinder
  double alpha = std::numbers::pi / 4.;
  double r = 50;

  Acts::Vector3 negPhiSecPos(r * std::cos(-alpha), r * std::sin(-alpha), 0.);
  Acts::Vector3 negPhiSecDir(-r * std::cos(-alpha), r * std::sin(-alpha), 0.);
  Acts::Vector3 posPhiSecPos(r * std::cos(alpha), r * std::sin(alpha), 0.);
  Acts::Vector3 posPhiSecDir(r * std::cos(alpha), -r * std::sin(alpha), 0.);

  Acts::CylinderVolumeBounds cTubeSector(10., 100., 200., alpha, 0.);
  auto cTubeSectorPortals = generatePortals(dTransform, cTubeSector, dVolume);
  BOOST_CHECK_EQUAL(cTubeSectorPortals.size(), 6u);
  // Check they are not nullptrs
  for (const auto& p : cTubeSectorPortals) {
    BOOST_REQUIRE_NE(p, nullptr);
  }

  // Pointing inside the volume
  testDetectorVolumeUpdate(*cTubeSectorPortals[0], negPos, negDir,
                           dVolume.get());
  testDetectorVolumeUpdate(*cTubeSectorPortals[1], posPos, posDir,
                           dVolume.get());
  testDetectorVolumeUpdate(*cTubeSectorPortals[2], outerPos, outerDir,
                           dVolume.get());
  testDetectorVolumeUpdate(*cTubeSectorPortals[3], innerPos, innerDir,
                           dVolume.get());
  testDetectorVolumeUpdate(*cTubeSectorPortals[4], negPhiSecPos, negPhiSecDir,
                           dVolume.get());
  testDetectorVolumeUpdate(*cTubeSectorPortals[5], posPhiSecPos, posPhiSecDir,
                           dVolume.get());

  // Pointing to nowhere land
  testDetectorVolumeUpdate(*cTubeSectorPortals[0], negPos, -negDir, nullptr);
  testDetectorVolumeUpdate(*cTubeSectorPortals[1], posPos, -posDir, nullptr);
  testDetectorVolumeUpdate(*cTubeSectorPortals[2], outerPos, -outerDir,
                           nullptr);
  testDetectorVolumeUpdate(*cTubeSectorPortals[3], innerPos, -innerDir,
                           nullptr);
  testDetectorVolumeUpdate(*cTubeSectorPortals[4], negPhiSecPos, -negPhiSecDir,
                           nullptr);
  testDetectorVolumeUpdate(*cTubeSectorPortals[5], posPhiSecPos, -posPhiSecDir,
                           nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
