// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/detail/CylindricalDetectorHelper.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <map>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Acts::Experimental {
class Portal;
}  // namespace Acts::Experimental

using namespace Acts;
using namespace Experimental;
using namespace Experimental::detail;
using namespace Experimental::detail::CylindricalDetectorHelper;

Logging::Level logLevel = Logging::VERBOSE;

GeometryContext tContext;
std::vector<std::shared_ptr<DetectorVolume>> eVolumes = {};

auto portalGenerator = defaultPortalGenerator();

BOOST_AUTO_TEST_SUITE(Experimental)

BOOST_AUTO_TEST_CASE(ConnectVolumeExceptions) {
  ACTS_LOCAL_LOGGER(getDefaultLogger("Faulty setups", logLevel));

  auto cBounds0 = std::make_unique<CylinderVolumeBounds>(0., 100., 100);
  auto volume0 = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume0", Transform3::Identity(),
      std::move(cBounds0), tryAllPortals());

  auto cBounds1 =
      std::make_unique<CylinderVolumeBounds>(0., 100., 100, 0.2, 1.);
  auto volume1 = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume0", Transform3::Identity(),
      std::move(cBounds1), tryAllPortals());

  ACTS_INFO("*** Test: nullptr in the list of volumes");

  // Invalid arguments: nullptr
  std::vector<std::shared_ptr<DetectorVolume>> volumesWithNullptr = {
      volume0, nullptr, volume1};
  BOOST_CHECK_THROW(connectInR(tContext, volumesWithNullptr, {}, logLevel),
                    std::invalid_argument);

  ACTS_INFO("*** Test: non-cylinder in the list of volumes");

  auto cubeBounds = std::make_unique<CuboidVolumeBounds>(100., 100., 100);
  auto cube = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Cube", Transform3::Identity(),
      std::move(cubeBounds), tryAllPortals());

  // Invalid arguments: cube
  std::vector<std::shared_ptr<DetectorVolume>> volumesWithCube = {
      volume0, volume1, cube};
  BOOST_CHECK_THROW(connectInR(tContext, volumesWithCube, {}, logLevel),
                    std::invalid_argument);

  ACTS_INFO("*** Test: non-aligned volume in the list of volumes");
  Transform3 rotated = Transform3::Identity();
  AngleAxis3 rotX(0.1234, Vector3::UnitX());
  rotated *= rotX;

  auto cBounds2 = std::make_unique<CylinderVolumeBounds>(0., 100., 100);
  auto volume2 = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume2", rotated, std::move(cBounds2),
      tryAllPortals());

  // Invalid arguments: non-aligned
  std::vector<std::shared_ptr<DetectorVolume>> volumesWithNonaligned = {
      volume0, volume1, volume2};
  BOOST_CHECK_THROW(connectInR(tContext, volumesWithNonaligned, {}, logLevel),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(ConnectInR) {
  ACTS_LOCAL_LOGGER(getDefaultLogger("Connect: R", logLevel));
  ACTS_INFO("*** Test: connect DetectorVolumes in R, create proto container");
  // Test with different opening angles
  std::vector<ActsScalar> testOpenings = {M_PI, 0.5 * M_PI};

  std::vector<ActsScalar> radii = {0., 10., 100., 200.};
  ActsScalar halfZ = 100.;

  // This should work for full cylinder and sector openings
  for (auto [io, opening] : enumerate(testOpenings)) {
    ACTS_INFO("    -> test  with phi opening: " << opening);
    std::string opStr = "opening_" + std::to_string(io);
    std::vector<std::shared_ptr<DetectorVolume>> rVolumes = {};
    // Create the voluems
    for (auto [i, r] : enumerate(radii)) {
      if (i > 0) {
        auto cBounds = std::make_unique<CylinderVolumeBounds>(
            radii[i - 1u], r, halfZ, opening, 0.);
        rVolumes.push_back(DetectorVolumeFactory::construct(
            portalGenerator, tContext, "Cylinder_r" + std::to_string(i),
            Transform3::Identity(), std::move(cBounds), tryAllPortals()));
      }
    }

    auto protoContainer = connectInR(tContext, rVolumes, {}, logLevel);
    // Check the portal setup
    BOOST_CHECK_EQUAL(rVolumes[0u]->portalPtrs()[2u],
                      rVolumes[1u]->portalPtrs()[3u]);
    BOOST_CHECK_EQUAL(rVolumes[1u]->portalPtrs()[2u],
                      rVolumes[2u]->portalPtrs()[3u]);
    BOOST_CHECK_EQUAL(rVolumes[0u]->portalPtrs()[0u],
                      rVolumes[1u]->portalPtrs()[0u]);
    BOOST_CHECK_EQUAL(rVolumes[1u]->portalPtrs()[0u],
                      rVolumes[2u]->portalPtrs()[0u]);
    BOOST_CHECK_EQUAL(rVolumes[0u]->portalPtrs()[1u],
                      rVolumes[1u]->portalPtrs()[1u]);
    BOOST_CHECK_EQUAL(rVolumes[1u]->portalPtrs()[1u],
                      rVolumes[2u]->portalPtrs()[1u]);
    BOOST_CHECK_EQUAL(rVolumes[0u]->portalPtrs()[0u], protoContainer[0u]);
    BOOST_CHECK_EQUAL(rVolumes[0u]->portalPtrs()[1u], protoContainer[1u]);

    // Assign geometry ids to the volumes
    Acts::Experimental::GeometryIdGenerator::Config generatorConfig;
    GeometryIdGenerator generator(
        generatorConfig, Acts::getDefaultLogger("SequentialIdGenerator",
                                                Acts::Logging::VERBOSE));
    auto cache = generator.generateCache();
    for (auto& vol : rVolumes) {
      generator.assignGeometryId(cache, *vol);
    }

    // A detector construction that should work
    auto detector =
        Detector::makeShared("DetectorInR", rVolumes, tryRootVolumes());

    // Make a rzphi grid
    const auto& volumes = detector->volumes();
    auto boundaries = rzphiBoundaries(tContext, volumes);
    const auto& rBoundaries = boundaries[0u];
    const auto& zBoundaries = boundaries[1u];

    // Check the radii
    std::vector<ActsScalar> zvalues = {-halfZ, halfZ};
    BOOST_CHECK(radii == rBoundaries);
    BOOST_CHECK(zvalues == zBoundaries);
  }

  // Invalid arguments
  ACTS_INFO("*** Test: faulty empty vector");
  BOOST_CHECK_THROW(connectInR(tContext, eVolumes, {}, logLevel),
                    std::invalid_argument);

  // Faulty setups, not matchint in R
  ACTS_INFO("*** Test: volumes are not matching in R");

  auto cBounds00 = std::make_unique<CylinderVolumeBounds>(0., 100., 100);
  auto volume00 = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume00", Transform3::Identity(),
      std::move(cBounds00), tryAllPortals());

  auto cBounds01 = std::make_unique<CylinderVolumeBounds>(101., 200., 100);
  auto volume01 = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume01", Transform3::Identity(),
      std::move(cBounds01), tryAllPortals());

  std::vector<std::shared_ptr<DetectorVolume>> volumesNotMatching = {volume00,
                                                                     volume01};
  BOOST_CHECK_THROW(connectInR(tContext, volumesNotMatching, {}, logLevel),
                    std::runtime_error);

  ACTS_INFO("*** Test: volume bounds are not aligned");
  Transform3 shifted = Transform3::Identity();
  shifted.pretranslate(Vector3(0., 0., 10.));

  auto cBounds10 = std::make_unique<CylinderVolumeBounds>(0., 100., 100);
  auto volume10 = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume10", shifted, std::move(cBounds10),
      tryAllPortals());

  auto cBounds11 = std::make_unique<CylinderVolumeBounds>(100., 200., 90);
  auto volume11 = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume11", shifted, std::move(cBounds11),
      tryAllPortals());

  std::vector<std::shared_ptr<DetectorVolume>> volumesNotAligned = {volume10,
                                                                    volume11};
  BOOST_CHECK_THROW(connectInR(tContext, volumesNotAligned, {}, logLevel),
                    std::runtime_error);
}

