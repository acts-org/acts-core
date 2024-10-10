// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Alignment.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/LineSurfaceStub.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace Acts {
class AssertionFailureException;
}  // namespace Acts

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();

BOOST_AUTO_TEST_SUITE(Surfaces)

/// Unit test for creating compliant/non-compliant LineSurface object
BOOST_AUTO_TEST_CASE(LineSurface_Constructors_test) {
  // Default ctor is deleted
  // LineSurfaceStub l;
  // ctor with translation, radius, halfz
  Translation3 translation{0., 1., 2.};
  Transform3 transform(translation);
  auto pTransform = Transform3(translation);
  const double radius{2.0}, halfz{20.};
  BOOST_CHECK(LineSurfaceStub(pTransform, radius, halfz).constructedOk());
  // ctor with nullptr for LineBounds
  BOOST_CHECK(LineSurfaceStub(pTransform).constructedOk());
  // ctor with LineBounds
  auto pLineBounds = std::make_shared<const LineBounds>(2., 10.0);
  BOOST_CHECK(LineSurfaceStub(pTransform, pLineBounds).constructedOk());
  // ctor with LineBounds, detector element, Identifier
  auto pMaterial =
      std::make_shared<const HomogeneousSurfaceMaterial>(makePercentSlab());
  DetectorElementStub detElement{pTransform, pLineBounds, 0.2, pMaterial};
  BOOST_CHECK(LineSurfaceStub(pLineBounds, detElement).constructedOk());
  LineSurfaceStub lineToCopy(pTransform, 2.0, 20.);
  // Copy ctor
  BOOST_CHECK(LineSurfaceStub(lineToCopy).constructedOk());
  // Copied and transformed ctor
  BOOST_CHECK(
      LineSurfaceStub(tgContext, lineToCopy, transform).constructedOk());

  /// Construct with nullptr bounds
  DetectorElementStub detElem;
  BOOST_CHECK_THROW(LineSurfaceStub nullBounds(nullptr, detElem),
                    AssertionFailureException);

  BOOST_TEST_MESSAGE(
      "All LineSurface constructors are callable without problem");
}

/// Unit tests of all named methods
BOOST_AUTO_TEST_CASE(LineSurface_allNamedMethods_test) {
  // binningPosition()
  Translation3 translation{0., 1., 2.};
  Transform3 transform(translation);
  LineSurfaceStub line(transform, 2.0, 20.);
  Vector3 referencePosition{0., 1., 2.};
  CHECK_CLOSE_ABS(referencePosition,
                  line.binningPosition(tgContext, BinningValue::binX), 1e-6);
  //
  // bounds()
  auto pLineBounds = std::make_shared<const LineBounds>(2., 10.0);
  LineSurfaceStub boundedLine(transform, pLineBounds);
  const LineBounds& bounds =
      dynamic_cast<const LineBounds&>(boundedLine.bounds());
  BOOST_CHECK_EQUAL(bounds, LineBounds(2., 10.0));
  //
  // globalToLocal()
  Vector3 gpos{0., 1., 0.};
  const Vector3 mom{20., 0., 0.};  // needs more realistic parameters
  Vector2 localPosition =
      line.globalToLocal(tgContext, gpos, mom.normalized()).value();
  const Vector2 expectedResult{0, -2};
  CHECK_CLOSE_ABS(expectedResult, localPosition, 1e-6);
  //
  // intersection
  {
    const Vector3 direction{0., 1., 2.};
    auto sfIntersection =
        line.intersect(tgContext, {0., 0., 0.}, direction.normalized(),
                       BoundaryTolerance::Infinite())
            .closest();
    BOOST_CHECK(sfIntersection.isValid());
    Vector3 expectedIntersection(0, 1., 2.);
    CHECK_CLOSE_ABS(sfIntersection.position(), expectedIntersection,
                    1e-6);  // need more tests..
    BOOST_CHECK_EQUAL(sfIntersection.object(), &line);
  }
  //
  // isOnSurface
  const Vector3 insidePosition{0., 2.5, 0.};
  BOOST_CHECK(line.isOnSurface(
      tgContext, insidePosition, mom,
      BoundaryTolerance::Infinite()));  // need better test here
  const Vector3 outsidePosition{100., 100., 200.};
  BOOST_CHECK(!line.isOnSurface(tgContext, outsidePosition, mom,
                                BoundaryTolerance::None()));
  //
  // localToGlobal
  Vector3 returnedGlobalPosition{0., 0., 0.};
  // Vector2 localPosition{0., 0.};
  const Vector3 momentum{300., 200., 0.};  // find better values!
  returnedGlobalPosition =
      line.localToGlobal(tgContext, localPosition, momentum.normalized());
  const Vector3 expectedGlobalPosition{0, 1, 0};
  CHECK_CLOSE_ABS(returnedGlobalPosition, expectedGlobalPosition, 1e-6);
  //
  // referenceFrame
  Vector3 globalPosition{0., 0., 0.};
  auto returnedRotationMatrix =
      line.referenceFrame(tgContext, globalPosition, momentum.normalized());
  double v0 = std::cos(std::atan(2. / 3.));
  double v1 = std::sin(std::atan(2. / 3.));
  RotationMatrix3 expectedRotationMatrix;
  expectedRotationMatrix << -v1, 0., v0, v0, 0., v1, 0., 1., -0.;
  // std::cout<<returnedRotationMatrix<<std::endl;
  // std::cout<<expectedRotationMatrix<<std::endl;
  CHECK_CLOSE_OR_SMALL(returnedRotationMatrix, expectedRotationMatrix, 1e-6,
                       1e-9);
  //
  // name()
  boost::test_tools::output_test_stream output;
  output << line.name();
  BOOST_CHECK(output.is_equal("Acts::LineSurface"));
  //
  // normal
  {
    Vector3 position{5, 5, 5};  // should be irrelevant
    Vector3 direction{1, 0, 0};
    CHECK_CLOSE_ABS(line.normal(tgContext, position, direction), direction,
                    1e-6);
  }
  {
    Vector3 position{5, 5, 5};  // should be irrelevant
    Vector3 direction = Vector3{1, 0, 0.1}.normalized();
    CHECK_CLOSE_ABS(line.normal(tgContext, position, direction),
                    Vector3::UnitX(), 1e-6);
  }
  {
    Vector3 position{5, 5, 5};  // should be irrelevant
    Vector3 direction{-1, 0, 0};
    CHECK_CLOSE_ABS(line.normal(tgContext, position, direction), direction,
                    1e-6);
  }
  {
    Vector3 position{5, 5, 5};  // should be irrelevant
    Vector3 direction{0, 1, 0};
    CHECK_CLOSE_ABS(line.normal(tgContext, position, direction), direction,
                    1e-6);
  }
  //
  // pathCorrection
  Vector3 any3DVector = Vector3::Random();
  CHECK_CLOSE_REL(line.pathCorrection(tgContext, any3DVector, any3DVector), 1.,
                  1e-6);
}

