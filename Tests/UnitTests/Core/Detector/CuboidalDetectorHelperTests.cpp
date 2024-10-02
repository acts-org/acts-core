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
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/detail/CuboidalDetectorHelper.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/StringHelpers.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"

#include <array>
#include <memory>
#include <stdexcept>

auto portalGenerator = Acts::Experimental::defaultPortalGenerator();
auto tContext = Acts::GeometryContext();

BOOST_AUTO_TEST_SUITE(Experimental)

BOOST_AUTO_TEST_CASE(CubicVolumeExceptions) {
  // A perfect box shape
  auto box = std::make_shared<Acts::CuboidVolumeBounds>(10, 10, 10);

  // Create volume A
  auto volumeA = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "VolumeA", Acts::Transform3::Identity(), box,
      Acts::Experimental::tryAllPortals());

  // Create volume B
  auto transformB = Acts::Transform3::Identity();
  transformB.prerotate(Acts::AngleAxis3(0.234, Acts::Vector3::UnitZ()));

  auto volumeB = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "volumeB", transformB, box,
      Acts::Experimental::tryAllPortals());
  // Build the container
  std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> volumes = {
      volumeA, volumeB};

  BOOST_CHECK_THROW(Acts::Experimental::detail::CuboidalDetectorHelper::connect(
                        tContext, volumes, Acts::BinningValue::binX, {},
                        Acts::Logging::VERBOSE),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(SimpleBoxConnection) {
  std::array<Acts::BinningValue, 3> binningValues = {Acts::BinningValue::binX,
                                                     Acts::BinningValue::binY,
                                                     Acts::BinningValue::binZ};
  for (auto bVal : binningValues) {
    // A perfect box shape
    auto box = std::make_shared<Acts::CuboidVolumeBounds>(10, 10, 10);

    // Create volume A
    auto volumeA = Acts::Experimental::DetectorVolumeFactory::construct(
        portalGenerator, tContext, "VolumeA", Acts::Transform3::Identity(), box,
        Acts::Experimental::tryAllPortals());

    // Move it into the bval direction
    auto transformB = Acts::Transform3::Identity();

    Acts::Vector3 translation = Acts::Vector3::Zero();
    translation[toUnderlying(bVal)] = 20;
    transformB.pretranslate(translation);
    // Create volume B
    auto volumeB = Acts::Experimental::DetectorVolumeFactory::construct(
        portalGenerator, tContext, "VolumeB", transformB, box,
        Acts::Experimental::tryAllPortals());
    // Build the container
    std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> volumes = {
        volumeA, volumeB};
    auto container =
        Acts::Experimental::detail::CuboidalDetectorHelper::connect(
            tContext, volumes, bVal, {}, Acts::Logging::VERBOSE);

    // Check the container is constructed
    BOOST_CHECK(!container.empty());

    Acts::ObjVisualization3D obj;
    Acts::GeometryView3D::drawDetectorVolume(obj, *volumeA, tContext);
    Acts::GeometryView3D::drawDetectorVolume(obj, *volumeB, tContext);
    obj.write("ConnectectBoxesRegular_" + Acts::binningValueName(bVal) +
              ".obj");
  }
}

BOOST_AUTO_TEST_CASE(IrregularBoxConnectionInZ) {
  std::vector<Acts::BinningValue> binningValues = {Acts::BinningValue::binX,
                                                   Acts::BinningValue::binY,
                                                   Acts::BinningValue::binZ};

  using HlPos = std::array<Acts::ActsScalar, 2u>;
  using VolHlPos = std::array<HlPos, 3u>;
  using VolSetup = std::array<VolHlPos, 3u>;

  VolHlPos cPA = {{{10., 0.}, {10., 0.}, {10., 0.}}};
  VolHlPos cPB = {{{20., 0.}, {20., 0.}, {20., 0.}}};
  VolHlPos sP = {{{10., -30.}, {30., 10.}, {90., 130.}}};

  std::array<VolSetup, 3u> volSetups = {
      {{sP, cPA, cPB}, {cPB, sP, cPA}, {cPA, cPB, sP}}};

  std::array<Acts::Transform3, 2u> transforms = {
      Acts::Transform3::Identity(),
      Acts::Transform3{Acts::Transform3::Identity()}.prerotate(
          Acts::AngleAxis3(0.34, Acts::Vector3(1., 1., 1.).normalized()))};

  // Try with arbitrary rotations
  for (auto [it, t] : Acts::enumerate(transforms)) {
    std::string trstr = it == 0 ? "" : "_rotated";
    auto rotation = t.rotation();
    // Try for all binning values
    for (auto bVal : binningValues) {
      auto [vsA, vsB, vsC] = volSetups[toUnderlying(bVal)];

      // Three box shares with different length in Z
      auto boxA = std::make_shared<Acts::CuboidVolumeBounds>(
          vsA[0][0], vsB[0][0], vsC[0][0]);
      auto boxB = std::make_shared<Acts::CuboidVolumeBounds>(
          vsA[1][0], vsB[1][0], vsC[1][0]);
      auto boxC = std::make_shared<Acts::CuboidVolumeBounds>(
          vsA[2][0], vsB[2][0], vsC[2][0]);

      auto transformA = Acts::Transform3::Identity();
      auto transformB = Acts::Transform3::Identity();
      auto transformC = Acts::Transform3::Identity();

      transformA.pretranslate(Acts::Vector3(vsA[0][1], vsB[0][1], vsC[0][1]));
      transformB.pretranslate(Acts::Vector3(vsA[1][1], vsB[1][1], vsC[1][1]));
      transformC.pretranslate(Acts::Vector3(vsA[2][1], vsB[2][1], vsC[2][1]));

      transformA.prerotate(rotation);
      transformB.prerotate(rotation);
      transformC.prerotate(rotation);

      // Create volume A, B, C
      auto volumeA = Acts::Experimental::DetectorVolumeFactory::construct(
          portalGenerator, tContext, "VolumeA", transformA, boxA,
          Acts::Experimental::tryAllPortals());
      auto volumeB = Acts::Experimental::DetectorVolumeFactory::construct(
          portalGenerator, tContext, "VolumeB", transformB, boxB,
          Acts::Experimental::tryAllPortals());
      auto volumeC = Acts::Experimental::DetectorVolumeFactory::construct(
          portalGenerator, tContext, "VolumeC", transformC, boxC,
          Acts::Experimental::tryAllPortals());

      // Build the container
      std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> volumes =
          {volumeA, volumeB, volumeC};
      auto container =
          Acts::Experimental::detail::CuboidalDetectorHelper::connect(
              tContext, volumes, bVal, {}, Acts::Logging::VERBOSE);

      // Check the container is constructed
      BOOST_CHECK(!container.empty());

      Acts::ObjVisualization3D obj;
      Acts::GeometryView3D::drawDetectorVolume(obj, *volumeA, tContext);
      Acts::GeometryView3D::drawDetectorVolume(obj, *volumeB, tContext);
      Acts::GeometryView3D::drawDetectorVolume(obj, *volumeC, tContext);
      obj.write("ConnectectBoxesIrregular_" + Acts::binningValueName(bVal) +
                trstr + ".obj");
    }
  }
}

BOOST_AUTO_TEST_CASE(ContainerConnection) {
  // A perfect box shape
  auto box = std::make_shared<Acts::CuboidVolumeBounds>(10, 10, 10);

  // Create an AB container

  // Create volume A
  auto volumeA = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "VolumeA", Acts::Transform3::Identity(), box,
      Acts::Experimental::tryAllPortals());

  // Move it into the bval direction
  auto transformB = Acts::Transform3::Identity();
  Acts::Vector3 translationB = Acts::Vector3::Zero();
  translationB[toUnderlying(Acts::BinningValue::binX)] = 20;
  transformB.pretranslate(translationB);
  // Create volume B
  auto volumeB = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "VolumeB", transformB, box,
      Acts::Experimental::tryAllPortals());
  // Build the container
  std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> volumes = {
      volumeA, volumeB};
  auto containerAB =
      Acts::Experimental::detail::CuboidalDetectorHelper::connect(
          tContext, volumes, Acts::BinningValue::binX, {},
          Acts::Logging::VERBOSE);

  // Create a CD container

  auto transformC = Acts::Transform3::Identity();
  Acts::Vector3 translationC = Acts::Vector3::Zero();
  translationC[toUnderlying(Acts::BinningValue::binY)] = 20;
  transformC.pretranslate(translationC);

  auto volumeC = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "VolumeC", transformC, box,
      Acts::Experimental::tryAllPortals());

  auto transformD = Acts::Transform3::Identity();
  Acts::Vector3 translationD = Acts::Vector3::Zero();
  translationD[toUnderlying(Acts::BinningValue::binX)] = 20;
  translationD[toUnderlying(Acts::BinningValue::binY)] = 20;
  transformD.pretranslate(translationD);

  auto volumeD = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "VolumeD", transformD, box,
      Acts::Experimental::tryAllPortals());

  volumes = {volumeC, volumeD};
  auto containerCD =
      Acts::Experimental::detail::CuboidalDetectorHelper::connect(
          tContext, volumes, Acts::BinningValue::binX, {},
          Acts::Logging::VERBOSE);

  auto containerABCD =
      Acts::Experimental::detail::CuboidalDetectorHelper::connect(
          tContext, {containerAB, containerCD}, Acts::BinningValue::binY, {},
          Acts::Logging::VERBOSE);

  // Check the container is constructed
  BOOST_CHECK(!containerABCD.empty());

  Acts::ObjVisualization3D obj;
  Acts::GeometryView3D::drawDetectorVolume(obj, *volumeA, tContext);
  Acts::GeometryView3D::drawDetectorVolume(obj, *volumeB, tContext);
  Acts::GeometryView3D::drawDetectorVolume(obj, *volumeC, tContext);
  Acts::GeometryView3D::drawDetectorVolume(obj, *volumeD, tContext);

  obj.write("ConnectContainers_binX.obj");
}

BOOST_AUTO_TEST_SUITE_END()
