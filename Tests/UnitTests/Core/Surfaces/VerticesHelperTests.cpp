// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/detail/VerticesHelper.hpp"

#include <algorithm>
#include <vector>

#include <Eigen/Geometry>

namespace Acts::detail::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)

BOOST_AUTO_TEST_CASE(VerticesHelperOnHyperPlane) {
  {
    // 0 points - always on a plane
    const std::vector<Vector3> testPlane = {};
    BOOST_CHECK(VerticesHelper::onHyperPlane(testPlane));
  }
  {
    // 1 point - always on a plane
    const std::vector<Vector3> testPlane = {Vector3(1., 3., 0.)};
    BOOST_CHECK(VerticesHelper::onHyperPlane(testPlane));
  }
  {
    // 2 points - always on a plane
    const std::vector<Vector3> testPlane = {Vector3(1., 3., 0.),
                                            Vector3(-2., 1., 0.)};
    BOOST_CHECK(VerticesHelper::onHyperPlane(testPlane));
  }
  {
    // 3 points - always on a plane
    const std::vector<Vector3> testPlane = {
        Vector3(1., 3., 0.), Vector3(-2., 1., 0.), Vector3(5., 8., 0.)};
    BOOST_CHECK(VerticesHelper::onHyperPlane(testPlane));
  }
  {
    // 4 points - these are on the xy-plane
    const std::vector<Vector3> testPlane = {
        Vector3(1., 3., 0.), Vector3(-2., 1., 0.), Vector3(5., 8., 0.),
        Vector3(-9., -9., 0.)};
    BOOST_CHECK(VerticesHelper::onHyperPlane(testPlane));
  }
  {
    // 4 points - these are NOT on the xy-plane
    const std::vector<Vector3> testPlane = {
        Vector3(1., 3., 0.), Vector3(-2., 1., 0.), Vector3(5., 8., 0.),
        Vector3(-9., -9., 1.)};
    BOOST_CHECK(!VerticesHelper::onHyperPlane(testPlane));
  }
  {
    // Test more points and add later an outlier.
    // Start with 6 points on the xy-plane
    std::vector<Vector3> testPlane = {
        Vector3(1., 3., 0.),   Vector3(-2., 1., 0.), Vector3(5., 8., 0.),
        Vector3(-9., -9., 0.), Vector3(5., 0., 0.),  Vector3(3., 1., 0.)};

    // All on a hyper plane
    BOOST_CHECK(VerticesHelper::onHyperPlane(testPlane));

    // Create the transform
    Transform3 transform(AngleAxis3(0.234, Vector3(0., 1., 0.)) *
                         AngleAxis3(-0.734, Vector3(1., 1., 1.).normalized()) *
                         Translation3(Vector3(-1., 2., 3.)));

    auto trfSpace = [](std::vector<Vector3>& vtxs,
                       const Transform3& trf) -> void {
      std::transform(vtxs.begin(), vtxs.end(), vtxs.begin(),
                     [&](auto& v) { return (trf * v); });
    };

    trfSpace(testPlane, transform);

    // All on a hyper plane
    BOOST_CHECK(VerticesHelper::onHyperPlane(testPlane));

    // One outside the s_onSurfaceTolerance
    testPlane.push_back(transform * Vector3(3., -4., 0.05));
    BOOST_CHECK(!VerticesHelper::onHyperPlane(testPlane));

    // But inside extended tolerance
    BOOST_CHECK(VerticesHelper::onHyperPlane(testPlane, 0.6));
  }
}

