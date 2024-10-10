// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Detector/Blueprint.hpp"
#include "Acts/Detector/CuboidalContainerBuilder.hpp"
#include "Acts/Detector/DetectorBuilder.hpp"
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/IndexedRootVolumeFinderBuilder.hpp"
#include "Acts/Detector/detail/BlueprintDrawer.hpp"
#include "Acts/Detector/detail/BlueprintHelper.hpp"
#include "Acts/Detector/interface/IInternalStructureBuilder.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningData.hpp"

#include <algorithm>
#include <fstream>

class SurfaceBuilder : public Acts::Experimental::IInternalStructureBuilder {
 public:
  SurfaceBuilder(const Acts::Transform3& transform,
                 const Acts::RectangleBounds& sBounds)
      : m_transform(transform), m_surfaceBounds(sBounds) {};

  /// Conrstruct and return the internal structure creation
  ///
  /// @param gctx the geometry context at the creation of the internal structure
  ///
  /// @return a consistent set of detector volume internals
  Acts::Experimental::InternalStructure construct(
      [[maybe_unused]] const Acts::GeometryContext& gctx) const final {
    auto surface = Acts::Surface::makeShared<Acts::PlaneSurface>(
        (m_transform),
        std::make_shared<Acts::RectangleBounds>(m_surfaceBounds));

    // Trivialities first: internal volumes
    std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>
        internalVolumes = {};
    Acts::Experimental::ExternalNavigationDelegate internalVolumeUpdater =
        Acts::Experimental::tryNoVolumes();

    // Retrieve the layer surfaces
    Acts::Experimental::InternalNavigationDelegate internalCandidatesUpdater =
        Acts::Experimental::tryAllPortalsAndSurfaces();

    // Return the internal structure
    return Acts::Experimental::InternalStructure{
        {surface},
        internalVolumes,
        std::move(internalCandidatesUpdater),
        std::move(internalVolumeUpdater)};
  }

