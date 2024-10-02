// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Detector/Blueprint.hpp"
#include "Acts/Detector/CylindricalContainerBuilder.hpp"
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
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Utilities/BinningData.hpp"

#include <fstream>

template <typename surface_type>
class SurfaceBuilder : public Acts::Experimental::IInternalStructureBuilder {
 public:
  SurfaceBuilder(const Acts::Transform3& trf, Acts::ActsScalar p0,
                 Acts::ActsScalar p1)
      : m_surface(Acts::Surface::makeShared<surface_type>(trf, p0, p1)) {}
  /// Conrstruct and return the internal structure creation
  ///
  /// @param gctx the geometry context at the creation of the internal structure
  ///
  /// @return a consistent set of detector volume internals
  Acts::Experimental::InternalStructure construct(
      [[maybe_unused]] const Acts::GeometryContext& gctx) const final {
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
        {m_surface},
        internalVolumes,
        std::move(internalCandidatesUpdater),
        std::move(internalVolumeUpdater)};
  }

 private:
  std::shared_ptr<Acts::Surface> m_surface;
};

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(CylindricalDetectorFromBlueprintTest) {
  Acts::GeometryContext tContext;

  // This tests shows how to careate cylindrical detector from a detector
  // blueprint.
  //
  // In general, the blueprint (lines below) is generated through reading in
  // or by parsing the geometry model (DD4heo, TGeo, Geant4, etc.). For
  // testing purpose, let us create the blueprint manually.
  //

  // Blueprint starts here ----------------

  // Detector dimensions
  Acts::ActsScalar detectorIr = 0.;
  Acts::ActsScalar detectorOr = 120.;
  Acts::ActsScalar detectorHz = 400.;

  // Beam pipe
  Acts::ActsScalar beamPipeOr = 20.;

  // Pixel system
  Acts::ActsScalar pixelIr = 25;
  Acts::ActsScalar pixelOr = 115;
  Acts::ActsScalar pixelEcHz = 50;
  Acts::ActsScalar pixelEcLayerHz = 10;

  // Create  root node
  std::vector<Acts::BinningValue> detectorBinning = {Acts::BinningValue::binR};
  std::vector<Acts::ActsScalar> detectorBoundaries = {detectorIr, detectorOr,
                                                      detectorHz};

  // The root node - detector
  auto detectorBpr = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "detector", Acts::Transform3::Identity(), Acts::VolumeBounds::eCylinder,
      detectorBoundaries, detectorBinning);

  // The beam pipe
  std::vector<Acts::ActsScalar> beamPipeBoundaries = {detectorIr, beamPipeOr,
                                                      detectorHz};

  auto beamPipeStructure =
      std::make_shared<SurfaceBuilder<Acts::CylinderSurface>>(
          Acts::Transform3::Identity(), 18, 0.99 * detectorHz);
  auto beamPipe = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "beam_pipe", Acts::Transform3::Identity(), Acts::VolumeBounds::eCylinder,
      beamPipeBoundaries, beamPipeStructure);
  detectorBpr->add(std::move(beamPipe));

  // A pixel system
  std::vector<Acts::ActsScalar> pixelBoundaries = {pixelIr, pixelOr,
                                                   detectorHz};
  std::vector<Acts::BinningValue> pixelBinning = {Acts::BinningValue::binZ};
  auto pixel = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "pixel", Acts::Transform3::Identity(), Acts::VolumeBounds::eCylinder,
      pixelBoundaries, pixelBinning);

  // Nec: Small differences to check if the adjustments are made
  std::vector<Acts::ActsScalar> pixelEcBoundaries = {pixelIr, pixelOr - 5.,
                                                     pixelEcHz};
  std::vector<Acts::BinningValue> pixelEcBinning = {Acts::BinningValue::binZ};

  Acts::Transform3 pixelNecTransform =
      Acts::Transform3::Identity() *
      Acts::Translation3(0., 0., -detectorHz + pixelEcHz);

  auto pixelNec = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "pixel_nec", pixelNecTransform, Acts::VolumeBounds::eCylinder,
      pixelEcBoundaries, pixelEcBinning);

  // Add a single encap layer
  std::vector<Acts::ActsScalar> pixelNecBoundaries = {pixelIr + 2, pixelOr - 7.,
                                                      pixelEcLayerHz};

  auto pixelNecLayerStructure =
      std::make_shared<SurfaceBuilder<Acts::DiscSurface>>(
          pixelNecTransform, pixelIr + 10., pixelOr - 10.);

  auto pixelNecLayer = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "pixel_nec_layer", pixelNecTransform, Acts::VolumeBounds::eCylinder,
      pixelNecBoundaries, pixelNecLayerStructure);

  pixelNec->add(std::move(pixelNecLayer));

  // Barrel
  std::vector<Acts::ActsScalar> pixelBarrelBoundaries = {
      pixelIr + 1, pixelOr - 1., detectorHz - 2 * pixelEcHz};
  std::vector<Acts::BinningValue> pixelBarrelBinning = {
      Acts::BinningValue::binR};

  auto pixelBarrel = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "pixel_barrel", Acts::Transform3::Identity(),
      Acts::VolumeBounds::eCylinder, pixelBarrelBoundaries, pixelBarrelBinning);

  auto pixelBarrelL0Structure =
      std::make_shared<SurfaceBuilder<Acts::CylinderSurface>>(
          Acts::Transform3::Identity(), 62.5, detectorHz - 2 * pixelEcHz - 10.);
  std::vector<Acts::ActsScalar> pixelBarrelL0Boundaries = {
      60, 65., detectorHz - 2 * pixelEcHz};
  auto pixelBarrelL0 = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "pixel_barrel_l0", Acts::Transform3::Identity(),
      Acts::VolumeBounds::eCylinder, pixelBarrelL0Boundaries,
      pixelBarrelL0Structure);

  auto pixelBarrelL1Structure =
      std::make_shared<SurfaceBuilder<Acts::CylinderSurface>>(
          Acts::Transform3::Identity(), 102.5,
          detectorHz - 2 * pixelEcHz - 10.);

  std::vector<Acts::ActsScalar> pixelBarrelL1Boundaries = {
      100, 105., detectorHz - 2 * pixelEcHz};
  auto pixelBarrelL1 = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "pixel_barrel_l1", Acts::Transform3::Identity(),
      Acts::VolumeBounds::eCylinder, pixelBarrelL1Boundaries,
      pixelBarrelL1Structure);
  pixelBarrel->add(std::move(pixelBarrelL0));
  pixelBarrel->add(std::move(pixelBarrelL1));

  Acts::Transform3 pixelPecTransform =
      Acts::Transform3::Identity() *
      Acts::Translation3(0., 0., detectorHz - pixelEcHz);

  auto pixelPec = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "pixel_pec", pixelPecTransform, Acts::VolumeBounds::eCylinder,
      pixelEcBoundaries, pixelEcBinning);

  std::vector<Acts::ActsScalar> pixelPecBoundaries = {pixelIr + 2, pixelOr - 7.,
                                                      10.};

  auto pixelPecLayerStructure =
      std::make_shared<SurfaceBuilder<Acts::DiscSurface>>(
          pixelPecTransform, pixelIr + 10., pixelOr - 10.);

  auto pixelPecLayer = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "pixel_pec_layer", pixelPecTransform, Acts::VolumeBounds::eCylinder,
      pixelPecBoundaries, pixelPecLayerStructure);

  pixelPec->add(std::move(pixelPecLayer));

  // Adding pixel
  pixel->add(std::move(pixelNec));
  pixel->add(std::move(pixelPec));
  pixel->add(std::move(pixelBarrel));

  detectorBpr->add(std::move(pixel));

  // An Indexed volume finder will be attached
  std::vector<Acts::BinningValue> rootVolumeBinning = {
      Acts::BinningValue::binZ, Acts::BinningValue::binR};
  detectorBpr->rootVolumeFinderBuilder =
      std::make_shared<Acts::Experimental::IndexedRootVolumeFinderBuilder>(
          rootVolumeBinning);

  // A geo ID generator
  detectorBpr->geoIdGenerator =
      std::make_shared<Acts::Experimental::GeometryIdGenerator>(
          Acts::Experimental::GeometryIdGenerator::Config{},
          Acts::getDefaultLogger("RecursiveIdGenerator",
                                 Acts::Logging::VERBOSE));

  // Complete and fill gaps
  Acts::Experimental::detail::BlueprintHelper::fillGaps(*detectorBpr);

  std::fstream fs("cylindrical_detector_blueprint.dot", std::ios::out);
  Acts::Experimental::detail::BlueprintDrawer::dotStream(fs, *detectorBpr);
  fs.close();

  // ----------------------------- end of blueprint

  // Create a Cylindrical detector builder from this blueprint
  auto detectorBuilder =
      std::make_shared<Acts::Experimental::CylindricalContainerBuilder>(
          *detectorBpr, Acts::Logging::VERBOSE);

  // Detector builder
  Acts::Experimental::DetectorBuilder::Config dCfg;
  dCfg.auxiliary =
      "*** Test : auto generated cylindrical detector builder  ***";
  dCfg.name = "Cylindrical detector from blueprint";
  dCfg.builder = detectorBuilder;
  dCfg.geoIdGenerator = detectorBpr->geoIdGenerator;

  auto detector = Acts::Experimental::DetectorBuilder(dCfg).construct(tContext);

  BOOST_REQUIRE_NE(detector, nullptr);

  // There should be 14 volumes, and they should be built in order
  // beam_pipe
  // detector_gap_0
  // pixel_nec_gap_0
  // pixel_nec_layer
  // pixel_nec_gap_1
  // pixel_barrel_gap_0
  // pixel_barrel_l0
  // pixel_barrel_gap_1
  // pixel_barrel_l1
  // pixel_barrel_gap_2
  // pixel_pec_gap_0
  // pixel_pec_layer
  // pixel_pec_gap_1
  // detector_gap_1
  BOOST_CHECK_EQUAL(detector->volumes().size(), 14u);
  BOOST_CHECK_EQUAL(detector->volumes()[0]->name(), "beam_pipe");
  BOOST_CHECK_EQUAL(detector->volumes()[1]->name(), "detector_gap_0");
  BOOST_CHECK_EQUAL(detector->volumes()[2]->name(), "pixel_nec_gap_0");
  BOOST_CHECK_EQUAL(detector->volumes()[3]->name(), "pixel_nec_layer");
  BOOST_CHECK_EQUAL(detector->volumes()[4]->name(), "pixel_nec_gap_1");
  BOOST_CHECK_EQUAL(detector->volumes()[5]->name(), "pixel_barrel_gap_0");
  BOOST_CHECK_EQUAL(detector->volumes()[6]->name(), "pixel_barrel_l0");
  BOOST_CHECK_EQUAL(detector->volumes()[7]->name(), "pixel_barrel_gap_1");
  BOOST_CHECK_EQUAL(detector->volumes()[8]->name(), "pixel_barrel_l1");
  BOOST_CHECK_EQUAL(detector->volumes()[9]->name(), "pixel_barrel_gap_2");
  BOOST_CHECK_EQUAL(detector->volumes()[10]->name(), "pixel_pec_gap_0");
  BOOST_CHECK_EQUAL(detector->volumes()[11]->name(), "pixel_pec_layer");
  BOOST_CHECK_EQUAL(detector->volumes()[12]->name(), "pixel_pec_gap_1");
  BOOST_CHECK_EQUAL(detector->volumes()[13]->name(), "detector_gap_1");
}

BOOST_AUTO_TEST_SUITE_END()