BOOST_AUTO_TEST_CASE(GeneratePhiSegments) {
  // Case (1): a small segment is given, no cartesian maximum vertex
  ActsScalar minPhi = 0.1;
  ActsScalar maxPhi = 0.3;

  auto phis = VerticesHelper::phiSegments(minPhi, maxPhi);
  BOOST_CHECK_EQUAL(phis.size(), 2u);
  BOOST_CHECK(phis[0] == minPhi);
  BOOST_CHECK(phis[1] == maxPhi);

  // Case (2) a small segment is given, with one maximum vertex at phi = 0
  minPhi = -0.1;
  phis = VerticesHelper::phiSegments(minPhi, maxPhi);
  BOOST_CHECK_EQUAL(phis.size(), 3u);
  BOOST_CHECK(phis[0] == minPhi);
  BOOST_CHECK(phis[1] == 0.);
  BOOST_CHECK(phis[2] == maxPhi);

  // Case (3) a small segment is given, with one maximum vertex at phi = 2pi,
  // and one extra value
  phis = VerticesHelper::phiSegments(minPhi, maxPhi, {0.25});
  BOOST_CHECK_EQUAL(phis.size(), 4u);
  BOOST_CHECK(phis[0] == minPhi);
  BOOST_CHECK(phis[1] == 0.);
  BOOST_CHECK(phis[2] == 0.25);
  BOOST_CHECK(phis[3] == maxPhi);

  // Case (4) a small segment is given, with one maximum vertex at phi = 2pi,
  // and two extra values, one outside & hence throw an exception
  BOOST_CHECK_THROW(VerticesHelper::phiSegments(minPhi, maxPhi, {0.25, 0.5}),
                    std::invalid_argument);

  // Case (5) an invalid phi range is given
  BOOST_CHECK_THROW(VerticesHelper::phiSegments(0.8, 0.2, {0.25, 0.5}),
                    std::invalid_argument);

  // Case (6) a wrong number of minimum segments is given
  BOOST_CHECK_THROW(VerticesHelper::phiSegments(0.1, 0.3, {0.25, 0.5}, 3),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(GenerateSegmentVertices) {
  // Case (1): a small segment is given, no cartesian maximum vertex & 1 step
  // segment
  ActsScalar rx = 10.;
  ActsScalar ry = 10.;
  ActsScalar minPhi = 0.1;
  ActsScalar maxPhi = 0.3;

  auto vertices = VerticesHelper::segmentVertices<Vector3, Transform3>(
      {rx, ry}, minPhi, maxPhi, {}, 1);
  std::size_t expectedVertices = 2u;
  BOOST_CHECK_EQUAL(vertices.size(), expectedVertices);

  // Now with a reference phi value
  vertices = VerticesHelper::segmentVertices<Vector3, Transform3>(
      {rx, ry}, minPhi, maxPhi, {0.2}, 1);
  expectedVertices = 3u;  // the reference is inserted
  BOOST_CHECK_EQUAL(vertices.size(), expectedVertices);

  // Now with more vertices - the the two corners and the ones from the
  // reference
  unsigned int quarterVertices = 36;
  vertices = VerticesHelper::segmentVertices<Vector3, Transform3>(
      {rx, ry}, minPhi, maxPhi, {}, quarterVertices);
  expectedVertices =
      static_cast<unsigned int>((maxPhi - minPhi) / M_PI_2 * quarterVertices) +
      2u;
  BOOST_CHECK_EQUAL(vertices.size(), expectedVertices);

  // Case (2) a small segment is given, with one maximum vertex at phi = 0
  minPhi = -0.1;
  vertices = VerticesHelper::segmentVertices<Vector3, Transform3>(
      {rx, ry}, minPhi, maxPhi, {}, 1);
  expectedVertices = 3u;
  BOOST_CHECK_EQUAL(vertices.size(), expectedVertices);

  // Same with more segments
  quarterVertices = 12;
  vertices = VerticesHelper::segmentVertices<Vector3, Transform3>(
      {rx, ry}, minPhi, maxPhi, {}, quarterVertices);
  // Extrema will be covered by the segments
  expectedVertices =
      static_cast<unsigned int>((maxPhi - minPhi) / M_PI_2 * quarterVertices) +
      2u;
  BOOST_CHECK_EQUAL(vertices.size(), expectedVertices);
}

BOOST_AUTO_TEST_CASE(GenerateCircleEllipseVertices) {
  // Case (1): A full disc
  ActsScalar ri = 0.;
  ActsScalar ro = 10.;

  // Extreme points in phi - only outer radius
  auto vertices = VerticesHelper::circularVertices(ri, ro, 0., M_PI, 1u);
  unsigned int expectedVertices = 5u;
  BOOST_CHECK_EQUAL(vertices.size(), expectedVertices);

  // Case (2): A ring
  ri = 3.;

  // Extreme points in phi - only outer radius
  vertices = VerticesHelper::circularVertices(ri, ro, 0., M_PI, 1u);
  expectedVertices = 10u;
  BOOST_CHECK_EQUAL(vertices.size(), expectedVertices);

  // Now with 10 bins per sector
  ri = 0.;

  vertices = VerticesHelper::circularVertices(ri, ro, 0., M_PI, 10u);
  expectedVertices = 41u;  // 4 sectors + 1 overlap at (-pi/pi)
  BOOST_CHECK_EQUAL(vertices.size(), expectedVertices);

  // Now ellipsiod
  ActsScalar riy = 4.;
  ActsScalar roy = 14.;
  vertices = VerticesHelper::ellipsoidVertices(ri, riy, ro, roy, 0., M_PI, 10u);
  expectedVertices = 41u;  // 4 sectors + 1 overlap at (-pi/pi)
  BOOST_CHECK_EQUAL(vertices.size(), expectedVertices);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::detail::Test
