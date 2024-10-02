// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/VolumeStructureBuilder.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"

#include <memory>
#include <vector>

using namespace Acts;
using namespace Acts::Experimental;

GeometryContext tContext;

BOOST_AUTO_TEST_SUITE(Detector)

// Test misconfiguration exception
BOOST_AUTO_TEST_CASE(VolumeStructureBuilderMisconfigured) {
  VolumeStructureBuilder::Config misConfig;

  BOOST_CHECK_THROW(
      VolumeStructureBuilder builder1(
          misConfig, getDefaultLogger("Builder1", Logging::VERBOSE)),
      std::invalid_argument);

  misConfig.boundValues = {0., 1., 2., 3.};
  BOOST_CHECK_THROW(
      VolumeStructureBuilder builder2(
          misConfig, getDefaultLogger("Builder2", Logging::VERBOSE)),
      std::invalid_argument);
}

// Test the creation of conical bounds
BOOST_AUTO_TEST_CASE(VolumeStructureBuilderCone) {
  // Conical volume from parameters
  VolumeStructureBuilder::Config coneValsConfig;
  coneValsConfig.boundValues = {0.2, -200., 0.3, -300., 100.};
  coneValsConfig.boundsType = VolumeBounds::BoundsType::eCone;

  VolumeStructureBuilder coneBuilderVals(
      coneValsConfig,
      getDefaultLogger("ConeStructureBuilderVals", Logging::VERBOSE));

  auto [transformVals, boundsVals, portalGeneratorVals] =
      coneBuilderVals.construct(tContext);
  BOOST_CHECK(transformVals.isApprox(Transform3::Identity()));
  BOOST_REQUIRE(boundsVals != nullptr);
  BOOST_CHECK_EQUAL(boundsVals->type(), VolumeBounds::BoundsType::eCone);
  BOOST_CHECK_EQUAL(boundsVals->values().size(), 7u);
  BOOST_CHECK_EQUAL(boundsVals->values().at(0u), 0.2);
  BOOST_CHECK_EQUAL(boundsVals->values().at(1u), -200.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(2u), 0.3);
  BOOST_CHECK_EQUAL(boundsVals->values().at(3u), -300.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(4u), 100.);

  // Misconfigured - values not complete
  VolumeStructureBuilder::Config coneMis1Config;
  coneMis1Config.boundValues = {100., 200.};
  coneMis1Config.boundsType = VolumeBounds::BoundsType::eCone;

  VolumeStructureBuilder coneBuilderMis1(
      coneMis1Config,
      getDefaultLogger("ConeStructureBuilderMis1", Logging::VERBOSE));

  BOOST_CHECK_THROW(coneBuilderMis1.construct(tContext), std::runtime_error);

  // Misconfigured - tried with extent
  VolumeStructureBuilder::Config coneMis2Config;
  coneMis2Config.extent = Extent{};
  coneMis2Config.boundsType = VolumeBounds::BoundsType::eCone;

  VolumeStructureBuilder coneBuilderMis2(
      coneMis2Config,
      getDefaultLogger("ConeStructureBuilderMis2", Logging::VERBOSE));

  BOOST_CHECK_THROW(coneBuilderMis2.construct(tContext), std::runtime_error);
}

