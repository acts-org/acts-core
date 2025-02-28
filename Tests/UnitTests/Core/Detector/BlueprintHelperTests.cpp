// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Detector/Blueprint.hpp"
#include "Acts/Detector/detail/BlueprintDrawer.hpp"
#include "Acts/Detector/detail/BlueprintHelper.hpp"

#include <exception>
#include <fstream>

namespace Acts::Experimental {
class IInternalStructureBuilder {};
}  // namespace Acts::Experimental

BOOST_AUTO_TEST_SUITE(Experimental)

BOOST_AUTO_TEST_CASE(BlueprintHelperSorting) {
  // Create  root node
  std::vector<Acts::AxisDirection> detectorBinning = {
      Acts::AxisDirection::AxisR};
  std::vector<double> detectorBoundaries = {0., 50., 100.};
  auto detector = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "detector", Acts::Transform3::Identity(), Acts::VolumeBounds::eCylinder,
      detectorBoundaries, detectorBinning);

  BOOST_CHECK_EQUAL(detector->parent, nullptr);
  BOOST_CHECK(detector->children.empty());
  BOOST_CHECK_EQUAL(detector->name, "detector");

  std::vector<Acts::AxisDirection> pixelsBinning = {Acts::AxisDirection::AxisZ};
  std::vector<double> pixelsBoundaries = {20., 50., 100.};

  auto pixels = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "pixels", Acts::Transform3::Identity(), Acts::VolumeBounds::eCylinder,
      pixelsBoundaries, pixelsBinning);

  std::vector<double> beamPipeBoundaries = {0., 20., 100.};
  auto beamPipe = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "beam_pipe", Acts::Transform3::Identity(), Acts::VolumeBounds::eOther,
      beamPipeBoundaries);

  std::vector<double> gapBoundaries = {20., 50., 10.};
  auto gap0 = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "gap0", Acts::Transform3::Identity() * Acts::Translation3(0., 0., -90.),
      Acts::VolumeBounds::eCylinder, gapBoundaries);

  std::vector<double> layerBoundaries = {20., 50., 80.};
  auto layer = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "layer", Acts::Transform3::Identity(), Acts::VolumeBounds::eCylinder,
      layerBoundaries,
      std::make_shared<Acts::Experimental::IInternalStructureBuilder>());

  auto gap1 = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "gap1", Acts::Transform3::Identity() * Acts::Translation3(0., 0., 90.),
      Acts::VolumeBounds::eCylinder, gapBoundaries);

  // Add the nodes in a random fashion
  pixels->add(std::move(gap1));
  pixels->add(std::move(gap0));
  pixels->add(std::move(layer));
  // Add pixels and beam pipe in reverse order
  detector->add(std::move(pixels));
  detector->add(std::move(beamPipe));

  std::ofstream fs("detector_unordered.dot");
  Acts::Experimental::detail::BlueprintDrawer::dotStream(fs, *detector);
  fs.close();

  // Sort the detector
  Acts::Experimental::detail::BlueprintHelper::sort(*detector);

  // Test the recursive sort worked
  BOOST_CHECK_EQUAL(detector->children.front()->name, "beam_pipe");
  BOOST_CHECK_EQUAL(detector->children.back()->name, "pixels");
  BOOST_CHECK_EQUAL(detector->children.back()->children.front()->name, "gap0");
  BOOST_CHECK_EQUAL(detector->children.back()->children[1u]->name, "layer");
  BOOST_CHECK_EQUAL(detector->children.back()->children.back()->name, "gap1");

  std::ofstream fs2("detector_ordered.dot");
  Acts::Experimental::detail::BlueprintDrawer::dotStream(fs2, *detector);
  fs2.close();
}