/// Unit test for testing LineSurface assignment
BOOST_AUTO_TEST_CASE(LineSurface_assignment_test) {
  Translation3 translation{0., 1., 2.};
  Transform3 transform(translation);
  LineSurfaceStub originalLine(transform, 2.0, 20.);
  LineSurfaceStub assignedLine(transform, 1.0, 1.0);
  BOOST_CHECK(assignedLine != originalLine);  // operator != from base
  assignedLine = originalLine;
  BOOST_CHECK(assignedLine == originalLine);  // operator == from base
}

/// Unit test for testing LineSurface alignment derivatives
BOOST_AUTO_TEST_CASE(LineSurfaceAlignment) {
  Translation3 translation{0., 1., 2.};
  Transform3 transform(translation);
  LineSurfaceStub line(transform, 2.0, 20.);

  const auto& rotation = transform.rotation();
  // The local frame z axis
  const Vector3 localZAxis = rotation.col(2);
  // Check the local z axis is aligned to global z axis
  CHECK_CLOSE_ABS(localZAxis, Vector3(0., 0., 1.), 1e-15);

  // Define the track (global) position and direction
  Vector3 globalPosition{1, 2, 4};
  Vector3 momentum{-1, 1, 1};
  Vector3 direction = momentum.normalized();

  // (a) Test the derivative of path length w.r.t. alignment parameters
  const AlignmentToPathMatrix& alignToPath =
      line.alignmentToPathDerivative(tgContext, globalPosition, direction);
  // The expected results
  AlignmentToPathMatrix expAlignToPath = AlignmentToPathMatrix::Zero();
  const double value = std::sqrt(3) / 2;
  expAlignToPath << -value, value, 0, -3 * value, -value, 0;
  // Check if the calculated derivative is as expected
  CHECK_CLOSE_ABS(alignToPath, expAlignToPath, 1e-10);

  // (b) Test the derivative of bound track parameters local position w.r.t.
  // position in local 3D Cartesian coordinates
  const auto& loc3DToLocBound =
      line.localCartesianToBoundLocalDerivative(tgContext, globalPosition);
  // Check if the result is as expected
  ActsMatrix<2, 3> expLoc3DToLocBound = ActsMatrix<2, 3>::Zero();
  expLoc3DToLocBound << 1 / std::sqrt(2), 1 / std::sqrt(2), 0, 0, 0, 1;
  CHECK_CLOSE_ABS(loc3DToLocBound, expLoc3DToLocBound, 1e-10);
}

