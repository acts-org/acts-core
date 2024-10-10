// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/BoundingBox.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Geometry)

BOOST_AUTO_TEST_CASE(CutoutCylinderVolumeBoundsConstruction) {
  CutoutCylinderVolumeBounds ccvb(5, 10, 15, 30, 25);
  ccvb.toStream(std::cout);

  // Test copy construction
  CutoutCylinderVolumeBounds copied(ccvb);
  BOOST_CHECK_EQUAL(ccvb, copied);

  // Test assigned
  CutoutCylinderVolumeBounds assigned = ccvb;
  BOOST_CHECK_EQUAL(ccvb, assigned);
}

BOOST_AUTO_TEST_CASE(CutoutCylinderVolumeBoundsRecreation) {
  CutoutCylinderVolumeBounds original(5, 10, 15, 30, 25);
  std::array<double, CutoutCylinderVolumeBounds::eSize> values{};
  std::vector<double> valvector = original.values();
  std::copy_n(valvector.begin(), CutoutCylinderVolumeBounds::eSize,
              values.begin());
  CutoutCylinderVolumeBounds recreated(values);
  BOOST_CHECK_EQUAL(original, recreated);
}

BOOST_AUTO_TEST_CASE(CutoutCylinderVolumeBoundsExceptions) {
  double rmin{5}, rmed{10}, rmax{15}, hz{30}, hzc{25};

  // Test negative rmin
  BOOST_CHECK_THROW(CutoutCylinderVolumeBounds(-rmin, rmed, rmax, hz, hzc),
                    std::logic_error);

  // Test negative rmed
  BOOST_CHECK_THROW(CutoutCylinderVolumeBounds(rmin, -rmed, rmax, hz, hzc),
                    std::logic_error);

  // Test negative rmax
  BOOST_CHECK_THROW(CutoutCylinderVolumeBounds(rmin, rmed, -rmax, hz, hzc),
                    std::logic_error);

  // Test swapped rmin / rmed
  BOOST_CHECK_THROW(CutoutCylinderVolumeBounds(rmed, rmin, rmax, hz, hzc),
                    std::logic_error);

  // Test swapped rmin / rmax
  BOOST_CHECK_THROW(CutoutCylinderVolumeBounds(rmax, rmed, rmin, hz, hzc),
                    std::logic_error);

  // Test swapped rmed / rmax
  BOOST_CHECK_THROW(CutoutCylinderVolumeBounds(rmin, rmax, rmed, hz, hzc),
                    std::logic_error);

  // Test negative hz
  BOOST_CHECK_THROW(CutoutCylinderVolumeBounds(rmin, rmed, rmax, -hz, hzc),
                    std::logic_error);

  // Test negative hzc
  BOOST_CHECK_THROW(CutoutCylinderVolumeBounds(rmin, rmed, rmax, hz, -hzc),
                    std::logic_error);
}

BOOST_AUTO_TEST_CASE(CutoutCylinderVolumeBoundsAccess) {
  double rmin{5}, rmed{10}, rmax{15}, hz{30}, hzc{25};
  CutoutCylinderVolumeBounds ccvb(rmin, rmed, rmax, hz, hzc);

  BOOST_CHECK_EQUAL(ccvb.get(CutoutCylinderVolumeBounds::eMinR), rmin);
  BOOST_CHECK_EQUAL(ccvb.get(CutoutCylinderVolumeBounds::eMedR), rmed);
  BOOST_CHECK_EQUAL(ccvb.get(CutoutCylinderVolumeBounds::eMaxR), rmax);
  BOOST_CHECK_EQUAL(ccvb.get(CutoutCylinderVolumeBounds::eHalfLengthZ), hz);
  BOOST_CHECK_EQUAL(ccvb.get(CutoutCylinderVolumeBounds::eHalfLengthZcutout),
                    hzc);
}