// Test the creation of a cubic bounds
BOOST_AUTO_TEST_CASE(VolumeStructureBuilderCuboid) {
  // Cuboid volume from parameters
  VolumeStructureBuilder::Config cuboidValsConfig;
  cuboidValsConfig.boundValues = {100., 200., 300.};
  cuboidValsConfig.boundsType = VolumeBounds::BoundsType::eCuboid;

  VolumeStructureBuilder cuboidBuilderVals(
      cuboidValsConfig,
      getDefaultLogger("CuboidStructureBuilderVals", Logging::VERBOSE));

  auto [transformVals, boundsVals, portalGeneratorVals] =
      cuboidBuilderVals.construct(tContext);
  BOOST_CHECK(transformVals.isApprox(Transform3::Identity()));
  BOOST_REQUIRE(boundsVals != nullptr);
  BOOST_CHECK_EQUAL(boundsVals->type(), VolumeBounds::BoundsType::eCuboid);
  BOOST_CHECK_EQUAL(boundsVals->values().size(), 3u);
  BOOST_CHECK_EQUAL(boundsVals->values().at(0u), 100.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(1u), 200.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(2u), 300.);

  // Cuboid volume from extent
  Extent cuboidExtent;
  cuboidExtent.set(BinningValue::binX, -100, 100);
  cuboidExtent.set(BinningValue::binY, -200, 200);
  cuboidExtent.set(BinningValue::binZ, -300, 300);

  VolumeStructureBuilder::Config cuboidExtentConfig;
  cuboidExtentConfig.boundsType = VolumeBounds::BoundsType::eCuboid;
  cuboidExtentConfig.extent = cuboidExtent;

  VolumeStructureBuilder cuboidBuilderExtent(
      cuboidExtentConfig,
      getDefaultLogger("CuboidStructureBuilderExtent", Logging::VERBOSE));

  auto [transformExtent, boundsExtent, portalGeneratorExtent] =
      cuboidBuilderExtent.construct(tContext);

  BOOST_CHECK(transformExtent.isApprox(Transform3::Identity()));
  BOOST_REQUIRE(boundsExtent != nullptr);
  BOOST_CHECK_EQUAL(boundsExtent->type(), VolumeBounds::BoundsType::eCuboid);
  BOOST_CHECK_EQUAL(boundsExtent->values().size(), 3u);
  BOOST_CHECK_EQUAL(boundsExtent->values().at(0u), 100.);
  BOOST_CHECK_EQUAL(boundsExtent->values().at(1u), 200.);
  BOOST_CHECK_EQUAL(boundsExtent->values().at(2u), 300.);

  // Misconfigured - values not correct
  VolumeStructureBuilder::Config cuboidMis1Config;
  cuboidMis1Config.boundValues = {100., 200.};
  cuboidMis1Config.boundsType = VolumeBounds::BoundsType::eCuboid;

  VolumeStructureBuilder cuboidBuilderMis1(
      cuboidMis1Config,
      getDefaultLogger("CuboidStructureBuilderMis1", Logging::VERBOSE));

  BOOST_CHECK_THROW(cuboidBuilderMis1.construct(tContext), std::runtime_error);

  // Misconfigured - extent not correct
  VolumeStructureBuilder::Config cuboidMis2Config;
  cuboidMis2Config.extent = Extent{};
  cuboidMis2Config.boundsType = VolumeBounds::BoundsType::eCuboid;

  VolumeStructureBuilder cuboidBuilderMis2(
      cuboidMis2Config,
      getDefaultLogger("CuboidStructureBuilderMis2", Logging::VERBOSE));

  BOOST_CHECK_THROW(cuboidBuilderMis2.construct(tContext), std::runtime_error);
}

// Test the creation of cutout cylinder bounds
BOOST_AUTO_TEST_CASE(VolumeStructureBuilderCutoutCylinder) {
  // Cutout Cylinder volume from parameters
  VolumeStructureBuilder::Config ccylValsConfig;
  ccylValsConfig.boundValues = {100, 120., 200, 300., 280.};
  ccylValsConfig.boundsType = VolumeBounds::BoundsType::eCutoutCylinder;

  VolumeStructureBuilder ccylBuilderVals(
      ccylValsConfig,
      getDefaultLogger("CutoutCylinderStructureBuilderVals", Logging::VERBOSE));

  auto [transformVals, boundsVals, portalGeneratorVals] =
      ccylBuilderVals.construct(tContext);
  BOOST_CHECK(transformVals.isApprox(Transform3::Identity()));
  BOOST_REQUIRE(boundsVals != nullptr);
  BOOST_CHECK_EQUAL(boundsVals->type(),
                    VolumeBounds::BoundsType::eCutoutCylinder);
  BOOST_CHECK_EQUAL(boundsVals->values().size(), 5u);
  BOOST_CHECK_EQUAL(boundsVals->values().at(0u), 100.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(1u), 120.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(2u), 200.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(3u), 300.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(4u), 280.);

  // Misconfigured - values not complete
  VolumeStructureBuilder::Config ccylMis1Config;
  ccylMis1Config.boundValues = {100., 200.};
  ccylMis1Config.boundsType = VolumeBounds::BoundsType::eCutoutCylinder;

  VolumeStructureBuilder ccylBuilderMis1(
      ccylMis1Config,
      getDefaultLogger("CutoutCylinderStructureBuilderMis1", Logging::VERBOSE));

  BOOST_CHECK_THROW(ccylBuilderMis1.construct(tContext), std::runtime_error);

  // Misconfigured - trying from extent
  VolumeStructureBuilder::Config ccylMis2Config;
  ccylMis2Config.extent = Extent{};
  ccylMis2Config.boundsType = VolumeBounds::BoundsType::eCutoutCylinder;

  VolumeStructureBuilder ccylBuilderMis2(
      ccylMis2Config,
      getDefaultLogger("CutoutCylinderStructureBuilderMis2", Logging::VERBOSE));

  BOOST_CHECK_THROW(ccylBuilderMis2.construct(tContext), std::runtime_error);
}