BOOST_AUTO_TEST_CASE(ConnectInZ) {
  ACTS_LOCAL_LOGGER(getDefaultLogger("Connect: Z", logLevel));
  ACTS_INFO("*** Test: connect DetectorVolumes in Z, create proto container");

  // @TODO: test with different transforms, this should work in, not used yet
  std::vector<Transform3> transforms = {Transform3::Identity()};
  std::vector<std::array<ActsScalar, 2>> radii = {{0., 100.}, {20., 120.}};
  std::vector<ActsScalar> zValues = {-100., -20, 10., 100., 200.};

  for (auto [it, t] : enumerate(transforms)) {
    ACTS_INFO("    -> test series with transform id " << it);

    std::string trfStr = "_transform_" + std::to_string(it);
    for (auto [ir, r] : enumerate(radii)) {
      ACTS_INFO("        -> test series with radii setup "
                << radii[ir][0u] << ", " << radii[ir][1u]);

      std::string radStr = "_radii_" + std::to_string(ir);
      std::vector<std::shared_ptr<DetectorVolume>> zVolumes = {};
      for (auto [i, z] : enumerate(zValues)) {
        if (i > 0) {
          auto cBounds = std::make_unique<CylinderVolumeBounds>(
              r[0], r[1], 0.5 * (z - zValues[i - 1u]));
          // z center
          ActsScalar zCenter = 0.5 * (z + zValues[i - 1u]);
          Transform3 ti = Transform3::Identity();
          ti.pretranslate(t.translation() +
                          zCenter * t.rotation().matrix().col(2));
          ti.prerotate(t.rotation());
          // create the volume
          zVolumes.push_back(DetectorVolumeFactory::construct(
              portalGenerator, tContext,
              "Cylinder_z" + std::to_string(i) + trfStr + radStr, ti,
              std::move(cBounds), tryAllPortals()));
        }
      }
      // Now call the connector
      auto protoContainer = connectInZ(tContext, zVolumes, {}, logLevel);

      // Check the portal setup.
      // Glued, remainders are outside skin
      BOOST_CHECK_EQUAL(zVolumes[0u]->portalPtrs()[1u],
                        zVolumes[1u]->portalPtrs()[0u]);
      BOOST_CHECK_EQUAL(zVolumes[1u]->portalPtrs()[1u],
                        zVolumes[2u]->portalPtrs()[0u]);
      BOOST_CHECK_EQUAL(zVolumes[2u]->portalPtrs()[1u],
                        zVolumes[3u]->portalPtrs()[0u]);
      BOOST_CHECK_EQUAL(protoContainer[0u], zVolumes[0u]->portalPtrs()[0u]);
      BOOST_CHECK_EQUAL(protoContainer[1u], zVolumes[3u]->portalPtrs()[1u]);

      // Covered with the same surface, shich is the outside skin
      std::vector<unsigned int> checkShared = {2u};
      if (radii[ir][0u] > 0.) {
        checkShared.push_back(3u);
      }

      for (const auto& ip : checkShared) {
        BOOST_CHECK_EQUAL(zVolumes[0u]->portalPtrs()[ip],
                          zVolumes[1u]->portalPtrs()[ip]);
        BOOST_CHECK_EQUAL(zVolumes[1u]->portalPtrs()[ip],
                          zVolumes[2u]->portalPtrs()[ip]);
        BOOST_CHECK_EQUAL(zVolumes[2u]->portalPtrs()[ip],
                          zVolumes[3u]->portalPtrs()[ip]);
        BOOST_CHECK_EQUAL(protoContainer[ip], zVolumes[0u]->portalPtrs()[ip]);
      }

      // Assign geometry ids to the volumes
      Acts::Experimental::GeometryIdGenerator::Config generatorConfig;
      GeometryIdGenerator generator(
          generatorConfig, Acts::getDefaultLogger("SequentialIdGenerator",
                                                  Acts::Logging::VERBOSE));
      auto cache = generator.generateCache();
      for (auto& vol : zVolumes) {
        generator.assignGeometryId(cache, *vol);
      }

      auto detector =
          Detector::makeShared("DetectorInZ", zVolumes, tryRootVolumes());
    }
  }

  // Invalid arguments
  BOOST_CHECK_THROW(connectInZ(tContext, eVolumes, {}, logLevel),
                    std::invalid_argument);

  // Volumes have different radii - other bounds will be the same
  auto cBounds00 = std::make_unique<CylinderVolumeBounds>(0., 100., 100);
  auto volume00 = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume00",
      Transform3::Identity() * Translation3(0., 0., -100.),
      std::move(cBounds00), tryAllPortals());

  auto cBounds01 = std::make_unique<CylinderVolumeBounds>(0., 105., 100);
  auto volume01 = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume01",
      Transform3::Identity() * Translation3(0., 0., 100.), std::move(cBounds01),
      tryAllPortals());

  std::vector<std::shared_ptr<DetectorVolume>> volumesNonalignedBounds = {
      volume00, volume01};
  BOOST_CHECK_THROW(connectInZ(tContext, volumesNonalignedBounds, {}, logLevel),
                    std::runtime_error);

  // Volumes are not attached
  auto cBounds10 = std::make_unique<CylinderVolumeBounds>(0., 100., 100);
  auto volume10 = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume00",
      Transform3::Identity() * Translation3(0., 0., -105.),
      std::move(cBounds10), tryAllPortals());

  auto cBounds11 = std::make_unique<CylinderVolumeBounds>(0., 100., 100);
  auto volume11 = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Volume01",
      Transform3::Identity() * Translation3(0., 0., 100.), std::move(cBounds11),
      tryAllPortals());

  std::vector<std::shared_ptr<DetectorVolume>> volumesNotAttached = {volume10,
                                                                     volume11};
  BOOST_CHECK_THROW(connectInZ(tContext, volumesNotAttached, {}, logLevel),
                    std::runtime_error);
}

