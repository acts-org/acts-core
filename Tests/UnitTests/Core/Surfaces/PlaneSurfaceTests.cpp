// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// #include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Alignment.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Result.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();

BOOST_AUTO_TEST_SUITE(PlaneSurfaces)
/// Unit test for creating compliant/non-compliant PlaneSurface object
BOOST_AUTO_TEST_CASE(PlaneSurfaceConstruction) {
  // PlaneSurface default constructor is deleted
  // bounds object, rectangle type
  auto rBounds = std::make_shared<const RectangleBounds>(3., 4.);
  /// Constructor with transform and bounds
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  // constructor with transform
  BOOST_CHECK_EQUAL(
      Surface::makeShared<PlaneSurface>(pTransform, rBounds)->type(),
      Surface::Plane);
  /// Copy constructor
  auto planeSurfaceObject =
      Surface::makeShared<PlaneSurface>(pTransform, rBounds);
  auto copiedPlaneSurface =
      Surface::makeShared<PlaneSurface>(*planeSurfaceObject);
  BOOST_CHECK_EQUAL(copiedPlaneSurface->type(), Surface::Plane);
  BOOST_CHECK(*copiedPlaneSurface == *planeSurfaceObject);
  //
  /// Copied and transformed
  auto copiedTransformedPlaneSurface = Surface::makeShared<PlaneSurface>(
      tgContext, *planeSurfaceObject, pTransform);
  BOOST_CHECK_EQUAL(copiedTransformedPlaneSurface->type(), Surface::Plane);

  /// Construct with nullptr bounds
  DetectorElementStub detElem;
  BOOST_CHECK_THROW(
      auto nullBounds = Surface::makeShared<PlaneSurface>(nullptr, detElem),
      AssertionFailureException);
}
//
/// Unit test for testing PlaneSurface properties
BOOST_AUTO_TEST_CASE(PlaneSurfaceProperties) {
  // bounds object, rectangle type
  auto rBounds = std::make_shared<const RectangleBounds>(3., 4.);
  /// Test clone method
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  auto planeSurfaceObject =
      Surface::makeShared<PlaneSurface>(pTransform, rBounds);
  // Is it in the right place?
  Translation3 translation2{0., 2., 4.};
  auto pTransform2 = Transform3(translation2);
  auto planeSurfaceObject2 =
      Surface::makeShared<PlaneSurface>(pTransform2, rBounds);
  /// Test type (redundant)
  BOOST_CHECK_EQUAL(planeSurfaceObject->type(), Surface::Plane);
  //
  /// Test binningPosition
  Vector3 binningPosition{0., 1., 2.};
  BOOST_CHECK_EQUAL(
      planeSurfaceObject->binningPosition(tgContext, BinningValue::binX),
      binningPosition);
  //
  /// Test referenceFrame
  Vector3 arbitraryGlobalPosition{2.0, 2.0, 2.0};
  Vector3 momentum{1.e6, 1.e6, 1.e6};
  RotationMatrix3 expectedFrame;
  expectedFrame << 1., 0., 0., 0., 1., 0., 0., 0., 1.;

  CHECK_CLOSE_OR_SMALL(planeSurfaceObject->referenceFrame(
                           tgContext, arbitraryGlobalPosition, momentum),
                       expectedFrame, 1e-6, 1e-9);
  //
  /// Test normal, given 3D position
  Vector3 normal3D(0., 0., 1.);
  BOOST_CHECK_EQUAL(planeSurfaceObject->normal(tgContext), normal3D);
  //
  /// Test bounds
  BOOST_CHECK_EQUAL(planeSurfaceObject->bounds().type(),
                    SurfaceBounds::eRectangle);

  /// Test localToGlobal
  Vector2 localPosition{1.5, 1.7};
  Vector3 globalPosition =
      planeSurfaceObject->localToGlobal(tgContext, localPosition, momentum);
  //
  // expected position is the translated one
  Vector3 expectedPosition{1.5 + translation.x(), 1.7 + translation.y(),
                           translation.z()};

  CHECK_CLOSE_REL(globalPosition, expectedPosition, 1e-2);
  //
  /// Testing globalToLocal
  localPosition =
      planeSurfaceObject->globalToLocal(tgContext, globalPosition, momentum)
          .value();
  Vector2 expectedLocalPosition{1.5, 1.7};

  CHECK_CLOSE_REL(localPosition, expectedLocalPosition, 1e-2);

  Vector3 globalPositionOff =
      globalPosition +
      planeSurfaceObject->normal(tgContext, localPosition) * 0.1;

  BOOST_CHECK(
      planeSurfaceObject->globalToLocal(tgContext, globalPositionOff, momentum)
          .error());
  BOOST_CHECK(planeSurfaceObject
                  ->globalToLocal(tgContext, globalPositionOff, momentum, 0.05)
                  .error());
  BOOST_CHECK(planeSurfaceObject
                  ->globalToLocal(tgContext, globalPositionOff, momentum, 0.2)
                  .ok());

  /// Test isOnSurface
  Vector3 offSurface{0, 1, -2.};
  BOOST_CHECK(planeSurfaceObject->isOnSurface(
      tgContext, globalPosition, momentum, BoundaryTolerance::None()));
  BOOST_CHECK(planeSurfaceObject->isOnSurface(tgContext, globalPosition,
                                              BoundaryTolerance::None()));
  BOOST_CHECK(!planeSurfaceObject->isOnSurface(tgContext, offSurface, momentum,
                                               BoundaryTolerance::None()));
  BOOST_CHECK(!planeSurfaceObject->isOnSurface(tgContext, offSurface,
                                               BoundaryTolerance::None()));
  //
  // Test intersection
  Vector3 direction{0., 0., 1.};
  auto sfIntersection = planeSurfaceObject
                            ->intersect(tgContext, offSurface, direction,
                                        BoundaryTolerance::None())
                            .closest();
  Intersection3D expectedIntersect{Vector3{0, 1, 2}, 4.,
                                   Intersection3D::Status::reachable};
  BOOST_CHECK(sfIntersection.isValid());
  BOOST_CHECK_EQUAL(sfIntersection.position(), expectedIntersect.position());
  BOOST_CHECK_EQUAL(sfIntersection.pathLength(),
                    expectedIntersect.pathLength());
  BOOST_CHECK_EQUAL(sfIntersection.object(), planeSurfaceObject.get());
  //

  /// Test pathCorrection
  CHECK_CLOSE_REL(planeSurfaceObject->pathCorrection(tgContext, offSurface,
                                                     momentum.normalized()),
                  std::sqrt(3), 0.01);
  //
  /// Test name
  BOOST_CHECK_EQUAL(planeSurfaceObject->name(),
                    std::string("Acts::PlaneSurface"));
  //
  /// Test dump
  // TODO 2017-04-12 msmk: check how to correctly check output
  //    boost::test_tools::output_test_stream dumpOuput;
  //    planeSurfaceObject.toStream(dumpOuput);
  //    BOOST_CHECK(dumpOuput.is_equal(
  //      "Acts::PlaneSurface\n"
  //      "    Center position  (x, y, z) = (0.0000, 1.0000, 2.0000)\n"
  //      "    Rotation:             colX = (1.000000, 0.000000, 0.000000)\n"
  //      "                          colY = (0.000000, 1.000000, 0.000000)\n"
  //      "                          colZ = (0.000000, 0.000000, 1.000000)\n"
  //      "    Bounds  : Acts::ConeBounds: (tanAlpha, minZ, maxZ, averagePhi,
  //      halfPhiSector) = (0.4142136, 0.0000000, inf, 0.0000000,
  //      3.1415927)"));
}

