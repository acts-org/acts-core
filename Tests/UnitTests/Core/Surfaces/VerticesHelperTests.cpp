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

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::detail::Test