// Test the creation of cylindrical bounds
BOOST_AUTO_TEST_CASE(VolumeStructureBuilderCylinder) {
  // Cylinder volume from parameters
  VolumeStructureBuilder::Config cylValsConfig;
  cylValsConfig.boundValues = {100, 200, 400., 0.3, 0.};
  cylValsConfig.boundsType = VolumeBounds::BoundsType::eCylinder;

  VolumeStructureBuilder cylBuilderVals(
      cylValsConfig,
      getDefaultLogger("CylinderStructureBuilderVals", Logging::VERBOSE));

  auto [transformVals, boundsVals, portalGeneratorVals] =
      cylBuilderVals.construct(tContext);
  BOOST_CHECK(transformVals.isApprox(Transform3::Identity()));
  BOOST_REQUIRE(boundsVals != nullptr);
  BOOST_CHECK_EQUAL(boundsVals->type(), VolumeBounds::BoundsType::eCylinder);
  BOOST_CHECK_EQUAL(boundsVals->values().size(), 7u);
  BOOST_CHECK_EQUAL(boundsVals->values().at(0u), 100.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(1u), 200.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(2u), 400.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(3u), 0.3);

  // Cylinder volume from extent
  Extent cylinderExtent;
  cylinderExtent.set(BinningValue::binR, 100., 200.);
  cylinderExtent.set(BinningValue::binZ, -800., 0.);

  VolumeStructureBuilder::Config cylExtentConfig;
  cylExtentConfig.extent = cylinderExtent;
  cylExtentConfig.boundsType = VolumeBounds::BoundsType::eCylinder;

  VolumeStructureBuilder cylBuilderExtent(
      cylExtentConfig,
      getDefaultLogger("CylinderStructureBuilderExtent", Logging::VERBOSE));

  auto [transformExtent, boundsExtent, portalGeneratorExtent] =
      cylBuilderExtent.construct(tContext);

  Transform3 shifted = Transform3::Identity();
  shifted.pretranslate(Vector3(0., 0., -400.));

  BOOST_CHECK(transformExtent.isApprox(shifted));
  BOOST_REQUIRE(boundsExtent != nullptr);
  BOOST_CHECK_EQUAL(boundsExtent->type(), VolumeBounds::BoundsType::eCylinder);
  BOOST_CHECK_EQUAL(boundsExtent->values().size(), 7u);
  BOOST_CHECK_EQUAL(boundsExtent->values().at(0u), 100.);
  BOOST_CHECK_EQUAL(boundsExtent->values().at(1u), 200.);
  BOOST_CHECK_EQUAL(boundsExtent->values().at(2u), 400.);

  // Misconfigured - values not complete
  VolumeStructureBuilder::Config cylMis1Config;
  cylMis1Config.boundValues = {100., 200.};
  cylMis1Config.boundsType = VolumeBounds::BoundsType::eCylinder;

  VolumeStructureBuilder cylBuilderMis1(
      cylMis1Config,
      getDefaultLogger("CylinderStructureBuilderMis1", Logging::VERBOSE));

  BOOST_CHECK_THROW(cylBuilderMis1.construct(tContext), std::runtime_error);

  // Misconfigured - trying from extent
  VolumeStructureBuilder::Config cylMis2Config;
  cylMis2Config.extent = Extent{};
  cylMis2Config.boundsType = VolumeBounds::BoundsType::eCylinder;

  VolumeStructureBuilder cylBuilderMis2(
      cylMis2Config,
      getDefaultLogger("CylinderStructureBuilderMis2", Logging::VERBOSE));

  BOOST_CHECK_THROW(cylBuilderMis2.construct(tContext), std::runtime_error);
}

