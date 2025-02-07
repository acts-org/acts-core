// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/data/test_case.hpp>
#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Alignment.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/SurfaceMergingException.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <cmath>
#include <memory>
#include <numbers>
#include <ostream>
#include <string>

using namespace Acts::UnitLiterals;

namespace Acts::Test {
// Create a test context
GeometryContext tgContext = GeometryContext();
auto logger = Acts::getDefaultLogger("UnitTests", Acts::Logging::VERBOSE);

BOOST_AUTO_TEST_SUITE(Surfaces)
/// Unit tests for creating DiscSurface object
BOOST_AUTO_TEST_CASE(DiscSurfaceConstruction) {
  /// Test default construction
  // default construction is deleted

  const double rMin = 1.;
  const double rMax = 5.;
  const double halfPhiSector = std::numbers::pi / 8.;

  /// Test DiscSurface constructor with default halfPhiSector
  BOOST_CHECK_NO_THROW(
      Surface::makeShared<DiscSurface>(Transform3::Identity(), rMin, rMax));

  /// Test DiscSurface constructor with a transform specified
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  BOOST_CHECK_NO_THROW(
      Surface::makeShared<DiscSurface>(pTransform, rMin, rMax, halfPhiSector));

  /// Copy constructed DiscSurface
  auto anotherDiscSurface =
      Surface::makeShared<DiscSurface>(pTransform, rMin, rMax, halfPhiSector);
  // N.B. Just using
  // BOOST_CHECK_NO_THROW(Surface::makeShared<DiscSurface>(anotherDiscSurface))
  // tries to call the (deleted) default constructor.
  auto copiedSurface = Surface::makeShared<DiscSurface>(*anotherDiscSurface);
  BOOST_TEST_MESSAGE("Copy constructed DiscSurface ok");

  /// Copied and transformed DiscSurface
  BOOST_CHECK_NO_THROW(Surface::makeShared<DiscSurface>(
      tgContext, *anotherDiscSurface, pTransform));

  /// Construct with nullptr bounds
  DetectorElementStub detElem;
  BOOST_CHECK_THROW(
      auto nullBounds = Surface::makeShared<DiscSurface>(nullptr, detElem),
      AssertionFailureException);
}

/// Unit tests of all named methods
BOOST_AUTO_TEST_CASE(DiscSurfaceProperties) {
  const double rMin = 1.;
  const double rMax = 5.;
  const double halfPhiSector = std::numbers::pi / 8.;

  const Vector3 origin3D{0, 0, 0};

  auto discSurfaceObject = Surface::makeShared<DiscSurface>(
      Transform3::Identity(), rMin, rMax, halfPhiSector);

  /// Test type
  BOOST_CHECK_EQUAL(discSurfaceObject->type(), Surface::Disc);

  /// Test normal, no local position specified
  Vector3 zAxis{0, 0, 1};
  BOOST_CHECK_EQUAL(discSurfaceObject->normal(tgContext), zAxis);

  /// Test normal, local position specified
  Vector2 lpos(2., 0.05);
  BOOST_CHECK_EQUAL(discSurfaceObject->normal(tgContext, lpos), zAxis);

  /// Test referencePosition
  BOOST_CHECK_EQUAL(
      discSurfaceObject->referencePosition(tgContext, AxisDirection::AxisRPhi),
      origin3D);

  /// Test bounds
  BOOST_CHECK_EQUAL(discSurfaceObject->bounds().type(), SurfaceBounds::eDisc);

  Vector3 ignoredMomentum{0., 0., 0.};
  /// Test isOnSurface()
  Vector3 point3DNotInSector{0., 1.2, 0};
  Vector3 point3DOnSurface{1.2, 0., 0};
  BOOST_CHECK(!discSurfaceObject->isOnSurface(tgContext, point3DNotInSector,
                                              ignoredMomentum,
                                              BoundaryTolerance::None()));
  BOOST_CHECK(!discSurfaceObject->isOnSurface(tgContext, point3DNotInSector,
                                              BoundaryTolerance::None()));
  BOOST_CHECK(discSurfaceObject->isOnSurface(
      tgContext, point3DOnSurface, ignoredMomentum, BoundaryTolerance::None()));
  BOOST_CHECK(discSurfaceObject->isOnSurface(tgContext, point3DOnSurface,
                                             BoundaryTolerance::None()));

  /// Test localToGlobal
  Vector3 returnedPosition{10.9, 8.7, 6.5};
  Vector3 expectedPosition{1.2, 0, 0};
  Vector2 rPhiOnDisc{1.2, 0.};
  Vector2 rPhiNotInSector{
      1.2, std::numbers::pi};  // outside sector at Phi=0, +/- pi/8
  returnedPosition =
      discSurfaceObject->localToGlobal(tgContext, rPhiOnDisc, ignoredMomentum);
  CHECK_CLOSE_ABS(returnedPosition, expectedPosition, 1e-6);

  returnedPosition = discSurfaceObject->localToGlobal(
      tgContext, rPhiNotInSector, ignoredMomentum);
  Vector3 expectedNonPosition{-1.2, 0, 0};
  CHECK_CLOSE_ABS(returnedPosition, expectedNonPosition, 1e-6);

  /// Test globalToLocal
  Vector2 returnedLocalPosition{33., 44.};
  Vector2 expectedLocalPosition{1.2, 0.};
  returnedLocalPosition =
      discSurfaceObject
          ->globalToLocal(tgContext, point3DOnSurface, ignoredMomentum)
          .value();
  CHECK_CLOSE_ABS(returnedLocalPosition, expectedLocalPosition, 1e-6);

  // Global to local does not check inside bounds
  returnedLocalPosition =
      discSurfaceObject
          ->globalToLocal(tgContext, point3DNotInSector, ignoredMomentum)
          .value();

  Vector3 pointOutsideR{0., 100., 0};
  returnedLocalPosition =
      discSurfaceObject
          ->globalToLocal(tgContext, pointOutsideR, ignoredMomentum)
          .value();

  /// Test localPolarToCartesian
  Vector2 rPhi1_1{std::numbers::sqrt2, std::numbers::pi / 4.};
  Vector2 cartesian1_1{1., 1.};
  CHECK_CLOSE_REL(discSurfaceObject->localPolarToCartesian(rPhi1_1),
                  cartesian1_1, 1e-6);

  /// Test localCartesianToPolar
  CHECK_CLOSE_REL(discSurfaceObject->localCartesianToPolar(cartesian1_1),
                  rPhi1_1, 1e-6);

  /// Test localPolarToLocalCartesian
  CHECK_CLOSE_REL(discSurfaceObject->localPolarToLocalCartesian(rPhi1_1),
                  cartesian1_1, 1e-6);

  /// Test localCartesianToGlobal
  Vector3 cartesian3D1_1{1., 1., 0.};
  CHECK_CLOSE_ABS(
      discSurfaceObject->localCartesianToGlobal(tgContext, cartesian1_1),
      cartesian3D1_1, 1e-6);

  /// Test globalToLocalCartesian
  CHECK_CLOSE_REL(
      discSurfaceObject->globalToLocalCartesian(tgContext, cartesian3D1_1),
      cartesian1_1, 1e-6);

  /// Test pathCorrection
  double projected3DMomentum = std::numbers::sqrt3 * 1.e6;
  Vector3 momentum{projected3DMomentum, projected3DMomentum,
                   projected3DMomentum};
  Vector3 ignoredPosition = discSurfaceObject->center(tgContext);
  CHECK_CLOSE_REL(discSurfaceObject->pathCorrection(tgContext, ignoredPosition,
                                                    momentum.normalized()),
                  std::numbers::sqrt3, 0.01);

  /// intersection test
  Vector3 globalPosition{1.2, 0., -10.};
  Vector3 direction{0., 0., 1.};  // must be normalised
  Vector3 expected{1.2, 0., 0.};

  // intersect is a struct of (Vector3) position, pathLength, distance and
  // (bool) valid, it's contained in a Surface intersection
  auto sfIntersection = discSurfaceObject
                            ->intersect(tgContext, globalPosition, direction,
                                        BoundaryTolerance::Infinite())
                            .closest();
  Intersection3D expectedIntersect{Vector3{1.2, 0., 0.}, 10.,
                                   IntersectionStatus::reachable};
  BOOST_CHECK(sfIntersection.isValid());
  CHECK_CLOSE_ABS(sfIntersection.position(), expectedIntersect.position(),
                  1e-9);
  CHECK_CLOSE_ABS(sfIntersection.pathLength(), expectedIntersect.pathLength(),
                  1e-9);
  BOOST_CHECK_EQUAL(sfIntersection.object(), discSurfaceObject.get());

  /// Test name
  boost::test_tools::output_test_stream nameOuput;
  nameOuput << discSurfaceObject->name();
  BOOST_CHECK(nameOuput.is_equal("Acts::DiscSurface"));
}

/// Unit test for testing DiscSurface assignment and equality
BOOST_AUTO_TEST_CASE(DiscSurfaceAssignment) {
  const double rMin = 1.;
  const double rMax = 5.;
  const double halfPhiSector = std::numbers::pi / 8.;

  auto discSurfaceObject = Surface::makeShared<DiscSurface>(
      Transform3::Identity(), rMin, rMax, halfPhiSector);
  auto assignedDisc =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 2.2, 4.4, 0.07);