BOOST_AUTO_TEST_CASE(PlaneSurfaceEqualityOperators) {
  // rectangle bounds
  auto rBounds = std::make_shared<const RectangleBounds>(3., 4.);
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  auto planeSurfaceObject =
      Surface::makeShared<PlaneSurface>(pTransform, rBounds);
  auto planeSurfaceObject2 =
      Surface::makeShared<PlaneSurface>(pTransform, rBounds);
  //
  /// Test equality operator
  BOOST_CHECK(*planeSurfaceObject == *planeSurfaceObject2);
  //
  BOOST_TEST_CHECKPOINT(
      "Create and then assign a PlaneSurface object to the existing one");
  /// Test assignment
  auto assignedPlaneSurface =
      Surface::makeShared<PlaneSurface>(Transform3::Identity(), nullptr);
  *assignedPlaneSurface = *planeSurfaceObject;
  /// Test equality of assigned to original
  BOOST_CHECK(*assignedPlaneSurface == *planeSurfaceObject);
}

/// Unit test for testing PlaneSurface extent via Polyhedron representation
BOOST_AUTO_TEST_CASE(PlaneSurfaceExtent) {
  // First test - non-rotated
  static const Transform3 planeZX = AngleAxis3(-0.5 * M_PI, Vector3::UnitX()) *
                                    AngleAxis3(-0.5 * M_PI, Vector3::UnitZ()) *
                                    Transform3::Identity();

  double rHx = 2.;
  double rHy = 4.;
  double yPs = 3.;
  auto rBounds = std::make_shared<RectangleBounds>(rHx, rHy);

  auto plane = Surface::makeShared<PlaneSurface>(
      Transform3(Translation3(Vector3(0., yPs, 0.)) * planeZX), rBounds);

  auto planeExtent = plane->polyhedronRepresentation(tgContext, 1).extent();

  CHECK_CLOSE_ABS(planeExtent.min(BinningValue::binZ), -rHx,
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtent.max(BinningValue::binZ), rHx,
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtent.min(BinningValue::binX), -rHy,
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtent.max(BinningValue::binX), rHy,
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtent.min(BinningValue::binY), yPs,
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtent.max(BinningValue::binY), yPs,
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtent.min(BinningValue::binR), yPs,
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtent.max(BinningValue::binR), std::hypot(yPs, rHy),
                  s_onSurfaceTolerance);

  // Now rotate
  double alpha = 0.123;
  auto planeRot = Surface::makeShared<PlaneSurface>(
      Transform3(Translation3(Vector3(0., yPs, 0.)) *
                 AngleAxis3(alpha, Vector3(0., 0., 1.)) * planeZX),
      rBounds);

  auto planeExtentRot =
      planeRot->polyhedronRepresentation(tgContext, 1).extent();
  CHECK_CLOSE_ABS(planeExtentRot.min(BinningValue::binZ), -rHx,
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtentRot.max(BinningValue::binZ), rHx,
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtentRot.min(BinningValue::binX),
                  -rHy * std::cos(alpha), s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtentRot.max(BinningValue::binX), rHy * std::cos(alpha),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtentRot.min(BinningValue::binY),
                  yPs - rHy * std::sin(alpha), s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtentRot.max(BinningValue::binY),
                  yPs + rHy * std::sin(alpha), s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(planeExtentRot.min(BinningValue::binR), yPs * std::cos(alpha),
                  s_onSurfaceTolerance);
}

BOOST_AUTO_TEST_CASE(RotatedTrapezoid) {
  double shortHalfX{100.};
  double longHalfX{200.};
  double halfY{300.};
  double rotAngle{45._degree};

  Vector2 edgePoint{longHalfX - 10., halfY};

  std::shared_ptr<TrapezoidBounds> bounds =
      std::make_shared<TrapezoidBounds>(shortHalfX, longHalfX, halfY);

  BOOST_CHECK(bounds->inside(edgePoint, BoundaryTolerance::None()));
  BOOST_CHECK(!bounds->inside(Eigen::Rotation2D(-rotAngle) * edgePoint,
                              BoundaryTolerance::None()));

  std::shared_ptr<TrapezoidBounds> rotatedBounds =
      std::make_shared<TrapezoidBounds>(shortHalfX, longHalfX, halfY, rotAngle);

  BOOST_CHECK(!rotatedBounds->inside(edgePoint, BoundaryTolerance::None()));
  BOOST_CHECK(rotatedBounds->inside(Eigen::Rotation2D(-rotAngle) * edgePoint,
                                    BoundaryTolerance::None()));
}

/// Unit test for testing PlaneSurface alignment derivatives
BOOST_AUTO_TEST_CASE(PlaneSurfaceAlignment) {
  // bounds object, rectangle type
  auto rBounds = std::make_shared<const RectangleBounds>(3., 4.);
  // Test clone method
  Translation3 translation{0., 1., 2.};
  double rotationAngle = M_PI_2;
  AngleAxis3 rotation(rotationAngle, Vector3::UnitY());
  RotationMatrix3 rotationMat = rotation.toRotationMatrix();

  auto pTransform = Transform3{translation * rotationMat};
  auto planeSurfaceObject =
      Surface::makeShared<PlaneSurface>(pTransform, rBounds);

  // The local frame z axis
  const Vector3 localZAxis = rotationMat.col(2);
  // Check the local z axis is aligned to global x axis
  CHECK_CLOSE_ABS(localZAxis, Vector3(1., 0., 0.), 1e-15);

  // Define the track (local) position and direction
  Vector2 localPosition{1, 2};
  Vector3 momentum{1, 0, 0};
  Vector3 direction = momentum.normalized();
  // Get the global position
  Vector3 globalPosition =
      planeSurfaceObject->localToGlobal(tgContext, localPosition, momentum);

  // (a) Test the derivative of path length w.r.t. alignment parameters
  const AlignmentToPathMatrix& alignToPath =
      planeSurfaceObject->alignmentToPathDerivative(tgContext, globalPosition,
                                                    direction);
  // The expected results
  AlignmentToPathMatrix expAlignToPath = AlignmentToPathMatrix::Zero();
  expAlignToPath << 1, 0, 0, 2, -1, 0;

  // Check if the calculated derivative is as expected
  CHECK_CLOSE_ABS(alignToPath, expAlignToPath, 1e-10);

  // (b) Test the derivative of bound track parameters local position w.r.t.
  // position in local 3D Cartesian coordinates
  const auto& loc3DToLocBound =
      planeSurfaceObject->localCartesianToBoundLocalDerivative(tgContext,
                                                               globalPosition);
  // For plane surface, this should be identity matrix
  CHECK_CLOSE_ABS(loc3DToLocBound, (ActsMatrix<2, 3>::Identity()), 1e-10);

  // (c) Test the derivative of bound parameters (only test loc0, loc1 here)
  // w.r.t. alignment parameters
  FreeVector derivatives = FreeVector::Zero();
  derivatives.head<3>() = direction;
  const AlignmentToBoundMatrix& alignToBound =
      planeSurfaceObject->alignmentToBoundDerivative(tgContext, globalPosition,
                                                     direction, derivatives);
  const AlignmentToPathMatrix alignToloc0 =
      alignToBound.block<1, 6>(eBoundLoc0, eAlignmentCenter0);
  const AlignmentToPathMatrix alignToloc1 =
      alignToBound.block<1, 6>(eBoundLoc1, eAlignmentCenter0);
  // The expected results
  AlignmentToPathMatrix expAlignToloc0;
  expAlignToloc0 << 0, 0, 1, 0, 0, 2;
  AlignmentToPathMatrix expAlignToloc1;
  expAlignToloc1 << 0, -1, 0, 0, 0, -1;
  // Check if the calculated derivatives are as expected
  CHECK_CLOSE_ABS(alignToloc0, expAlignToloc0, 1e-10);
  CHECK_CLOSE_ABS(alignToloc1, expAlignToloc1, 1e-10);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