BOOST_AUTO_TEST_CASE(BlueprintCylindricalGapFilling) {
  // Detector dimensions
  double detectorIr = 0.;
  double detectorOr = 120.;
  double detectorHz = 400.;

  // Beam pipe
  double beamPipeOr = 20.;

  // Pixel system
  double pixelIr = 25;
  double pixelOr = 115;
  double pixelEcHz = 50;

  auto innerBuilder =
      std::make_shared<Acts::Experimental::IInternalStructureBuilder>();

  // Create  root node
  std::vector<Acts::AxisDirection> detectorBinning = {
      Acts::AxisDirection::AxisR};
  std::vector<double> detectorBoundaries = {detectorIr, detectorOr, detectorHz};

  // The root node - detector
  auto detector = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "detector", Acts::Transform3::Identity(), Acts::VolumeBounds::eCylinder,
      detectorBoundaries, detectorBinning);

  // The beam pipe
  std::vector<double> beamPipeBoundaries = {detectorIr, beamPipeOr, detectorHz};
  auto beamPipe = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "beam_pipe", Acts::Transform3::Identity(), Acts::VolumeBounds::eCylinder,
      beamPipeBoundaries, innerBuilder);
  detector->add(std::move(beamPipe));

  // A pixel system
  std::vector<double> pixelBoundaries = {pixelIr, pixelOr, detectorHz};
  std::vector<Acts::AxisDirection> pixelBinning = {Acts::AxisDirection::AxisZ};
  auto pixel = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "pixel", Acts::Transform3::Identity(), Acts::VolumeBounds::eCylinder,
      pixelBoundaries, pixelBinning);

  // Nec: Small differences to check if the adjustments are made
  std::vector<double> pixelEcBoundaries = {pixelIr, pixelOr - 5., pixelEcHz};
  std::vector<Acts::AxisDirection> pixelEcBinning = {
      Acts::AxisDirection::AxisZ};

  auto pixelNec = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "pixelNec",
      Acts::Transform3::Identity() *
          Acts::Translation3(0., 0., -detectorHz + pixelEcHz),
      Acts::VolumeBounds::eCylinder, pixelEcBoundaries, pixelEcBinning);

  // Add a single encap layer
  std::vector<double> pixelNecBoundaries = {pixelIr + 2, pixelOr - 7., 10.};
  auto pixelNecLayer =
      std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
          "pixelNecLayer",
          Acts::Transform3::Identity() *
              Acts::Translation3(0., 0., -detectorHz + pixelEcHz),
          Acts::VolumeBounds::eCylinder, pixelNecBoundaries, innerBuilder);

  pixelNec->add(std::move(pixelNecLayer));

  // Barrel
  std::vector<double> pixelBarrelBoundaries = {pixelIr + 1, pixelOr - 1.,
                                               detectorHz - 2 * pixelEcHz};
  std::vector<Acts::AxisDirection> pixelBarrelBinning = {
      Acts::AxisDirection::AxisR};

  auto pixelBarrel = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "pixelBarrel", Acts::Transform3::Identity(),
      Acts::VolumeBounds::eCylinder, pixelBarrelBoundaries, pixelBarrelBinning);

  std::vector<double> pixelBarrelL0Boundaries = {60, 65.,
                                                 detectorHz - 2 * pixelEcHz};
  auto pixelBarrelL0 =
      std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
          "pixelBarrelL0", Acts::Transform3::Identity(),
          Acts::VolumeBounds::eCylinder, pixelBarrelL0Boundaries, innerBuilder);

  std::vector<double> pixelBarrelL1Boundaries = {100, 105.,
                                                 detectorHz - 2 * pixelEcHz};
  auto pixelBarrelL1 =
      std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
          "pixelBarrelL1", Acts::Transform3::Identity(),
          Acts::VolumeBounds::eCylinder, pixelBarrelL1Boundaries, innerBuilder);
  pixelBarrel->add(std::move(pixelBarrelL0));
  pixelBarrel->add(std::move(pixelBarrelL1));

  auto pixelPec = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "pixelPec",
      Acts::Transform3::Identity() *
          Acts::Translation3(0., 0., +detectorHz - pixelEcHz),
      Acts::VolumeBounds::eCylinder, pixelEcBoundaries, pixelEcBinning);

  std::vector<double> pixelPecBoundaries = {pixelIr + 2, pixelOr - 7., 10.};
  auto pixelPecLayer =
      std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
          "pixelPecLayer",
          Acts::Transform3::Identity() *
              Acts::Translation3(0., 0., detectorHz - pixelEcHz),
          Acts::VolumeBounds::eCylinder, pixelPecBoundaries, innerBuilder);

  pixelPec->add(std::move(pixelPecLayer));

  // Adding pixel
  pixel->add(std::move(pixelNec));
  pixel->add(std::move(pixelPec));
  pixel->add(std::move(pixelBarrel));

  detector->add(std::move(pixel));

  std::ofstream fs("detector_with_gaps.dot");
  Acts::Experimental::detail::BlueprintDrawer::dotStream(fs, *detector);
  fs.close();

  // Simple test
  BOOST_CHECK_EQUAL(detector->children.size(), 2u);
  BOOST_CHECK_EQUAL(detector->children[0u]->name, "beam_pipe");
  BOOST_CHECK_EQUAL(detector->children[1u]->name, "pixel");

  // Now fill the gaps
  Acts::Experimental::detail::BlueprintHelper::fillGaps(*detector);

  // Do the tests again
  BOOST_CHECK_EQUAL(detector->children.size(), 4u);
  BOOST_CHECK_EQUAL(detector->children[0u]->name, "beam_pipe");
  BOOST_CHECK_EQUAL(detector->children[1u]->name, "detector_gap_0");
  BOOST_CHECK_EQUAL(detector->children[2u]->name, "pixel");
  BOOST_CHECK_EQUAL(detector->children[3u]->name, "detector_gap_1");

  // Adjustment of gap parameters
  BOOST_CHECK_EQUAL(detector->children[1u]->boundaryValues[0], beamPipeOr);
  BOOST_CHECK_EQUAL(detector->children[1u]->boundaryValues[1], pixelIr);
  BOOST_CHECK_EQUAL(detector->children[1u]->boundaryValues[2], detectorHz);

  BOOST_CHECK_EQUAL(detector->children[3u]->boundaryValues[0], pixelOr);
  BOOST_CHECK_EQUAL(detector->children[3u]->boundaryValues[1], detectorOr);
  BOOST_CHECK_EQUAL(detector->children[3u]->boundaryValues[2], detectorHz);

  // Check the pixel system: Nec / Barrel / Pec
  BOOST_CHECK_EQUAL(detector->children[2u]->children.size(), 3u);
  BOOST_CHECK_EQUAL(detector->children[2u]->children[0u]->children.size(), 3u);
  BOOST_CHECK_EQUAL(detector->children[2u]->children[1u]->children.size(), 5u);
  BOOST_CHECK_EQUAL(detector->children[2u]->children[2u]->children.size(), 3u);

  // Nec test
  BOOST_CHECK_EQUAL(
      detector->children[2u]->children[0u]->children[0]->boundaryValues[0],
      pixelIr);
  BOOST_CHECK_EQUAL(
      detector->children[2u]->children[0u]->children[0]->boundaryValues[1],
      pixelOr);

  BOOST_CHECK_EQUAL(
      detector->children[2u]->children[0u]->children[1]->boundaryValues[0],
      pixelIr);
  BOOST_CHECK_EQUAL(
      detector->children[2u]->children[0u]->children[1]->boundaryValues[1],
      pixelOr);

  BOOST_CHECK_EQUAL(
      detector->children[2u]->children[0u]->children[2]->boundaryValues[0],
      pixelIr);
  BOOST_CHECK_EQUAL(
      detector->children[2u]->children[0u]->children[2]->boundaryValues[1],
      pixelOr);

  std::ofstream fs2("detector_without_gaps.dot");
  Acts::Experimental::detail::BlueprintDrawer::dotStream(fs2, *detector);
  fs2.close();
}

BOOST_AUTO_TEST_CASE(BlueprintCylindricalGapException) {
  auto innerBuilder =
      std::make_shared<Acts::Experimental::IInternalStructureBuilder>();

  // The root node - detector
  std::vector<double> detectorBoundaries = {0., 50., 100.};
  std::vector<Acts::AxisDirection> detectorBinning = {
      Acts::AxisDirection::AxisX};
  auto detector = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "detector", Acts::Transform3::Identity(), Acts::VolumeBounds::eCylinder,
      detectorBoundaries, detectorBinning);

  // Add a volume
  std::vector<double> volTwoBoundaries = {0., 20., 100.};
  auto vol = std::make_unique<Acts::Experimental::Gen2Blueprint::Node>(
      "vol", Acts::Transform3::Identity(), Acts::VolumeBounds::eCylinder,
      volTwoBoundaries, innerBuilder);
  detector->add(std::move(vol));

  // Throw because cylinders can not be binned in x
  BOOST_CHECK_THROW(
      Acts::Experimental::detail::BlueprintHelper::fillGaps(*detector),
      std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