// Test the creation of generic cuboid bounds
BOOST_AUTO_TEST_CASE(VolumeStructureBuilderGenericCuboid) {
  // Cuboid volume from parameters
  VolumeStructureBuilder::Config gcubValsConfig;
  gcubValsConfig.boundValues = {0, 0, 0, 2,   0, 0.4, 2,   1, 0.4, 0, 1, 0,
                                0, 0, 1, 1.8, 0, 1,   1.8, 1, 1,   0, 1, 1};
  gcubValsConfig.boundsType = VolumeBounds::BoundsType::eGenericCuboid;

  VolumeStructureBuilder gcubBuilderVals(
      gcubValsConfig,
      getDefaultLogger("GenericCuboidStructureBuilderVals", Logging::VERBOSE));

  auto [transformVals, boundsVals, portalGeneratorVals] =
      gcubBuilderVals.construct(tContext);
  BOOST_CHECK(transformVals.isApprox(Transform3::Identity()));
  BOOST_REQUIRE(boundsVals != nullptr);
  BOOST_CHECK_EQUAL(boundsVals->type(),
                    VolumeBounds::BoundsType::eGenericCuboid);
  BOOST_CHECK_EQUAL(boundsVals->values().size(), 24u);

  // Misconfigured - values not complete
  VolumeStructureBuilder::Config gcubMis1Config;
  gcubMis1Config.boundsType = VolumeBounds::BoundsType::eGenericCuboid;
  gcubMis1Config.boundValues = {100.};

  VolumeStructureBuilder gcubBuilderMis1(
      gcubMis1Config,
      getDefaultLogger("GenericCuboidStructureBuilderMis1", Logging::VERBOSE));

  BOOST_CHECK_THROW(gcubBuilderMis1.construct(tContext), std::runtime_error);

  // Misconfigured - tried with extent
  VolumeStructureBuilder::Config gcubMis2Config;
  gcubMis2Config.boundsType = VolumeBounds::BoundsType::eGenericCuboid;
  gcubMis2Config.extent = Extent{};

  VolumeStructureBuilder gcubBuilderMis2(
      gcubMis2Config,
      getDefaultLogger("GenericCuboidStructureBuilderMis2", Logging::VERBOSE));
}

// Test the creation of the trapezoidal bounds
BOOST_AUTO_TEST_CASE(VolumeStructureBuilderTrapezoid) {
  // Cuboid volume from parameters
  VolumeStructureBuilder::Config trapValsConfig;
  trapValsConfig.boundValues = {100., 200., 300., 10.};
  trapValsConfig.boundsType = VolumeBounds::BoundsType::eTrapezoid;

  VolumeStructureBuilder trapBuilderVals(
      trapValsConfig,
      getDefaultLogger("TrapezoidStructureBuilderVals", Logging::VERBOSE));

  auto [transformVals, boundsVals, portalGeneratorVals] =
      trapBuilderVals.construct(tContext);
  BOOST_CHECK(transformVals.isApprox(Transform3::Identity()));
  BOOST_REQUIRE(boundsVals != nullptr);
  BOOST_CHECK_EQUAL(boundsVals->type(), VolumeBounds::BoundsType::eTrapezoid);
  BOOST_CHECK_EQUAL(boundsVals->values().size(), 6u);
  BOOST_CHECK_EQUAL(boundsVals->values().at(0u), 100.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(1u), 200.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(2u), 300.);
  BOOST_CHECK_EQUAL(boundsVals->values().at(3u), 10.);

  // Misconfigured - values not complete
  VolumeStructureBuilder::Config trapMis1Config;
  trapMis1Config.boundsType = VolumeBounds::BoundsType::eTrapezoid;
  trapMis1Config.boundValues = {100., 200.};

  VolumeStructureBuilder trapBuilderMis1(
      trapMis1Config,
      getDefaultLogger("TrapezoidStructureBuilderMis1", Logging::VERBOSE));

  BOOST_CHECK_THROW(trapBuilderMis1.construct(tContext), std::runtime_error);

  // Misconfigured - tried with extent
  VolumeStructureBuilder::Config trapMis2Config;
  trapMis2Config.boundsType = VolumeBounds::BoundsType::eTrapezoid;
  trapMis2Config.extent = Extent{};

  VolumeStructureBuilder trapBuilderMis2(
      trapMis2Config,
      getDefaultLogger("TrapezoidStructureBuilderMis2", Logging::VERBOSE));
}

BOOST_AUTO_TEST_SUITE_END()
