// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Surfaces/ConeBounds.hpp"
#include "Acts/Surfaces/ConeSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Result.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();

BOOST_AUTO_TEST_SUITE(ConeSurfaces)

/// Unit test for creating compliant/non-compliant ConeSurface object
BOOST_AUTO_TEST_CASE(ConeSurfaceConstruction) {
  // ConeSurface default constructor is deleted
  //
  /// Constructor with transform, alpha and symmetry
  /// indicator
  double alpha{M_PI / 8.}, halfPhiSector{M_PI / 16.}, zMin{1.0}, zMax{10.};
  bool symmetric(false);
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  BOOST_CHECK_EQUAL(
      Surface::makeShared<ConeSurface>(Transform3::Identity(), alpha, symmetric)
          ->type(),
      Surface::Cone);
  BOOST_CHECK_EQUAL(
      Surface::makeShared<ConeSurface>(pTransform, alpha, symmetric)->type(),
      Surface::Cone);
  //
  /// Constructor with transform pointer, alpha,z min and max, halfPhiSector
  BOOST_CHECK_EQUAL(Surface::makeShared<ConeSurface>(pTransform, alpha, zMin,
                                                     zMax, halfPhiSector)
                        ->type(),
                    Surface::Cone);

  /// Constructor with transform and ConeBounds pointer
  // ConeBounds (double alpha, double zmin, double zmax, double halfphi=M_PI,
  // double avphi=0.)
  auto pConeBounds =
      std::make_shared<const ConeBounds>(alpha, zMin, zMax, halfPhiSector, 0.);
  BOOST_CHECK_EQUAL(
      Surface::makeShared<ConeSurface>(pTransform, pConeBounds)->type(),
      Surface::Cone);
  //
  //
  /// Copy constructor
  auto coneSurfaceObject =
      Surface::makeShared<ConeSurface>(pTransform, alpha, symmetric);
  auto copiedConeSurface = Surface::makeShared<ConeSurface>(*coneSurfaceObject);
  BOOST_CHECK_EQUAL(copiedConeSurface->type(), Surface::Cone);
  BOOST_CHECK(*copiedConeSurface == *coneSurfaceObject);
  //
  /// Copied and transformed
  auto copiedTransformedConeSurface = Surface::makeShared<ConeSurface>(
      tgContext, *coneSurfaceObject, pTransform);
  BOOST_CHECK_EQUAL(copiedTransformedConeSurface->type(), Surface::Cone);

  /// Construct with nullptr bounds
  BOOST_CHECK_THROW(auto nullBounds = Surface::makeShared<ConeSurface>(
                        Transform3::Identity(), nullptr),
                    AssertionFailureException);
}
//
/// Unit test for testing ConeSurface properties
BOOST_AUTO_TEST_CASE(ConeSurfaceProperties) {
  /// Test clone method
  double alpha{M_PI / 8.} /*,halfPhiSector{M_PI/16.}, zMin{1.0}, zMax{10.}*/;
  bool symmetric(false);
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  auto coneSurfaceObject =
      Surface::makeShared<ConeSurface>(pTransform, alpha, symmetric);
  //
  /// Test type (redundant)
  BOOST_CHECK_EQUAL(coneSurfaceObject->type(), Surface::Cone);
  //
  /// Test binningPosition
  Vector3 binningPosition{0., 1., 2.};
  CHECK_CLOSE_ABS(
      coneSurfaceObject->binningPosition(tgContext, BinningValue::binPhi),
      binningPosition, 1e-6);
  //
  /// Test referenceFrame
  Vector3 globalPosition{2.0, 2.0, 2.0};
  Vector3 momentum{1.e6, 1.e6, 1.e6};
  double rootHalf = std::sqrt(0.5);
  RotationMatrix3 expectedFrame;
  expectedFrame << -rootHalf, 0., rootHalf, rootHalf, 0., rootHalf, 0., 1., 0.;
  CHECK_CLOSE_OR_SMALL(
      coneSurfaceObject->referenceFrame(tgContext, globalPosition, momentum),
      expectedFrame, 1e-6, 1e-9);
  //
  /// Test normal, given 3D position
  Vector3 origin{0., 0., 0.};
  Vector3 normal3D = {0., -1., 0.};
  CHECK_CLOSE_ABS(coneSurfaceObject->normal(tgContext, origin), normal3D, 1e-6);
  //
  /// Test normal given 2D rphi position
  Vector2 positionPiBy2(1.0, M_PI / 2.);
  Vector3 normalAtPiBy2{0.0312768, 0.92335, -0.382683};

  CHECK_CLOSE_OR_SMALL(coneSurfaceObject->normal(tgContext, positionPiBy2),
                       normalAtPiBy2, 1e-2, 1e-9);
  //
  /// Test rotational symmetry axis
  Vector3 symmetryAxis{0., 0., 1.};
  CHECK_CLOSE_ABS(coneSurfaceObject->rotSymmetryAxis(tgContext), symmetryAxis,
                  1e-6);
  //
  /// Test bounds
  BOOST_CHECK_EQUAL(coneSurfaceObject->bounds().type(), SurfaceBounds::eCone);
  //
  /// Test localToGlobal
  Vector2 localPosition{1.0, M_PI / 2.0};
  globalPosition =
      coneSurfaceObject->localToGlobal(tgContext, localPosition, momentum);
  // std::cout<<globalPosition<<std::endl;
  Vector3 expectedPosition{0.0220268, 1.65027, 3.5708};

  CHECK_CLOSE_REL(globalPosition, expectedPosition, 1e-2);
  //
  /// Testing globalToLocal
  localPosition =
      coneSurfaceObject->globalToLocal(tgContext, globalPosition, momentum)
          .value();
  // std::cout<<localPosition<<std::endl;
  Vector2 expectedLocalPosition{1.0, M_PI / 2.0};

  CHECK_CLOSE_REL(localPosition, expectedLocalPosition, 1e-6);
  //
  /// Test isOnSurface
  Vector3 offSurface{100, 1, 2};
  BOOST_CHECK(coneSurfaceObject->isOnSurface(
      tgContext, globalPosition, momentum, BoundaryTolerance::None()));
  BOOST_CHECK(!coneSurfaceObject->isOnSurface(tgContext, offSurface, momentum,
                                              BoundaryTolerance::None()));

  /// Test pathCorrection
  CHECK_CLOSE_REL(coneSurfaceObject->pathCorrection(tgContext, offSurface,
                                                    momentum.normalized()),
                  0.40218866453252877, 0.01);
  //
  /// Test name
  BOOST_CHECK_EQUAL(coneSurfaceObject->name(),
                    std::string("Acts::ConeSurface"));
  //
  /// Test dump
  // TODO 2017-04-12 msmk: check how to correctly check output
  //    boost::test_tools::output_test_stream dumpOuput;
  //    coneSurfaceObject.toStream(dumpOuput);
  //    BOOST_CHECK(dumpOuput.is_equal(
  //      "Acts::ConeSurface\n"
  //      "    Center position  (x, y, z) = (0.0000, 1.0000, 2.0000)\n"
  //      "    Rotation:             colX = (1.000000, 0.000000, 0.000000)\n"
  //      "                          colY = (0.000000, 1.000000, 0.000000)\n"
  //      "                          colZ = (0.000000, 0.000000, 1.000000)\n"
  //      "    Bounds  : Acts::ConeBounds: (tanAlpha, minZ, maxZ, averagePhi,
  //      halfPhiSector) = (0.4142136, 0.0000000, inf, 0.0000000,
  //      3.1415927)"));
}