BOOST_AUTO_TEST_CASE(ConnectInPhi) {
  ACTS_LOCAL_LOGGER(getDefaultLogger("Connect: Phi", logLevel));
  ACTS_INFO("*** Test: connect DetectorVolumes in Phi, create proto container");

  std::vector<Transform3> transforms = {Transform3::Identity()};
  unsigned int phiSectors = 5;
  ActsScalar phiHalfSector = M_PI / phiSectors;

  for (auto [it, t] : enumerate(transforms)) {
    ACTS_INFO("    -> test series with transform id " << it);

    std::vector<std::shared_ptr<DetectorVolume>> phiVolumes = {};
    for (unsigned int i = 0; i < phiSectors; ++i) {
      auto cBounds = std::make_unique<CylinderVolumeBounds>(
          10., 100., 100., phiHalfSector,
          -M_PI + (2u * i + 1u) * phiHalfSector);

      // create the volume
      phiVolumes.push_back(DetectorVolumeFactory::construct(
          portalGenerator, tContext, "Cylinder_phi" + std::to_string(i), t,
          std::move(cBounds), tryAllPortals()));
    }

    auto protoContainer = connectInPhi(tContext, phiVolumes, {}, logLevel);

    // All phiVolumes share : inner tube, outer cover, negative & positive disc
    std::vector<unsigned int> checkShared = {0u, 1u, 2u, 3u};
    for (auto [iv, v] : enumerate(phiVolumes)) {
      if (iv > 0u) {
        auto current = v;
        auto last = phiVolumes[iv - 1u];
        for (const auto& ch : checkShared) {
          BOOST_CHECK_EQUAL(current->portalPtrs()[ch], last->portalPtrs()[ch]);
        }
      }
    }

    // Assign geometry ids to the volumes
    Acts::Experimental::GeometryIdGenerator::Config generatorConfig;
    GeometryIdGenerator generator(
        generatorConfig, Acts::getDefaultLogger("SequentialIdGenerator",
                                                Acts::Logging::VERBOSE));
    auto cache = generator.generateCache();
    for (auto& vol : phiVolumes) {
      generator.assignGeometryId(cache, *vol);
    }

    auto detector =
        Detector::makeShared("DetectorInPhi", phiVolumes, tryRootVolumes());
  }

  // Invalid arguments
  BOOST_CHECK_THROW(connectInPhi(tContext, eVolumes, {}, logLevel),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(WrapVolumeinRZ) {
  ACTS_LOCAL_LOGGER(getDefaultLogger("Wrap: Z-R", logLevel));
  ACTS_INFO(
      "*** Test: wrap volume in Z-R with CutoutCylinderVolume, create proto "
      "container");

  // @TODO: test with different transforms, this should work in, not used yet
  std::vector<Transform3> transforms = {Transform3::Identity()};

  // Test with different inner radii
  std::vector<std::array<ActsScalar, 3u>> radii = {{0., 100., 500.},
                                                   {20., 120., 500.}};

  ActsScalar innerHalfZ = 150.;
  ActsScalar outerHalfZ = 175.;

  // Set up all the different tests
  for (auto [it, tf] : enumerate(transforms)) {
    ACTS_INFO("    Test series with transform id " << it);

    std::string trfStr = "_transform_" + std::to_string(it);
    for (auto [ir, r] : enumerate(radii)) {
      ACTS_INFO("    -> test series with radii setup " << radii[ir][0u] << ", "
                                                       << radii[ir][1u]);

      std::vector<std::shared_ptr<DetectorVolume>> volumes = {};

      std::string radStr = "_radii_" + std::to_string(ir);
      // Create the inner bounds
      auto iBounds = std::make_unique<CylinderVolumeBounds>(
          radii[ir][0u], radii[ir][1u], innerHalfZ);
      volumes.push_back(DetectorVolumeFactory::construct(
          portalGenerator, tContext, "InnerCylinder" + radStr + trfStr, tf,
          std::move(iBounds), tryAllPortals()));

      // Create the wrapping bounds
      auto wBounds = std::make_unique<CutoutCylinderVolumeBounds>(
          radii[ir][0u], radii[ir][1u], radii[ir][2u], outerHalfZ, innerHalfZ);

      volumes.push_back(DetectorVolumeFactory::construct(
          portalGenerator, tContext, "WrappingCylinder" + radStr + trfStr, tf,
          std::move(wBounds), tryAllPortals()));

      wrapInZR(tContext, volumes, logLevel);
    }
  }

  // Invalid arguments
  BOOST_CHECK_THROW(wrapInZR(tContext, eVolumes, logLevel),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(ProtoContainerZR) {
  ACTS_LOCAL_LOGGER(getDefaultLogger("Container: Z-R", logLevel));
  ACTS_INFO("*** Test: create a container in Z-R.");

  auto transform = Transform3::Identity();

  std::vector<ActsScalar> innerMostRadii = {0., 2.};

  for (auto [ir, imr] : enumerate(innerMostRadii)) {
    ACTS_INFO("    -> test series innermost radius setup "
              << innerMostRadii[ir]);

    // A container in R
    std::vector<ActsScalar> radii = {25., 100., 200.};
    ActsScalar halfZ = 200;

    // An innermost Pipe
    auto bBounds =
        std::make_unique<CylinderVolumeBounds>(imr, radii[0u], halfZ);

    auto innerPipe = DetectorVolumeFactory::construct(
        portalGenerator, tContext, "InnerPipe", transform, std::move(bBounds),
        tryAllPortals());

    // Make a container representation out of it
    std::map<unsigned int, std::shared_ptr<Portal>> ipContainer;
    for (auto [ip, p] : enumerate(innerPipe->portalPtrs())) {
      ipContainer[ip] = p;
    }

    // Create the r - sorted volumes
    std::vector<std::shared_ptr<DetectorVolume>> rVolumes = {};
    // Create the voluems
    for (auto [i, r] : enumerate(radii)) {
      if (i > 0) {
        auto cBounds =
            std::make_unique<CylinderVolumeBounds>(radii[i - 1u], r, halfZ);
        rVolumes.push_back(DetectorVolumeFactory::construct(
            portalGenerator, tContext, "Cylinder_r" + std::to_string(i),
            transform, std::move(cBounds), tryAllPortals()));
      }
    }

    auto protoContainerInR = connectInR(tContext, rVolumes, {}, logLevel);

    std::vector<ActsScalar> zValues = {-200., -120, 10., 100., 200.};
    std::vector<std::shared_ptr<DetectorVolume>> zVolumes = {};
    for (auto [i, z] : enumerate(zValues)) {
      if (i > 0) {
        auto cBounds = std::make_unique<CylinderVolumeBounds>(
            200., 300., 0.5 * (z - zValues[i - 1u]));
        // z center
        ActsScalar zCenter = 0.5 * (z + zValues[i - 1u]);
        Transform3 ti = transform;
        ti.pretranslate(transform.translation() +
                        zCenter * transform.rotation().matrix().col(2));

        // create the volume
        zVolumes.push_back(DetectorVolumeFactory::construct(
            portalGenerator, tContext, "Cylinder_z" + std::to_string(i), ti,
            std::move(cBounds), tryAllPortals()));
      }
    }
    // Now call the connector
    auto protoContainerInZ = connectInZ(tContext, zVolumes, {}, logLevel);
    auto centralContainer = connectInR(
        tContext, {ipContainer, protoContainerInR, protoContainerInZ}, {},
        logLevel);

    // Let's make two endcaps
    // Nec
    auto necBounds = std::make_unique<CylinderVolumeBounds>(imr, 300., 50.);

    auto necTransform = Transform3::Identity();
    necTransform.pretranslate(Vector3(0., 0., -250));
    auto necVolume = DetectorVolumeFactory::construct(
        portalGenerator, tContext, "Nec", necTransform, std::move(necBounds),
        tryAllPortals());

    std::map<unsigned int, std::shared_ptr<Portal>> necContainer;
    for (auto [ip, p] : enumerate(necVolume->portalPtrs())) {
      necContainer[ip] = p;
    }

    // Pec container
    auto pecInnerBounds =
        std::make_unique<CylinderVolumeBounds>(imr, 175., 100.);

    auto pecOuterBounds =
        std::make_unique<CylinderVolumeBounds>(175., 300., 100.);

    auto pecTransform = Transform3::Identity();
    pecTransform.pretranslate(Vector3(0., 0., 300));
    auto pecInner = DetectorVolumeFactory::construct(
        portalGenerator, tContext, "PecInner", pecTransform,
        std::move(pecInnerBounds), tryAllPortals());
    auto pecOuter = DetectorVolumeFactory::construct(
        portalGenerator, tContext, "PecOuter", pecTransform,
        std::move(pecOuterBounds), tryAllPortals());

    std::vector<std::shared_ptr<DetectorVolume>> pecVolumes = {pecInner,
                                                               pecOuter};
    auto pecContainer = connectInR(tContext, pecVolumes, {}, logLevel);

    auto overallContainer = connectInZ(
        tContext, {necContainer, centralContainer, pecContainer}, {}, logLevel);

    //  Add them together
    std::vector<std::shared_ptr<DetectorVolume>> dVolumes;
    dVolumes.push_back(innerPipe);
    dVolumes.push_back(necVolume);
    dVolumes.insert(dVolumes.end(), rVolumes.begin(), rVolumes.end());
    dVolumes.insert(dVolumes.end(), zVolumes.begin(), zVolumes.end());
    dVolumes.push_back(pecInner);
    dVolumes.push_back(pecOuter);

    // Assign geometry ids to the volumes
    Acts::Experimental::GeometryIdGenerator::Config generatorConfig;
    GeometryIdGenerator generator(
        generatorConfig, Acts::getDefaultLogger("SequentialIdGenerator",
                                                Acts::Logging::VERBOSE));
    auto cache = generator.generateCache();
    for (auto& vol : dVolumes) {
      generator.assignGeometryId(cache, *vol);
    }

    auto detector = Detector::makeShared("DetectorFromProtoContainer", dVolumes,
                                         tryRootVolumes());
  }  // test with different innermost radii
}

BOOST_AUTO_TEST_CASE(WrapContainernRZ) {
  ACTS_LOCAL_LOGGER(getDefaultLogger("Container: Wrap", logLevel));
  ACTS_INFO("*** Test: create a container in Z-R by wrapping.");

  // Test with different inner radii
  std::vector<std::array<ActsScalar, 3u>> radii = {{0., 100., 500.},
                                                   {20., 120., 500.}};

  ActsScalar innerHalfZ = 150.;
  ActsScalar innerBarrelHalfZ = 75.;
  ActsScalar innerEndcapHalfZ = 0.5 * (innerHalfZ - innerBarrelHalfZ);
  ActsScalar outerHalfZ = 175.;

  Transform3 tf = Transform3::Identity();

  // Set up all the different tests
  for (auto [ir, r] : enumerate(radii)) {
    std::string radStr = "_radii_" + std::to_string(ir);
    ACTS_INFO("    -> test series innermost radius setup " << radii[ir][0u]);

    // Let's create the inner container first
    std::vector<std::shared_ptr<DetectorVolume>> iVolumes = {};

    auto iNecBounds = std::make_unique<CylinderVolumeBounds>(
        radii[ir][0u], radii[ir][1u], innerEndcapHalfZ);
    Transform3 ntf = tf;
    ntf.pretranslate(Vector3(0., 0., -innerBarrelHalfZ - innerEndcapHalfZ));
    iVolumes.push_back(DetectorVolumeFactory::construct(
        portalGenerator, tContext, "InnerNec" + radStr, ntf,
        std::move(iNecBounds), tryAllPortals()));

    auto iBarrelBounds = std::make_unique<CylinderVolumeBounds>(
        radii[ir][0u], radii[ir][1u], innerBarrelHalfZ);
    iVolumes.push_back(DetectorVolumeFactory::construct(
        portalGenerator, tContext, "InnerBarrel" + radStr, tf,
        std::move(iBarrelBounds), tryAllPortals()));

    auto iPecBounds = std::make_unique<CylinderVolumeBounds>(
        radii[ir][0u], radii[ir][1u], innerEndcapHalfZ);
    Transform3 ptf = tf;
    ptf.pretranslate(Vector3(0., 0., innerBarrelHalfZ + innerEndcapHalfZ));
    iVolumes.push_back(DetectorVolumeFactory::construct(
        portalGenerator, tContext, "InnerPec" + radStr, ptf,
        std::move(iPecBounds), tryAllPortals()));

    auto innerContainer = connectInZ(tContext, iVolumes, {}, logLevel);

    // Create the wrapping volume
    auto wBounds = std::make_unique<CutoutCylinderVolumeBounds>(
        radii[ir][0u], radii[ir][1u], radii[ir][2u], outerHalfZ, innerHalfZ);
    auto wVolume = DetectorVolumeFactory::construct(
        portalGenerator, tContext, "WrappingVolume" + radStr, tf,
        std::move(wBounds), tryAllPortals());

    std::vector<DetectorComponent::PortalContainer> containers;
    containers.push_back(innerContainer);

    DetectorComponent::PortalContainer outerContainer;
    for (auto [ip, p] : enumerate(wVolume->portalPtrs())) {
      outerContainer[ip] = p;
    }
    containers.push_back(outerContainer);

    auto detector = wrapInZR(tContext, containers, logLevel);
  }
}

BOOST_AUTO_TEST_CASE(RZPhiBoundaries) {
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

  std::vector<std::shared_ptr<DetectorVolume>> volumes = {
      innerV, middleLV, middleDV, middleUV, middleRV, outerV};

  auto boundaries =
      rzphiBoundaries(tContext, volumes, 0., Acts::Logging::VERBOSE);
  BOOST_CHECK_EQUAL(boundaries.size(), 3u);
  // Check the r boundaries
  std::vector<ActsScalar> rBoundaries = {0., 20., 40., 60., 120.};
  BOOST_CHECK(boundaries[0u] == rBoundaries);
  // Check the z boundaries
  std::vector<ActsScalar> zBoundaries = {-100., -90., 90., 100.};
  BOOST_CHECK(boundaries[1u] == zBoundaries);
  BOOST_CHECK_EQUAL(boundaries[2u].size(), 2u);
}

BOOST_AUTO_TEST_CASE(RZPhiBoundariesWithTolerance) {
  auto innerB = std::make_unique<CylinderVolumeBounds>(0., 20., 100);
  auto innerV = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Inner", Transform3::Identity(),
      std::move(innerB), tryAllPortals());

  auto outerB = std::make_unique<CylinderVolumeBounds>(20.001, 100., 100);
  auto outerV = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Inner", Transform3::Identity(),
      std::move(outerB), tryAllPortals());

  std::vector<std::shared_ptr<DetectorVolume>> volumes = {innerV, outerV};

  auto boundariesWoTol =
      rzphiBoundaries(tContext, volumes, 0., Acts::Logging::VERBOSE);
  BOOST_CHECK_EQUAL(boundariesWoTol[0u].size(), 4u);

  auto boundariesWTol =
      rzphiBoundaries(tContext, volumes, 0.01, Acts::Logging::VERBOSE);
  BOOST_CHECK_EQUAL(boundariesWTol[0u].size(), 3u);
}

BOOST_AUTO_TEST_SUITE_END()