BOOST_AUTO_TEST_CASE(LineSurfaceTransformRoundTrip) {
  LineSurfaceStub surface(Transform3::Identity());

  auto roundTrip = [&surface](const Vector3& pos, const Vector3& dir) {
    auto intersection = surface.intersect(tgContext, pos, dir).closest();
    Vector3 global = intersection.position();
    Vector2 local = *surface.globalToLocal(tgContext, global, dir);
    Vector3 global2 = surface.localToGlobal(tgContext, local, dir);
    return std::make_tuple(global, local, global2);
  };

  {
    Vector3 pos = {-0.02801, 0.00475611, 0.285106};
    Vector3 dir = Vector3(-0.03951, -0.221457, -0.564298).normalized();

    auto [global, local, global2] = roundTrip(pos, dir);

    CHECK_CLOSE_ABS(global, global2, 1e-10);
  }

  {
    Vector3 pos = {-64.2892, 65.2697, -0.839014};
    Vector3 dir = Vector3(-0.236602, -0.157616, 0.956786).normalized();

    auto [global, local, global2] = roundTrip(pos, dir);

    CHECK_CLOSE_ABS(global, global2, 1e-10);
  }
}

BOOST_AUTO_TEST_CASE(LineSurfaceTransformRoundTripEtaStability) {
  LineSurfaceStub surface(Transform3::Identity());

  // eta=6 is already crashing
  const std::vector<double> etas = {0, 1, 2, 3, 4, 5};

  for (double eta : etas) {
    Vector3 pca = {5, 0, 0};
    Vector3 dir = makeDirectionFromPhiEta(M_PI_2, eta);
    Vector3 pos = pca + dir;

    auto intersection = surface.intersect(tgContext, pos, dir).closest();

    Vector3 global = intersection.position();
    Vector2 local = *surface.globalToLocal(tgContext, global, dir);
    Vector3 global2 = surface.localToGlobal(tgContext, local, dir);

    CHECK_CLOSE_ABS(global, global2, 1e-10);
    CHECK_CLOSE_ABS(pca, global2, 1e-10);
  }
}

BOOST_AUTO_TEST_CASE(LineSurfaceIntersection) {
  using namespace Acts::UnitLiterals;

  double eps = 1e-10;

  Vector3 direction = Vector3(1, 1, 100).normalized();
  BoundVector boundVector;
  boundVector << 1_cm, 1_cm, VectorHelpers::phi(direction),
      VectorHelpers::theta(direction), 1, 0;
  double pathLimit = 1_cm;

  auto surface = std::make_shared<LineSurfaceStub>(Transform3::Identity());

  BoundTrackParameters initialParams{surface, boundVector, std::nullopt,
                                     ParticleHypothesis::pion()};

  using Propagator = Propagator<StraightLineStepper>;
  using PropagatorOptions = Propagator::Options<>;

  Propagator propagator({});

  CurvilinearTrackParameters displacedParameters{
      Vector4::Zero(), Vector3::Zero(), 1, std::nullopt,
      ParticleHypothesis::pion()};
  {
    PropagatorOptions options(tgContext, {});
    options.direction = Acts::Direction::Backward;
    options.pathLimit = pathLimit;

    auto result = propagator.propagate(initialParams, options);
    BOOST_CHECK(result.ok());
    BOOST_CHECK(result.value().endParameters);

    displacedParameters = result.value().endParameters.value();
  }

  auto intersection =
      surface
          ->intersect(tgContext, displacedParameters.position(tgContext),
                      displacedParameters.direction())
          .closest();
  CHECK_CLOSE_ABS(intersection.pathLength(), pathLimit, eps);

  BoundTrackParameters endParameters{surface, BoundVector::Zero(), std::nullopt,
                                     ParticleHypothesis::pion()};
  {
    PropagatorOptions options(tgContext, {});
    options.direction = Acts::Direction::Forward;
    options.stepping.maxStepSize = 1_mm;

    auto result = propagator.propagate(displacedParameters, *surface, options);
    BOOST_CHECK(result.ok());
    BOOST_CHECK(result.value().endParameters);
    CHECK_CLOSE_ABS(result.value().pathLength, pathLimit, eps);
    endParameters = result.value().endParameters.value();
  }

  CHECK_CLOSE_ABS(initialParams.parameters(), endParameters.parameters(), eps);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