BOOST_AUTO_TEST_CASE(ConeSurfaceEqualityOperators) {
  double alpha{M_PI / 8.} /*, halfPhiSector{M_PI/16.}, zMin{1.0}, zMax{10.}*/;
  bool symmetric(false);
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  auto coneSurfaceObject =
      Surface::makeShared<ConeSurface>(pTransform, alpha, symmetric);
  //
  auto coneSurfaceObject2 =
      Surface::makeShared<ConeSurface>(pTransform, alpha, symmetric);
  //
  /// Test equality operator
  BOOST_CHECK(*coneSurfaceObject == *coneSurfaceObject2);
  //
  BOOST_TEST_CHECKPOINT(
      "Create and then assign a ConeSurface object to the existing one");
  /// Test assignment
  auto assignedConeSurface =
      Surface::makeShared<ConeSurface>(Transform3::Identity(), 0.1, true);
  *assignedConeSurface = *coneSurfaceObject;
  /// Test equality of assigned to original
  BOOST_CHECK(*assignedConeSurface == *coneSurfaceObject);
}

BOOST_AUTO_TEST_CASE(ConeSurfaceExtent) {
  double alpha{M_PI / 8.}, zMin{0.}, zMax{10.};

  Translation3 translation{0., 0., 0.};

  // Testing a Full cone
  auto pTransform = Transform3(translation);
  auto pConeBounds = std::make_shared<const ConeBounds>(alpha, zMin, zMax);
  auto pCone = Surface::makeShared<ConeSurface>(pTransform, pConeBounds);
  auto pConeExtent = pCone->polyhedronRepresentation(tgContext, 1).extent();

  double rMax = zMax * std::tan(alpha);
  CHECK_CLOSE_ABS(zMin, pConeExtent.min(BinningValue::binZ),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(zMax, pConeExtent.max(BinningValue::binZ),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(0., pConeExtent.min(BinningValue::binR),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(rMax, pConeExtent.max(BinningValue::binR),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(-rMax, pConeExtent.min(BinningValue::binX),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(rMax, pConeExtent.max(BinningValue::binX),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(-rMax, pConeExtent.min(BinningValue::binY),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(rMax, pConeExtent.max(BinningValue::binY),
                  s_onSurfaceTolerance);

  // Now a sector
  double halfPhiSector = M_PI / 8.;
  pConeBounds =
      std::make_shared<const ConeBounds>(alpha, zMin, zMax, halfPhiSector, 0.);
  pCone = Surface::makeShared<ConeSurface>(pTransform, pConeBounds);
  pConeExtent = pCone->polyhedronRepresentation(tgContext, 1).extent();

  CHECK_CLOSE_ABS(zMin, pConeExtent.min(BinningValue::binZ),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(zMax, pConeExtent.max(BinningValue::binZ),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(0., pConeExtent.min(BinningValue::binR),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(rMax, pConeExtent.max(BinningValue::binR),
                  s_onSurfaceTolerance);
}

/// Unit test for testing ConeSurface alignment derivatives
BOOST_AUTO_TEST_CASE(ConeSurfaceAlignment) {
  double alpha{M_PI / 8.};
  bool symmetric(false);
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  auto coneSurfaceObject =
      Surface::makeShared<ConeSurface>(pTransform, alpha, symmetric);

  const auto& rotation = pTransform.rotation();
  // The local frame z axis
  const Vector3 localZAxis = rotation.col(2);
  // Check the local z axis is aligned to global z axis
  CHECK_CLOSE_ABS(localZAxis, Vector3(0., 0., 1.), 1e-15);

  /// Define the track (global) position and direction
  Vector3 globalPosition{0, 1. + std::tan(alpha), 3};

  // Test the derivative of bound track parameters local position w.r.t.
  // position in local 3D Cartesian coordinates
  const auto& loc3DToLocBound =
      coneSurfaceObject->localCartesianToBoundLocalDerivative(tgContext,
                                                              globalPosition);
  // Check if the result is as expected
  ActsMatrix<2, 3> expLoc3DToLocBound = ActsMatrix<2, 3>::Zero();
  expLoc3DToLocBound << -1, 0, M_PI / 2. * std::tan(alpha), 0, 0, 1;
  CHECK_CLOSE_ABS(loc3DToLocBound, expLoc3DToLocBound, 1e-10);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Acts::Test