 private:
  Acts::Transform3 m_transform;
  Acts::RectangleBounds m_surfaceBounds;
};

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(CuboidalDetectorFromBlueprintTest) {
  Acts::GeometryContext tContext;

  // This tests shows how to careate cuboidal detector from a detector
  // blueprint.
  //
  // In general, the blueprint (lines below) is generated through reading in
  // or by parsing the geometry model (DD4heo, TGeo, Geant4, etc.). For
  // testing purpose, let us create the blueprint manually.
  //

  // Blueprint starts here ----------------

  // Detector dimensions
  Acts::ActsScalar detectorX = 100.;
  Acts::ActsScalar detectorY = 100.;
  Acts::ActsScalar detectorZ = 100.;

  // Pixel system
  Acts::ActsScalar pixelX = 20;
  Acts::ActsScalar pixelY = 100;
  Acts::ActsScalar pixelZ = 10;

  // Create  root node
  std::vector<Acts::BinningValue> detectorBins = {Acts::BinningValue::binX};
  std::vector<Acts::ActsScalar> detectorBounds = {detectorX, detectorY,
                                                  detectorZ};

  // The root node - detector
  auto detectorBpr = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "detector", Acts::Transform3::Identity(), Acts::VolumeBounds::eCuboid,
      detectorBounds, detectorBins);

  // Left arm
  std::vector<Acts::ActsScalar> leftArmBounds = {detectorX * 0.5, detectorY,
                                                 detectorZ};

  std::vector<Acts::BinningValue> leftArmBins = {Acts::BinningValue::binZ};

  Acts::Transform3 leftArmTransform =
      Acts::Transform3::Identity() *
      Acts::Translation3(-detectorX * 0.5, 0., 0);

  auto leftArm = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "leftArm", leftArmTransform, Acts::VolumeBounds::eCuboid, leftArmBounds,
      leftArmBins);

  // Pixel layer L1
  std::vector<Acts::ActsScalar> pixelL1Boundaries = {pixelX, pixelY, pixelZ};

  Acts::Transform3 pixelL1Transform =
      Acts::Transform3::Identity() *
      Acts::Translation3(-pixelX - 5, 0., -detectorZ + pixelZ + 5);

  auto pixelL1Structure = std::make_shared<SurfaceBuilder>(
      pixelL1Transform, Acts::RectangleBounds(pixelX * 0.8, pixelY * 0.8));

  auto pixelL1 = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "pixelL1", pixelL1Transform, Acts::VolumeBounds::eCuboid,
      pixelL1Boundaries, pixelL1Structure);

  // Pixel layer L2
  std::vector<Acts::ActsScalar> pixelL2Boundaries = {pixelX, pixelY, pixelZ};

  Acts::Transform3 pixelL2Transform =
      Acts::Transform3::Identity() *
      Acts::Translation3(-pixelX - 5, 0., -detectorZ + 2 * pixelZ + 5 * 5);

  auto pixelL2Structure = std::make_shared<SurfaceBuilder>(
      pixelL2Transform, Acts::RectangleBounds(pixelX * 0.8, pixelY * 0.8));

  auto pixelL2 = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "pixelL2", pixelL2Transform, Acts::VolumeBounds::eCuboid,
      pixelL2Boundaries, pixelL2Structure);

  // Add pixel layers to left arm
  // and left arm to detector
  leftArm->add(std::move(pixelL1));
  leftArm->add(std::move(pixelL2));
  detectorBpr->add(std::move(leftArm));

  // Right arm
  std::vector<Acts::ActsScalar> rightArmBounds = {detectorX * 0.5, detectorY,
                                                  detectorZ};

  std::vector<Acts::BinningValue> rightArmBins = {Acts::BinningValue::binZ};

  Acts::Transform3 rightArmTransform =
      Acts::Transform3::Identity() * Acts::Translation3(detectorX * 0.5, 0., 0);

  auto rightArm = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "rightArm", rightArmTransform, Acts::VolumeBounds::eCuboid,
      rightArmBounds, rightArmBins);

  // Pixel layer R1
  std::vector<Acts::ActsScalar> pixelR1Boundaries = {pixelX, pixelY, pixelZ};

  Acts::Transform3 pixelR1Transform =
      Acts::Transform3::Identity() *
      Acts::Translation3(pixelX + 5, 0., -detectorZ + pixelZ + 5);

  auto pixelR1Structure = std::make_shared<SurfaceBuilder>(
      pixelR1Transform, Acts::RectangleBounds(pixelX * 0.8, pixelY * 0.8));

  auto pixelR1 = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "pixelR1", pixelR1Transform, Acts::VolumeBounds::eCuboid,
      pixelR1Boundaries, pixelR1Structure);

  // Pixel layer R2
  std::vector<Acts::ActsScalar> pixelR2Boundaries = {pixelX, pixelY, pixelZ};

  Acts::Transform3 pixelR2Transform =
      Acts::Transform3::Identity() *
      Acts::Translation3(pixelX + 5, 0., -detectorZ + 2 * pixelZ + 5 * 5);

  auto pixelR2Structure = std::make_shared<SurfaceBuilder>(
      pixelR2Transform, Acts::RectangleBounds(pixelX * 0.8, pixelY * 0.8));

  auto pixelR2 = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "pixelR2", pixelR2Transform, Acts::VolumeBounds::eCuboid,
      pixelR2Boundaries, pixelR2Structure);

  // Add pixel layers to right arm
  // and right arm to detector
  rightArm->add(std::move(pixelR1));
  rightArm->add(std::move(pixelR2));
  detectorBpr->add(std::move(rightArm));

  // A geo ID generator
  detectorBpr->geoIdGenerator =
      std::make_shared<Acts::Experimental::GeometryIdGenerator>(
          Acts::Experimental::GeometryIdGenerator::Config{},
          Acts::getDefaultLogger("RecursiveIdGenerator",
                                 Acts::Logging::VERBOSE));

  std::cout << "Fill gaps ..." << std::endl;
  // Complete and fill gaps
  Acts::Experimental::detail::BlueprintHelper::fillGaps(*detectorBpr);
  std::cout << "Filled gaps ..." << std::endl;

  std::fstream fs("cylindrical_detector_blueprint.dot", std::ios::out);
  Acts::Experimental::detail::BlueprintDrawer::dotStream(fs, *detectorBpr);
  fs.close();

  // ----------------------------- end of blueprint

  // Create a Cuboidal detector builder from this blueprint
  auto detectorBuilder =
      std::make_shared<Acts::Experimental::CuboidalContainerBuilder>(
          *detectorBpr, Acts::Logging::VERBOSE);

  // Detector builder
  Acts::Experimental::DetectorBuilder::Config dCfg;
  dCfg.auxiliary = "*** Test : auto generated cuboidal detector builder  ***";
  dCfg.name = "Cuboidal detector from blueprint";
  dCfg.builder = detectorBuilder;
  dCfg.geoIdGenerator = detectorBpr->geoIdGenerator;

  auto detector = Acts::Experimental::DetectorBuilder(dCfg).construct(tContext);

  BOOST_REQUIRE_NE(detector, nullptr);

  // There should be 10 volumes, and they should be built in order
  // leftArm_gap_0
  // pixelL1
  // leftArm_gap_1
  // pixelL2
  // leftArm_gap_2
  // rightArm_gap_0
  // pixelR1
  // rightArm_gap_1
  // pixelR2
  // rightArm_gap_2
  BOOST_CHECK_EQUAL(detector->volumes().size(), 10u);
  BOOST_CHECK_EQUAL(detector->volumes()[0]->name(), "leftArm_gap_0");
  BOOST_CHECK_EQUAL(detector->volumes()[1]->name(), "pixelL1");
  BOOST_CHECK_EQUAL(detector->volumes()[2]->name(), "leftArm_gap_1");
  BOOST_CHECK_EQUAL(detector->volumes()[3]->name(), "pixelL2");
  BOOST_CHECK_EQUAL(detector->volumes()[4]->name(), "leftArm_gap_2");
  BOOST_CHECK_EQUAL(detector->volumes()[5]->name(), "rightArm_gap_0");
  BOOST_CHECK_EQUAL(detector->volumes()[6]->name(), "pixelR1");
  BOOST_CHECK_EQUAL(detector->volumes()[7]->name(), "rightArm_gap_1");
  BOOST_CHECK_EQUAL(detector->volumes()[8]->name(), "pixelR2");
  BOOST_CHECK_EQUAL(detector->volumes()[9]->name(), "rightArm_gap_2");

  // Volumes have to be contained within the
  // initial detector bounds
  double internalStretchLeftZ =
      detector->volumes()[0]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binZ)] +
      detector->volumes()[1]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binZ)] +
      detector->volumes()[2]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binZ)] +
      detector->volumes()[3]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binZ)] +
      detector->volumes()[4]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binZ)];

  double internalStretchRightZ =
      detector->volumes()[5]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binZ)] +
      detector->volumes()[6]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binZ)] +
      detector->volumes()[7]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binZ)] +
      detector->volumes()[8]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binZ)] +
      detector->volumes()[9]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binZ)];

  double internalStretchX1 =
      detector->volumes()[0]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binX)] +
      detector->volumes()[5]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binX)];

  double internalStretchX2 =
      detector->volumes()[1]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binX)] +
      detector->volumes()[6]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binX)];

  double internalStretchX3 =
      detector->volumes()[2]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binX)] +
      detector->volumes()[7]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binX)];

  double internalStretchX4 =
      detector->volumes()[3]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binX)] +
      detector->volumes()[8]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binX)];

  double internalStretchX5 =
      detector->volumes()[4]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binX)] +
      detector->volumes()[9]
          ->volumeBounds()
          .values()[toUnderlying(Acts::BinningValue::binX)];

  BOOST_CHECK_EQUAL(internalStretchLeftZ, detectorZ);
  BOOST_CHECK_EQUAL(internalStretchRightZ, detectorZ);
  BOOST_CHECK_EQUAL(internalStretchX1, detectorX);
  BOOST_CHECK_EQUAL(internalStretchX2, detectorX);
  BOOST_CHECK_EQUAL(internalStretchX3, detectorX);
  BOOST_CHECK_EQUAL(internalStretchX4, detectorX);
  BOOST_CHECK_EQUAL(internalStretchX5, detectorX);

  for (auto& volume : detector->volumes()) {
    BOOST_CHECK_EQUAL(
        volume->volumeBounds().values()[toUnderlying(Acts::BinningValue::binY)],
        detectorY);
  }

  // There should be surfaces inside the pixel
  // volumes
  BOOST_CHECK_EQUAL(detector->volumes()[1]->surfaces().size(), 1u);
  BOOST_CHECK_EQUAL(detector->volumes()[3]->surfaces().size(), 1u);
  BOOST_CHECK_EQUAL(detector->volumes()[6]->surfaces().size(), 1u);
  BOOST_CHECK_EQUAL(detector->volumes()[8]->surfaces().size(), 1u);

  // There should be 8 Z-portals from
  // connecting the arms, 4 outside Z-portals,
  // 3 X-portals from fusing the containers, and
  // 2+2 Y-portals that were replaced when connecting
  // in Z, but didn't get renewed when connecting in X
  // 8+4+3+2+2 = 19 in total
  std::vector<const Acts::Experimental::Portal*> portals;
  for (auto& volume : detector->volumes()) {
    portals.insert(portals.end(), volume->portals().begin(),
                   volume->portals().end());
  }
  std::ranges::sort(portals);
  auto last = std::unique(portals.begin(), portals.end());
  portals.erase(last, portals.end());
  BOOST_CHECK_EQUAL(portals.size(), 19u);

  // Volumes should have the same Y-normal-direction portals
  // but only within the same arm. CuboidalDetectorHelper
  // does not yet connect the containers in a consistent way
  bool samePortalY1 = true, samePortalY2 = true;
  for (int i = 0; i < 4; i++) {
    samePortalY1 =
        samePortalY1 && (detector->volumes()[i]->portals().at(4) ==
                         detector->volumes()[i + 1]->portals().at(4));
    samePortalY2 =
        samePortalY2 && (detector->volumes()[5 + i]->portals().at(4) ==
                         detector->volumes()[5 + i + 1]->portals().at(4));
  }
  BOOST_CHECK_EQUAL(samePortalY1, true);
  BOOST_CHECK_EQUAL(samePortalY2, true);
  samePortalY1 = true, samePortalY2 = true;
  for (int i = 0; i < 4; i++) {
    samePortalY1 =
        samePortalY1 && (detector->volumes()[i]->portals().at(5) ==
                         detector->volumes()[i + 1]->portals().at(5));
    samePortalY2 =
        samePortalY2 && (detector->volumes()[5 + i]->portals().at(5) ==
                         detector->volumes()[5 + i + 1]->portals().at(5));
  }
  BOOST_CHECK_EQUAL(samePortalY1, true);
  BOOST_CHECK_EQUAL(samePortalY2, true);

  // Volumes should be connected in Z-direction
  for (int i = 0; i < 4; i++) {
    bool samePortalZ1 = (detector->volumes()[i]->portals().at(1) ==
                         detector->volumes()[i + 1]->portals().at(0));
    bool samePortalZ2 = (detector->volumes()[5 + i]->portals().at(1) ==
                         detector->volumes()[5 + i + 1]->portals().at(0));

    BOOST_CHECK_EQUAL(samePortalZ1, true);
    BOOST_CHECK_EQUAL(samePortalZ2, true);
  }

  // Volumes should be connected in X-direction
  for (int i = 0; i < 5; i++) {
    bool samePortalX = (detector->volumes()[i]->portals().at(3) ==
                        detector->volumes()[5 + i]->portals().at(2));
    BOOST_CHECK_EQUAL(samePortalX, true);
  }
}

BOOST_AUTO_TEST_SUITE_END()
