// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"
#include "Acts/Surfaces/ConeBounds.hpp"
#include "Acts/Surfaces/ConeSurface.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"

#include <fstream>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>

using namespace Acts;

namespace {
std::ofstream out;

Acts::GeometryContext gctx;
}  // namespace

BOOST_AUTO_TEST_SUITE(SurfaceJsonConversion)

BOOST_AUTO_TEST_CASE(ConeSurfaceRoundTripTests) {
  Transform3 trf(Transform3::Identity() * Translation3(0., 0., -7.));
  auto cone = std::make_shared<ConeBounds>(0.123, 10., 100.);
  auto coneRef = Surface::makeShared<ConeSurface>(trf, cone);
  coneRef->assignGeometryId(GeometryIdentifier(13u));

  // Test a cone
  nlohmann::json coneOut = SurfaceJsonConverter::toJson(gctx, *coneRef);
  out.open("ConeSurface.json");
  out << coneOut.dump(2);
  out.close();

  auto in = std::ifstream("ConeSurface.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json coneIn;
  in >> coneIn;
  in.close();

  auto coneTest = SurfaceJsonConverter::fromJson(coneIn);

  BOOST_CHECK(coneTest->transform(gctx).isApprox(coneRef->transform(gctx)));
  BOOST_CHECK_EQUAL(coneTest->geometryId(), coneRef->geometryId());
  BOOST_CHECK_EQUAL(coneTest->bounds(), coneRef->bounds());
}

BOOST_AUTO_TEST_CASE(DiscSurfaceRoundTripTests) {
  Transform3 trf(Transform3::Identity() * Translation3(0., 0., -7.));
  auto ring = std::make_shared<RadialBounds>(0., 4.);
  auto ringDiscRef = Surface::makeShared<DiscSurface>(trf, ring);
  ringDiscRef->assignGeometryId(GeometryIdentifier(10u));

  // Test a disc
  nlohmann::json discOut = SurfaceJsonConverter::toJson(gctx, *ringDiscRef);
  out.open("DiscSurface.json");
  out << discOut.dump(2);
  out.close();

  auto in = std::ifstream("DiscSurface.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json discIn;
  in >> discIn;
  in.close();

  auto ringDiscTest = SurfaceJsonConverter::fromJson(discIn);

  BOOST_CHECK(
      ringDiscTest->transform(gctx).isApprox(ringDiscRef->transform(gctx)));
  BOOST_CHECK_EQUAL(ringDiscTest->geometryId(), ringDiscRef->geometryId());
  BOOST_CHECK_EQUAL(ringDiscTest->bounds(), ringDiscRef->bounds());
}

BOOST_AUTO_TEST_CASE(CylinderSurfaceRoundTripTests) {
  Transform3 trf(Transform3::Identity() * Translation3(0., 0., -7.));
  auto tube = std::make_shared<CylinderBounds>(5., 20.);
  auto cylinderRef = Surface::makeShared<CylinderSurface>(trf, tube);
  cylinderRef->assignGeometryId(GeometryIdentifier(11u));

  // Test a cyoinder
  nlohmann::json cylinderOut = SurfaceJsonConverter::toJson(gctx, *cylinderRef);
  out.open("CylinderSurface.json");
  out << cylinderOut.dump(2);
  out.close();

  auto in = std::ifstream("CylinderSurface.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json cylinderIn;
  in >> cylinderIn;
  in.close();

  auto cylinderTest = SurfaceJsonConverter::fromJson(cylinderIn);

  BOOST_CHECK(
      cylinderTest->transform(gctx).isApprox(cylinderRef->transform(gctx)));
  BOOST_CHECK_EQUAL(cylinderTest->geometryId(), cylinderRef->geometryId());
  BOOST_CHECK_EQUAL(cylinderTest->bounds(), cylinderRef->bounds());
}

BOOST_AUTO_TEST_CASE(PlaneSurfaceRoundTripTests) {
  Transform3 trf(Transform3::Identity() * Translation3(0., 0., -7.));
  auto trapezoid = std::make_shared<TrapezoidBounds>(2., 3., 4.);
  auto trapezoidPlaneRef = Surface::makeShared<PlaneSurface>(trf, trapezoid);
  trapezoidPlaneRef->assignGeometryId(GeometryIdentifier(9u));

  // Test a plane
  nlohmann::json planeOut =
      SurfaceJsonConverter::toJson(gctx, *trapezoidPlaneRef);
  to_json(planeOut, *trapezoidPlaneRef);
  out.open("PlaneSurface.json");
  out << planeOut.dump(2);
  out.close();

  auto in = std::ifstream("PlaneSurface.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json planeIn;
  in >> planeIn;
  in.close();

  auto trapezoidPlaneTest = SurfaceJsonConverter::fromJson(planeIn);

  BOOST_CHECK(trapezoidPlaneTest->transform(gctx).isApprox(
      trapezoidPlaneRef->transform(gctx)));
  BOOST_CHECK_EQUAL(trapezoidPlaneTest->geometryId(),
                    trapezoidPlaneRef->geometryId());
  BOOST_CHECK_EQUAL(trapezoidPlaneTest->bounds(), trapezoidPlaneRef->bounds());
}

BOOST_AUTO_TEST_CASE(StrawSurfaceRoundTripTests) {
  Transform3 trf(Transform3::Identity() * Translation3(0., 0., -7.));
  auto straw = std::make_shared<LineBounds>(1., 100.);
  auto strawRef = Surface::makeShared<StrawSurface>(trf, straw);
  strawRef->assignGeometryId(GeometryIdentifier(12u));

  // Test a straw
  nlohmann::json strawOut = SurfaceJsonConverter::toJson(gctx, *strawRef);
  out.open("StrawSurface.json");
  out << strawOut.dump(2);
  out.close();

  auto in = std::ifstream("StrawSurface.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json strawIn;
  in >> strawIn;
  in.close();

  auto strawTest = SurfaceJsonConverter::fromJson(strawIn);

  BOOST_CHECK(strawTest->transform(gctx).isApprox(strawRef->transform(gctx)));
  BOOST_CHECK_EQUAL(strawTest->geometryId(), strawRef->geometryId());
  BOOST_CHECK_EQUAL(strawTest->bounds(), strawRef->bounds());
}

BOOST_AUTO_TEST_CASE(PerigeeRoundTripTests) {
  Transform3 trf(Transform3::Identity() * Translation3(-1., -2., -7.));
  auto perigeeRef = Surface::makeShared<PerigeeSurface>(trf);
  perigeeRef->assignGeometryId(GeometryIdentifier(99u));

  // Test a perigee
  nlohmann::json perigeeOut = SurfaceJsonConverter::toJson(gctx, *perigeeRef);
  out.open("PerigeeSurface.json");
  out << perigeeOut.dump(2);
  out.close();

  auto in = std::ifstream("PerigeeSurface.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json perigeeIn;
  in >> perigeeIn;
  in.close();

  auto perigeeTest = SurfaceJsonConverter::fromJson(perigeeIn);

  BOOST_CHECK(
      perigeeTest->transform(gctx).isApprox(perigeeRef->transform(gctx)));
  BOOST_CHECK_EQUAL(perigeeTest->geometryId(), perigeeRef->geometryId());
}

BOOST_AUTO_TEST_CASE(SurfacesDetrayTests) {
  Transform3 trf(Transform3::Identity() * Translation3(0., 0., -7.));
  auto trapezoid = std::make_shared<TrapezoidBounds>(2., 3., 4.);
  auto trapezoidPlaneRef = Surface::makeShared<PlaneSurface>(trf, trapezoid);
  trapezoidPlaneRef->assignGeometryId(GeometryIdentifier(9u));

  // Test a rectangle
  nlohmann::json trapOut =
      SurfaceJsonConverter::toJsonDetray(gctx, *trapezoidPlaneRef);
  out.open("Surfaces-detray.json");
  out << trapOut.dump(2);
  out.close();
}

BOOST_AUTO_TEST_SUITE_END()
