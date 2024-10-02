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
#include "Acts/Geometry/BoundarySurfaceFace.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BoundingBox.hpp"

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Volumes)

BOOST_AUTO_TEST_CASE(bounding_box_creation) {
  float tol = 1e-4;

  TrapezoidVolumeBounds tvb(5, 10, 8, 4);

  auto bb = tvb.boundingBox();
  CHECK_CLOSE_ABS(bb.max(), Vector3(10, 8, 4), tol);
  CHECK_CLOSE_ABS(bb.min(), Vector3(-10, -8, -4), tol);

  Transform3 trf;

  trf = Translation3(Vector3(0, 30, 20));

  bb = tvb.boundingBox(&trf);
  CHECK_CLOSE_ABS(bb.max(), Vector3(10, 38, 24), tol);
  CHECK_CLOSE_ABS(bb.min(), Vector3(-10, 22, 16), tol);

  trf = AngleAxis3(M_PI / 2., Vector3(-2, 4, 5).normalized());

  bb = tvb.boundingBox(&trf);
  CHECK_CLOSE_ABS(bb.max(), Vector3(9.32577, 11.4906, 11.5777), tol);
  CHECK_CLOSE_ABS(bb.min(), Vector3(-9.77021, -8.65268, -9.23688), tol);
}

BOOST_AUTO_TEST_CASE(TrapezoidVolumeBoundarySurfaces) {
  TrapezoidVolumeBounds tvb(5, 10, 8, 4);

  auto tvbOrientedSurfaces = tvb.orientedSurfaces(Transform3::Identity());
  BOOST_CHECK_EQUAL(tvbOrientedSurfaces.size(), 6);

  auto geoCtx = GeometryContext();

  for (auto& os : tvbOrientedSurfaces) {
    auto osCenter = os.surface->center(geoCtx);
    const auto* pSurface =
        dynamic_cast<const Acts::PlaneSurface*>(os.surface.get());
    BOOST_REQUIRE_MESSAGE(pSurface != nullptr,
                          "The surface is not a plane surface");
    auto osNormal = pSurface->normal(geoCtx);
    // Check if you step inside the volume with the oriented normal
    Vector3 insideTvb = osCenter + os.direction * osNormal;
    Vector3 outsideTvb = osCenter - os.direction * osNormal;
    BOOST_CHECK(tvb.inside(insideTvb));
    BOOST_CHECK(!tvb.inside(outsideTvb));
  }

  Vector3 xaxis(1., 0., 0.);
  Vector3 yaxis(0., 1., 0.);
  Vector3 zaxis(0., 0., 1.);

  // Test the orientation of the boundary surfaces
  auto nFaceXY =
      tvbOrientedSurfaces[negativeFaceXY].surface->transform(geoCtx).rotation();
  BOOST_CHECK(nFaceXY.col(0).isApprox(xaxis));
  BOOST_CHECK(nFaceXY.col(1).isApprox(yaxis));
  BOOST_CHECK(nFaceXY.col(2).isApprox(zaxis));

  auto pFaceXY =
      tvbOrientedSurfaces[positiveFaceXY].surface->transform(geoCtx).rotation();
  BOOST_CHECK(pFaceXY.col(0).isApprox(xaxis));
  BOOST_CHECK(pFaceXY.col(1).isApprox(yaxis));
  BOOST_CHECK(pFaceXY.col(2).isApprox(zaxis));

  auto nFaceZX =
      tvbOrientedSurfaces[negativeFaceZX].surface->transform(geoCtx).rotation();
  BOOST_CHECK(nFaceZX.col(0).isApprox(zaxis));
  BOOST_CHECK(nFaceZX.col(1).isApprox(xaxis));
  BOOST_CHECK(nFaceZX.col(2).isApprox(yaxis));

  auto pFaceZX =
      tvbOrientedSurfaces[positiveFaceZX].surface->transform(geoCtx).rotation();
  BOOST_CHECK(pFaceZX.col(0).isApprox(zaxis));
  BOOST_CHECK(pFaceZX.col(1).isApprox(xaxis));
  BOOST_CHECK(pFaceZX.col(2).isApprox(yaxis));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
