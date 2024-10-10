// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/IndexedRootVolumeFinderBuilder.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Navigation/PortalNavigation.hpp"
#include "Acts/Utilities/Logger.hpp"

using namespace Acts;
using namespace Acts::Experimental;

GeometryContext tContext;
Logging::Level logLevel = Logging::VERBOSE;

BOOST_AUTO_TEST_SUITE(DetectorTests)

BOOST_AUTO_TEST_CASE(IndexedRootVolumeFinderBuilderCylindrical) {
  auto portalGenerator = defaultPortalGenerator();

  auto innerB = std::make_unique<CylinderVolumeBounds>(0., 20., 100);
  auto innerV = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Inner", Transform3::Identity(),
      std::move(innerB), tryAllPortals());

  auto middleLB = std::make_unique<CylinderVolumeBounds>(20., 60., 5);
  auto middleLT = Transform3::Identity();
  middleLT.pretranslate(Vector3(0., 0., -95));
  auto middleLV = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "MiddleLeft", middleLT, std::move(middleLB),
      tryAllPortals());

  auto middleDB = std::make_unique<CylinderVolumeBounds>(20., 40., 90);
  auto middleDV = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "MiddleDown", Transform3::Identity(),
      std::move(middleDB), tryAllPortals());

  auto middleUB = std::make_unique<CylinderVolumeBounds>(40., 60., 90);
  auto middleUV = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "MiddleUp", Transform3::Identity(),
      std::move(middleUB), tryAllPortals());

  auto middleRB = std::make_unique<CylinderVolumeBounds>(20., 60., 5);
  auto middleRT = Transform3::Identity();
  middleRT.pretranslate(Vector3(0., 0., 95));
  auto middleRV = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "middleRight", middleRT, std::move(middleRB),
      tryAllPortals());

  auto outerB = std::make_unique<CylinderVolumeBounds>(60., 120., 100);
  auto outerV = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Outer", Transform3::Identity(),
      std::move(outerB), tryAllPortals());

  std::vector<std::shared_ptr<DetectorVolume>> rootVolumes = {
      innerV, middleLV, middleDV, middleUV, middleRV, outerV};

  IndexedRootVolumeFinderBuilder builder(
      {Acts::BinningValue::binZ, Acts::BinningValue::binR});

  // Let's construct a detector
  auto rootVolumeFinder = builder.construct(tContext, rootVolumes);

  Acts::Experimental::GeometryIdGenerator::Config generatorConfig;
  Acts::Experimental::GeometryIdGenerator generator(
      generatorConfig,
      Acts::getDefaultLogger("SequentialIdGenerator", Acts::Logging::VERBOSE));
  auto cache = generator.generateCache();
  for (auto& vol : rootVolumes) {
    generator.assignGeometryId(cache, *vol);
  }

  auto detectorIndexed = Detector::makeShared("IndexedDetector", rootVolumes,
                                              std::move(rootVolumeFinder));

  BOOST_CHECK_EQUAL(
      detectorIndexed->findDetectorVolume(tContext, {10., 0., 0.}),
      innerV.get());
  BOOST_CHECK_EQUAL(
      detectorIndexed->findDetectorVolume(tContext, {25., 0., -93.}),
      middleLV.get());
  BOOST_CHECK_EQUAL(
      detectorIndexed->findDetectorVolume(tContext, {35., 0., 0.}),
      middleDV.get());
  BOOST_CHECK_EQUAL(
      detectorIndexed->findDetectorVolume(tContext, {55., 0., 0.}),
      middleUV.get());
  BOOST_CHECK_EQUAL(
      detectorIndexed->findDetectorVolume(tContext, {40., 0., 92.}),
      middleRV.get());
  BOOST_CHECK_EQUAL(
      detectorIndexed->findDetectorVolume(tContext, {65., 0., 0.}),
      outerV.get());
}

BOOST_AUTO_TEST_SUITE_END()