BOOST_AUTO_TEST_CASE(CutoutCylinderVolumeBoundsInside) {
  CutoutCylinderVolumeBounds ccvb(5, 10, 15, 30, 25);

  BOOST_CHECK(!ccvb.inside({0, 0, 0}));
  BOOST_CHECK(!ccvb.inside({0, 3, 0}));
  BOOST_CHECK(!ccvb.inside({3, 0, 0}));
  BOOST_CHECK(!ccvb.inside({0, 7, 0}));
  BOOST_CHECK(!ccvb.inside({7, 0, 0}));
  BOOST_CHECK(ccvb.inside({0, 13, 0}));
  BOOST_CHECK(ccvb.inside({13, 0, 0}));
  BOOST_CHECK(!ccvb.inside({0, 17, 0}));
  BOOST_CHECK(!ccvb.inside({17, 0, 0}));

  // outside in z
  BOOST_CHECK(!ccvb.inside({0, 0, 35}));
  BOOST_CHECK(!ccvb.inside({0, 0, -35}));
  BOOST_CHECK(!ccvb.inside({0, 3, 35}));
  BOOST_CHECK(!ccvb.inside({0, 3, -35}));
  BOOST_CHECK(!ccvb.inside({3, 0, 35}));
  BOOST_CHECK(!ccvb.inside({3, 0, -35}));
  BOOST_CHECK(!ccvb.inside({0, 10, 35}));
  BOOST_CHECK(!ccvb.inside({0, 10, -35}));
  BOOST_CHECK(!ccvb.inside({10, 0, 35}));
  BOOST_CHECK(!ccvb.inside({10, 0, -35}));
  BOOST_CHECK(!ccvb.inside({0, 20, 35}));
  BOOST_CHECK(!ccvb.inside({0, 20, -35}));
  BOOST_CHECK(!ccvb.inside({20, 0, 35}));
  BOOST_CHECK(!ccvb.inside({20, 0, -35}));

  // in the choke point in z
  BOOST_CHECK(!ccvb.inside({0, 0, 27}));
  BOOST_CHECK(!ccvb.inside({0, 0, -27}));
  BOOST_CHECK(!ccvb.inside({0, 3, 27}));
  BOOST_CHECK(!ccvb.inside({0, 3, -27}));
  BOOST_CHECK(!ccvb.inside({3, 0, 27}));
  BOOST_CHECK(!ccvb.inside({3, 0, -27}));
  BOOST_CHECK(ccvb.inside({0, 7, 27}));
  BOOST_CHECK(ccvb.inside({0, 7, -27}));
  BOOST_CHECK(ccvb.inside({7, 0, 27}));
  BOOST_CHECK(ccvb.inside({7, 0, -27}));
  BOOST_CHECK(ccvb.inside({0, 13, 27}));
  BOOST_CHECK(ccvb.inside({0, 13, -27}));
  BOOST_CHECK(ccvb.inside({13, 0, 27}));
  BOOST_CHECK(ccvb.inside({13, 0, -27}));
  BOOST_CHECK(!ccvb.inside({0, 17, 27}));
  BOOST_CHECK(!ccvb.inside({0, 17, -27}));
  BOOST_CHECK(!ccvb.inside({17, 0, 27}));
  BOOST_CHECK(!ccvb.inside({17, 0, -27}));

  // right inside the choke point in z
  BOOST_CHECK(!ccvb.inside({0, 0, 23}));
  BOOST_CHECK(!ccvb.inside({0, 0, -23}));
  BOOST_CHECK(!ccvb.inside({0, 3, 23}));
  BOOST_CHECK(!ccvb.inside({0, 3, -23}));
  BOOST_CHECK(!ccvb.inside({3, 0, 23}));
  BOOST_CHECK(!ccvb.inside({3, 0, -23}));
  BOOST_CHECK(!ccvb.inside({0, 7, 23}));
  BOOST_CHECK(!ccvb.inside({0, 7, -23}));
  BOOST_CHECK(!ccvb.inside({7, 0, 23}));
  BOOST_CHECK(!ccvb.inside({7, 0, -23}));
  BOOST_CHECK(ccvb.inside({0, 13, 23}));
  BOOST_CHECK(ccvb.inside({0, 13, -23}));
  BOOST_CHECK(ccvb.inside({13, 0, 23}));
  BOOST_CHECK(ccvb.inside({13, 0, -23}));
  BOOST_CHECK(!ccvb.inside({0, 17, 23}));
  BOOST_CHECK(!ccvb.inside({0, 17, -23}));
  BOOST_CHECK(!ccvb.inside({17, 0, 23}));
  BOOST_CHECK(!ccvb.inside({17, 0, -23}));
}

BOOST_AUTO_TEST_CASE(CutoutCylinderVolumeBoundsBoundingBox) {
  GeometryContext tgContext = GeometryContext();
  CutoutCylinderVolumeBounds ccvb(5, 10, 15, 30, 25);
  auto box = ccvb.boundingBox();
  CHECK_CLOSE_ABS(box.min(), Vector3(-15, -15, -30), 1e-6);
  CHECK_CLOSE_ABS(box.max(), Vector3(15, 15, 30), 1e-6);

  auto ccvbSurfaces = ccvb.orientedSurfaces(Transform3::Identity());
}

BOOST_AUTO_TEST_CASE(CutoutCylinderVolumeOrientedBoundaries) {
  GeometryContext tgContext = GeometryContext();

  CutoutCylinderVolumeBounds ccvb(5, 10, 15, 30, 25);

  auto ccvbOrientedSurfaces = ccvb.orientedSurfaces(Transform3::Identity());
  BOOST_CHECK_EQUAL(ccvbOrientedSurfaces.size(), 8);

  auto geoCtx = GeometryContext();
  Vector3 xaxis(1., 0., 0.);
  Vector3 yaxis(0., 1., 0.);
  Vector3 zaxis(0., 0., 1.);

  for (auto& os : ccvbOrientedSurfaces) {
    auto onSurface = os.surface->binningPosition(geoCtx, BinningValue::binR);
    auto locPos =
        os.surface->globalToLocal(geoCtx, onSurface, Vector3::Zero()).value();
    auto osNormal = os.surface->normal(geoCtx, locPos);
    // Check if you step inside the volume with the oriented normal
    Vector3 insideCcvb = onSurface + os.direction * osNormal;
    Vector3 outsideCCvb = onSurface - os.direction * osNormal;

    BOOST_CHECK(ccvb.inside(insideCcvb));
    BOOST_CHECK(!ccvb.inside(outsideCCvb));

    // Test the orientation of the boundary surfaces
    auto rot = os.surface->transform(geoCtx).rotation();
    BOOST_CHECK(rot.col(0).isApprox(xaxis));
    BOOST_CHECK(rot.col(1).isApprox(yaxis));
    BOOST_CHECK(rot.col(2).isApprox(zaxis));
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