  BOOST_CHECK_NO_THROW(*assignedDisc = *discSurfaceObject);
  BOOST_CHECK((*assignedDisc) == (*discSurfaceObject));
}

/// Unit test for testing DiscSurface assignment and equality
BOOST_AUTO_TEST_CASE(DiscSurfaceExtent) {
  const double rMin = 1.;
  const double rMax = 5.;

  auto pDisc =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), 0., rMax);
  auto pDiscExtent = pDisc->polyhedronRepresentation(tgContext, 1).extent();

  CHECK_CLOSE_ABS(0., pDiscExtent.min(AxisDirection::AxisZ),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(0., pDiscExtent.max(AxisDirection::AxisZ),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(0., pDiscExtent.min(AxisDirection::AxisR),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(rMax, pDiscExtent.max(AxisDirection::AxisR),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(-rMax, pDiscExtent.min(AxisDirection::AxisX),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(rMax, pDiscExtent.max(AxisDirection::AxisX),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(-rMax, pDiscExtent.min(AxisDirection::AxisY),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(rMax, pDiscExtent.max(AxisDirection::AxisY),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(-std::numbers::pi, pDiscExtent.min(AxisDirection::AxisPhi),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(std::numbers::pi, pDiscExtent.max(AxisDirection::AxisPhi),
                  s_onSurfaceTolerance);

  auto pRing =
      Surface::makeShared<DiscSurface>(Transform3::Identity(), rMin, rMax);
  auto pRingExtent = pRing->polyhedronRepresentation(tgContext, 1).extent();

  CHECK_CLOSE_ABS(0., pRingExtent.min(AxisDirection::AxisZ),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(0., pRingExtent.max(AxisDirection::AxisZ),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(rMin, pRingExtent.min(AxisDirection::AxisR),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(rMax, pRingExtent.max(AxisDirection::AxisR),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(-rMax, pRingExtent.min(AxisDirection::AxisX),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(rMax, pRingExtent.max(AxisDirection::AxisX),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(-rMax, pRingExtent.min(AxisDirection::AxisY),
                  s_onSurfaceTolerance);
  CHECK_CLOSE_ABS(rMax, pRingExtent.max(AxisDirection::AxisY),
                  s_onSurfaceTolerance);
}

/// Unit test for testing DiscSurface alignment derivatives
BOOST_AUTO_TEST_CASE(DiscSurfaceAlignment) {
  Translation3 translation{0., 1., 2.};
  Transform3 transform(translation);
  const double rMin = 1.;
  const double rMax = 5.;
  const double halfPhiSector = std::numbers::pi / 8.;

  auto discSurfaceObject =
      Surface::makeShared<DiscSurface>(transform, rMin, rMax, halfPhiSector);

  const auto& rotation = transform.rotation();
  // The local frame z axis
  const Vector3 localZAxis = rotation.col(2);
  // Check the local z axis is aligned to global z axis
  CHECK_CLOSE_ABS(localZAxis, Vector3(0., 0., 1.), 1e-15);

  // Define the track (global) position and direction
  Vector3 globalPosition{0, 4, 2};
  Vector3 momentum{0, 0, 1};
  Vector3 direction = momentum.normalized();

  // (a) Test the derivative of path length w.r.t. alignment parameters
  const AlignmentToPathMatrix& alignToPath =
      discSurfaceObject->alignmentToPathDerivative(tgContext, globalPosition,
                                                   direction);
  // The expected results
  AlignmentToPathMatrix expAlignToPath = AlignmentToPathMatrix::Zero();
  expAlignToPath << 0, 0, 1, 3, 0, 0;
  // Check if the calculated derivative is as expected
  CHECK_CLOSE_ABS(alignToPath, expAlignToPath, 1e-10);

  // (b) Test the derivative of bound track parameters local position w.r.t.
  // position in local 3D Cartesian coordinates
  const auto& loc3DToLocBound =
      discSurfaceObject->localCartesianToBoundLocalDerivative(tgContext,
                                                              globalPosition);
  // Check if the result is as expected
  ActsMatrix<2, 3> expLoc3DToLocBound = ActsMatrix<2, 3>::Zero();
  expLoc3DToLocBound << 0, 1, 0, -1. / 3, 0, 0;
  CHECK_CLOSE_ABS(loc3DToLocBound, expLoc3DToLocBound, 1e-10);
}

BOOST_AUTO_TEST_CASE(DiscSurfaceBinningPosition) {
  using namespace Acts::UnitLiterals;
  Vector3 s{5_mm, 7_mm, 10_cm};
  Transform3 trf;
  trf = Translation3(s) * AngleAxis3{0.5, Vector3::UnitZ()};

  double minR = 300;
  double maxR = 330;

  {
    // Radial Bounds
    auto bounds =
        std::make_shared<RadialBounds>(minR, maxR, std::numbers::pi / 8, 0.1);
    auto disc = Acts::Surface::makeShared<Acts::DiscSurface>(trf, bounds);

    Vector3 bp = disc->referencePosition(tgContext, AxisDirection::AxisR);
    double r = (bounds->rMax() + bounds->rMin()) / 2.0;
    double phi = bounds->get(RadialBounds::eAveragePhi);
    Vector3 exp = Vector3{r * std::cos(phi), r * std::sin(phi), 0};
    exp = trf * exp;

    BOOST_CHECK_EQUAL(bp, exp);
    BOOST_CHECK_EQUAL(
        disc->referencePositionValue(tgContext, AxisDirection::AxisR),
        VectorHelpers::perp(exp));

    bp = disc->referencePosition(tgContext, AxisDirection::AxisPhi);
    BOOST_CHECK_EQUAL(bp, exp);
    BOOST_CHECK_EQUAL(
        disc->referencePositionValue(tgContext, AxisDirection::AxisPhi),
        VectorHelpers::phi(exp));

    for (auto b :
         {AxisDirection::AxisX, AxisDirection::AxisY, AxisDirection::AxisZ,
          AxisDirection::AxisEta, AxisDirection::AxisRPhi,
          AxisDirection::AxisTheta, AxisDirection::AxisMag}) {
      BOOST_TEST_CONTEXT("binValue: " << b) {
        BOOST_CHECK_EQUAL(disc->referencePosition(tgContext, b),
                          disc->center(tgContext));
      }
    }
  }

  {
    // Annulus Bounds
    double minPhiRel = -0.3;
    double maxPhiRel = 0.2;
    Vector2 origin{5_mm, 5_mm};
    auto bounds = std::make_shared<AnnulusBounds>(minR, maxR, minPhiRel,
                                                  maxPhiRel, origin);

    auto disc = Acts::Surface::makeShared<Acts::DiscSurface>(trf, bounds);

    Vector3 bp = disc->referencePosition(tgContext, AxisDirection::AxisR);
    double r = (bounds->rMax() + bounds->rMin()) / 2.0;
    double phi = bounds->get(AnnulusBounds::eAveragePhi);
    Vector3 exp = Vector3{r * std::cos(phi), r * std::sin(phi), 0};
    exp = trf * exp;

    BOOST_CHECK_EQUAL(bp, exp);

    bp = disc->referencePosition(tgContext, AxisDirection::AxisPhi);
    BOOST_CHECK_EQUAL(bp, exp);

    for (auto b :
         {AxisDirection::AxisX, AxisDirection::AxisY, AxisDirection::AxisZ,
          AxisDirection::AxisEta, AxisDirection::AxisRPhi,
          AxisDirection::AxisTheta, AxisDirection::AxisMag}) {
      BOOST_TEST_CONTEXT("binValue: " << b) {
        BOOST_CHECK_EQUAL(disc->referencePosition(tgContext, b),
                          disc->center(tgContext));
      }
    }
  }
}

BOOST_AUTO_TEST_SUITE(DiscSurfaceMerging)

namespace {
std::shared_ptr<DiscSurface> makeDisc(const Transform3& transform, double rMin,
                                      double rMax,
                                      double halfPhi = std::numbers::pi,
                                      double avgPhi = 0) {
  return Surface::makeShared<DiscSurface>(
      transform, std::make_shared<RadialBounds>(rMin, rMax, halfPhi, avgPhi));
}

}  // namespace

BOOST_AUTO_TEST_CASE(IncompatibleBounds) {
  Logging::ScopedFailureThreshold ft{Logging::FATAL};
  Transform3 base = Transform3::Identity();
  auto discRadial = makeDisc(base, 30_mm, 100_mm);
  auto discTrap =
      Surface::makeShared<DiscSurface>(base, 20_mm, 40_mm, 100_mm, 150_mm);
  auto discTrap2 =
      Surface::makeShared<DiscSurface>(base, 20_mm, 40_mm, 30_mm, 100_mm);

  BOOST_CHECK_THROW(
      discRadial->mergedWith(*discTrap, AxisDirection::AxisR, false, *logger),

      SurfaceMergingException);

  BOOST_CHECK_THROW(
      discTrap2->mergedWith(*discTrap, AxisDirection::AxisR, false, *logger),
      SurfaceMergingException);
}

BOOST_AUTO_TEST_CASE(InvalidDetectorElement) {
  DetectorElementStub detElem;

  auto bounds1 = std::make_shared<RadialBounds>(30_mm, 100_mm);
  auto disc1 = Surface::makeShared<DiscSurface>(bounds1, detElem);

  auto bounds2 = std::make_shared<RadialBounds>(100_mm, 150_mm);
  auto disc2 = Surface::makeShared<DiscSurface>(bounds2, detElem);

  BOOST_CHECK_THROW(
      disc1->mergedWith(*disc2, AxisDirection::AxisR, false, *logger),
      SurfaceMergingException);
}

BOOST_DATA_TEST_CASE(IncompatibleRDirection,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm})),
                     angle, offset) {
  Logging::ScopedFailureThreshold ft{Logging::FATAL};

  Transform3 base =
      AngleAxis3(angle * 1_degree, Vector3::UnitX()) * Translation3(offset);

  auto disc = makeDisc(base, 30_mm, 100_mm);

  // Disc with overlap in r
  auto discOverlap = makeDisc(base, 90_mm, 150_mm);
  BOOST_CHECK_THROW(disc->mergedWith(*discOverlap, Acts::AxisDirection::AxisR,
                                     false, *logger),
                    SurfaceMergingException);

  // Disc with gap in r
  auto discGap = makeDisc(base, 110_mm, 150_mm);
  BOOST_CHECK_THROW(
      disc->mergedWith(*discGap, Acts::AxisDirection::AxisR, false, *logger),
      SurfaceMergingException);

  auto discShiftedZ = Surface::makeShared<DiscSurface>(
      base * Translation3{Vector3::UnitZ() * 10_mm}, 100_mm, 150_mm);
  BOOST_CHECK_THROW(disc->mergedWith(*discShiftedZ, Acts::AxisDirection::AxisR,
                                     false, *logger),
                    SurfaceMergingException);

  auto discShiftedXy = makeDisc(
      base * Translation3{Vector3{1_mm, 2_mm, 200_mm}}, 100_mm, 150_mm);
  BOOST_CHECK_THROW(disc->mergedWith(*discShiftedXy, Acts::AxisDirection::AxisZ,
                                     false, *logger),
                    SurfaceMergingException);

  auto discRotatedZ = makeDisc(base * AngleAxis3{10_degree, Vector3::UnitZ()},
                               100_mm, 150_mm, 60_degree, 0_degree);
  BOOST_CHECK_THROW(disc->mergedWith(*discRotatedZ, Acts::AxisDirection::AxisR,
                                     false, *logger),
                    SurfaceMergingException);

  auto discRotatedX =
      makeDisc(base * AngleAxis3{10_degree, Vector3::UnitX()}, 100_mm, 150_mm);
  BOOST_CHECK_THROW(disc->mergedWith(*discRotatedX, Acts::AxisDirection::AxisR,
                                     false, *logger),
                    SurfaceMergingException);

  // Test not same phi sector
  auto discPhi1 = makeDisc(base, 30_mm, 100_mm, 10_degree, 40_degree);
  auto discPhi2 = makeDisc(base, 100_mm, 160_mm, 20_degree, 40_degree);
  auto discPhi3 = makeDisc(base, 100_mm, 160_mm, 10_degree, 50_degree);
  BOOST_CHECK_THROW(
      discPhi1->mergedWith(*discPhi2, AxisDirection::AxisR, false, *logger),
      SurfaceMergingException);

  BOOST_CHECK_THROW(
      discPhi1->mergedWith(*discPhi3, AxisDirection::AxisR, false, *logger),
      SurfaceMergingException);
}

BOOST_DATA_TEST_CASE(RDirection,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm})),
                     angle, offset) {
  Transform3 base =
      AngleAxis3(angle * 1_degree, Vector3::UnitX()) * Translation3(offset);

  auto disc = makeDisc(base, 30_mm, 100_mm);

  auto disc2 =
      makeDisc(base * AngleAxis3(14_degree, Vector3::UnitZ()), 100_mm, 150_mm);

  auto [disc3, reversed] =
      disc->mergedWith(*disc2, Acts::AxisDirection::AxisR, false, *logger);
  BOOST_REQUIRE_NE(disc3, nullptr);
  BOOST_CHECK(!reversed);

  auto [disc3Reversed, reversed2] =
      disc2->mergedWith(*disc, Acts::AxisDirection::AxisR, false, *logger);
  BOOST_REQUIRE_NE(disc3Reversed, nullptr);
  BOOST_CHECK(disc3->bounds() == disc3Reversed->bounds());
  BOOST_CHECK(reversed2);

  const auto* bounds = dynamic_cast<const RadialBounds*>(&disc3->bounds());
  BOOST_REQUIRE_NE(bounds, nullptr);

  BOOST_CHECK_EQUAL(bounds->get(RadialBounds::eMinR), 30_mm);
  BOOST_CHECK_EQUAL(bounds->get(RadialBounds::eMaxR), 150_mm);

  // Disc did not move
  BOOST_CHECK_EQUAL(base.matrix(), disc3->transform(tgContext).matrix());

  // Rotation in z depends on the ordering, the left side "wins"
  Transform3 expected12 = base;
  BOOST_CHECK_EQUAL(expected12.matrix(), disc3->transform(tgContext).matrix());

  Transform3 expected21 = base * AngleAxis3(14_degree, Vector3::UnitZ());
  CHECK_CLOSE_OR_SMALL(disc3Reversed->transform(tgContext).matrix(),
                       expected21.matrix(), 1e-6, 1e-10);

  // Test r merging with phi sectors (matching)
  auto discPhi1 = makeDisc(base, 30_mm, 100_mm, 10_degree, 40_degree);
  auto discPhi2 = makeDisc(base, 100_mm, 160_mm, 10_degree, 40_degree);
  auto [discPhi12, reversedPhi12] =
      discPhi1->mergedWith(*discPhi2, AxisDirection::AxisR, false, *logger);
  BOOST_REQUIRE_NE(discPhi12, nullptr);

  const auto* boundsPhi12 =
      dynamic_cast<const RadialBounds*>(&discPhi12->bounds());
  BOOST_REQUIRE_NE(boundsPhi12, nullptr);

  BOOST_CHECK_EQUAL(boundsPhi12->get(RadialBounds::eMinR), 30_mm);
  BOOST_CHECK_EQUAL(boundsPhi12->get(RadialBounds::eMaxR), 160_mm);
  BOOST_CHECK_EQUAL(boundsPhi12->get(RadialBounds::eHalfPhiSector), 10_degree);
}

BOOST_DATA_TEST_CASE(IncompatiblePhiDirection,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::xrange(-1300, 1300, 104)),
                     angle, offset, phiShift) {
  Logging::ScopedFailureThreshold ft{Logging::FATAL};
  Transform3 base =
      AngleAxis3(angle * 1_degree, Vector3::UnitX()) * Translation3(offset);

  auto a = [phiShift](double v) {
    return detail::radian_sym(v + phiShift * 1_degree);
  };

  auto discPhi = makeDisc(base, 30_mm, 100_mm, 10_degree, a(40_degree));

  // Disc with overlap in phi
  auto discPhi2 = makeDisc(base, 30_mm, 100_mm, 45_degree, a(85_degree));
  BOOST_CHECK_THROW(discPhi->mergedWith(*discPhi2, Acts::AxisDirection::AxisPhi,
                                        false, *logger),
                    SurfaceMergingException);

  // Disc with gap in phi
  auto discPhi3 = makeDisc(base, 30_mm, 100_mm, 45_degree, a(105_degree));
  BOOST_CHECK_THROW(discPhi->mergedWith(*discPhi3, Acts::AxisDirection::AxisPhi,
                                        false, *logger),
                    SurfaceMergingException);

  // Disc with a z shift
  auto discPhi4 = makeDisc(base * Translation3{Vector3::UnitZ() * 20_mm}, 30_mm,
                           100_mm, 45_degree, a(95_degree));
  BOOST_CHECK_THROW(discPhi->mergedWith(*discPhi4, Acts::AxisDirection::AxisPhi,
                                        false, *logger),
                    SurfaceMergingException);

  // Disc with different r bounds: could be merged in r but not in phi
  auto discPhi5 = makeDisc(base, 100_mm, 150_mm, 45_degree, a(95_degree));
  BOOST_CHECK_THROW(discPhi->mergedWith(*discPhi5, Acts::AxisDirection::AxisPhi,
                                        false, *logger),
                    SurfaceMergingException);
}

BOOST_DATA_TEST_CASE(PhiDirection,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::xrange(-1300, 1300, 104)),
                     angle, offset, phiShift) {
  Transform3 base =
      AngleAxis3(angle * 1_degree, Vector3::UnitX()) * Translation3(offset);

  auto a = [phiShift](double v) {
    return detail::radian_sym(v + phiShift * 1_degree);
  };

  BOOST_TEST_CONTEXT("Internal rotation") {
    auto disc = makeDisc(base, 30_mm, 100_mm, 10_degree, a(40_degree));
    auto disc2 = makeDisc(base, 30_mm, 100_mm, 45_degree, a(95_degree));

    auto [disc3, reversed] =
        disc->mergedWith(*disc2, Acts::AxisDirection::AxisPhi, false, *logger);
    BOOST_REQUIRE_NE(disc3, nullptr);
    BOOST_CHECK_EQUAL(base.matrix(), disc3->transform(tgContext).matrix());
    BOOST_CHECK(reversed);

    auto [disc3Reversed, reversed2] =
        disc2->mergedWith(*disc, Acts::AxisDirection::AxisPhi, false, *logger);
    BOOST_REQUIRE_NE(disc3Reversed, nullptr);
    BOOST_CHECK(*disc3 == *disc3Reversed);
    BOOST_CHECK(!reversed2);

    const auto* bounds = dynamic_cast<const RadialBounds*>(&disc3->bounds());
    BOOST_REQUIRE_NE(bounds, nullptr);

    BOOST_CHECK_SMALL(
        detail::difference_periodic(bounds->get(RadialBounds::eAveragePhi),
                                    a(85_degree), 2 * std::numbers::pi),
        1e-6);
    BOOST_CHECK_CLOSE(bounds->get(RadialBounds::eHalfPhiSector), 55_degree,
                      1e-6);

    auto disc4 = makeDisc(base, 30_mm, 100_mm, 20_degree, a(170_degree));
    auto disc5 = makeDisc(base, 30_mm, 100_mm, 10_degree, a(-160_degree));
    auto [disc45, reversed45] =
        disc4->mergedWith(*disc5, Acts::AxisDirection::AxisPhi, false, *logger);
    BOOST_REQUIRE_NE(disc45, nullptr);
    BOOST_CHECK_EQUAL(base.matrix(), disc45->transform(tgContext).matrix());
    BOOST_CHECK(reversed45);

    auto [disc54, reversed54] =
        disc5->mergedWith(*disc4, Acts::AxisDirection::AxisPhi, false, *logger);
    BOOST_REQUIRE_NE(disc54, nullptr);
    BOOST_CHECK(!reversed54);

    BOOST_CHECK(*disc54 == *disc45);

    const auto* bounds45 = dynamic_cast<const RadialBounds*>(&disc45->bounds());
    BOOST_REQUIRE_NE(bounds, nullptr);

    BOOST_CHECK_SMALL(
        detail::difference_periodic(bounds45->get(RadialBounds::eAveragePhi),
                                    a(180_degree), 2 * std::numbers::pi),
        1e-6);
    BOOST_CHECK_CLOSE(bounds45->get(RadialBounds::eHalfPhiSector), 30_degree,
                      1e-6);

    auto disc6 = makeDisc(base, 30_mm, 100_mm, 90_degree, a(0_degree));
    auto disc7 = makeDisc(base, 30_mm, 100_mm, 90_degree, a(180_degree));

    auto [disc67, reversed67] =
        disc6->mergedWith(*disc7, Acts::AxisDirection::AxisPhi, false, *logger);
    BOOST_REQUIRE_NE(disc67, nullptr);
    CHECK_CLOSE_OR_SMALL(disc67->transform(tgContext).matrix(), base.matrix(),
                         1e-6, 1e-10);
    BOOST_CHECK(!reversed67);

    auto [disc76, reversed76] =
        disc7->mergedWith(*disc6, Acts::AxisDirection::AxisPhi, false, *logger);
    BOOST_REQUIRE_NE(disc76, nullptr);
    // surfaces are not equal because bounds are not equal
    BOOST_CHECK(*disc76 != *disc67);
    // bounds are different because of avg phi
    BOOST_CHECK_NE(disc76->bounds(), disc67->bounds());
    // transforms should be the same
    BOOST_CHECK_EQUAL(disc76->transform(tgContext).matrix(),
                      disc67->transform(tgContext).matrix());
    // not reversed either because you get the ordering you put in
    BOOST_CHECK(!reversed76);

    const auto* bounds67 = dynamic_cast<const RadialBounds*>(&disc67->bounds());
    BOOST_REQUIRE_NE(bounds67, nullptr);
    BOOST_CHECK_SMALL(
        detail::difference_periodic(bounds67->get(RadialBounds::eAveragePhi),
                                    a(90_degree), 2 * std::numbers::pi),
        1e-6);
    BOOST_CHECK_CLOSE(bounds67->get(RadialBounds::eHalfPhiSector), 180_degree,
                      1e-6);
  }

  BOOST_TEST_CONTEXT("External rotation") {
    Transform3 trf1 = base * AngleAxis3(a(40_degree), Vector3::UnitZ());
    auto disc = makeDisc(trf1, 30_mm, 100_mm, 10_degree, 0_degree);
    Transform3 trf2 = base * AngleAxis3(a(95_degree), Vector3::UnitZ());
    auto disc2 = makeDisc(trf2, 30_mm, 100_mm, 45_degree, 0_degree);

    auto [disc3, reversed] =
        disc->mergedWith(*disc2, Acts::AxisDirection::AxisPhi, true, *logger);
    BOOST_REQUIRE_NE(disc3, nullptr);
    Transform3 trfExpected12 =
        base * AngleAxis3(a(85_degree), Vector3::UnitZ());
    CHECK_CLOSE_OR_SMALL(disc3->transform(tgContext).matrix(),
                         trfExpected12.matrix(), 1e-6, 1e-10);
    BOOST_CHECK(reversed);

    auto [disc3Reversed, reversed2] =
        disc2->mergedWith(*disc, Acts::AxisDirection::AxisPhi, true, *logger);
    BOOST_REQUIRE_NE(disc3Reversed, nullptr);
    BOOST_CHECK(*disc3 == *disc3Reversed);
    BOOST_CHECK(!reversed2);

    const auto* bounds = dynamic_cast<const RadialBounds*>(&disc3->bounds());
    BOOST_REQUIRE_NE(bounds, nullptr);

    BOOST_CHECK_EQUAL(bounds->get(RadialBounds::eAveragePhi), 0);
    BOOST_CHECK_CLOSE(bounds->get(RadialBounds::eHalfPhiSector), 55_degree,
                      1e-6);

    Transform3 trf4 = base * AngleAxis3(a(170_degree), Vector3::UnitZ());
    auto disc4 = makeDisc(trf4, 30_mm, 100_mm, 20_degree, 0_degree);
    Transform3 trf5 = base * AngleAxis3(a(-160_degree), Vector3::UnitZ());
    auto disc5 = makeDisc(trf5, 30_mm, 100_mm, 10_degree, 0_degree);
    auto [disc45, reversed45] =
        disc4->mergedWith(*disc5, Acts::AxisDirection::AxisPhi, true, *logger);
    BOOST_REQUIRE_NE(disc45, nullptr);
    Transform3 trfExpected45 =
        base * AngleAxis3(a(180_degree), Vector3::UnitZ());
    CHECK_CLOSE_OR_SMALL(disc45->transform(tgContext).matrix(),
                         trfExpected45.matrix(), 1e-6, 1e-10);
    BOOST_CHECK(reversed45);

    auto [disc54, reversed54] =
        disc5->mergedWith(*disc4, Acts::AxisDirection::AxisPhi, true, *logger);
    BOOST_REQUIRE_NE(disc54, nullptr);
    BOOST_CHECK(!reversed54);

    BOOST_CHECK(*disc54 == *disc45);

    const auto* bounds45 = dynamic_cast<const RadialBounds*>(&disc45->bounds());
    BOOST_REQUIRE_NE(bounds, nullptr);

    BOOST_CHECK_EQUAL(bounds45->get(RadialBounds::eAveragePhi), 0);
    BOOST_CHECK_CLOSE(bounds45->get(RadialBounds::eHalfPhiSector), 30_degree,
                      1e-6);

    Transform3 trf6 = base * AngleAxis3(a(0_degree), Vector3::UnitZ());
    auto disc6 = makeDisc(trf6, 30_mm, 100_mm, 90_degree, 0_degree);
    Transform3 trf7 = base * AngleAxis3(a(180_degree), Vector3::UnitZ());
    auto disc7 = makeDisc(trf7, 30_mm, 100_mm, 90_degree, 0_degree);
    auto [disc67, reversed67] =
        disc6->mergedWith(*disc7, Acts::AxisDirection::AxisPhi, true, *logger);
    BOOST_REQUIRE_NE(disc67, nullptr);
    Transform3 trfExpected67 =
        base * AngleAxis3(a(90_degree), Vector3::UnitZ());
    CHECK_CLOSE_OR_SMALL(disc67->transform(tgContext).matrix(),
                         trfExpected67.matrix(), 1e-6, 1e-10);
    BOOST_CHECK(!reversed67);

    auto [disc76, reversed76] =
        disc7->mergedWith(*disc6, Acts::AxisDirection::AxisPhi, true, *logger);
    BOOST_REQUIRE_NE(disc76, nullptr);
    // surfaces are not equal due to different transforms
    BOOST_CHECK(*disc76 != *disc67);
    BOOST_CHECK_NE(disc76->transform(tgContext).matrix(),
                   disc67->transform(tgContext).matrix());
    // bounds should be equal however
    BOOST_CHECK_EQUAL(disc76->bounds(), disc67->bounds());

    // not reversed either because you get the ordering you put in
    BOOST_CHECK(!reversed76);

    const auto* bounds67 = dynamic_cast<const RadialBounds*>(&disc67->bounds());
    BOOST_REQUIRE_NE(bounds67, nullptr);
    BOOST_CHECK_EQUAL(bounds67->get(RadialBounds::eAveragePhi), 0);
    BOOST_CHECK_CLOSE(bounds67->get(RadialBounds::eHalfPhiSector), 180_degree,
                      1e-6);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
