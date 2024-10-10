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
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/BoundarySurfaceFace.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Acts::Test {

GeometryContext gctx = GeometryContext();

double hx{10.}, hy{20.}, hz{30.};

BOOST_AUTO_TEST_SUITE(Geometry)

BOOST_AUTO_TEST_CASE(CuboidVolumeConstruction) {
  // Test Construction
  CuboidVolumeBounds box(hx, hy, hz);

  // Test copy construction
  CuboidVolumeBounds copied(box);
  BOOST_CHECK_EQUAL(box, copied);

  // Test assigned
  CuboidVolumeBounds assigned = box;
  BOOST_CHECK_EQUAL(box, assigned);
}

BOOST_AUTO_TEST_CASE(CuboidVolumeRecreation) {
  CuboidVolumeBounds original(hx, hy, hz);
  auto valvector = original.values();
  std::array<double, CuboidVolumeBounds::eSize> values{};
  std::copy_n(valvector.begin(), CuboidVolumeBounds::eSize, values.begin());
  CuboidVolumeBounds recreated(values);
  BOOST_CHECK_EQUAL(original, recreated);
}

BOOST_AUTO_TEST_CASE(CuboidVolumeException) {
  // Test exception negative x
  BOOST_CHECK_THROW(CuboidVolumeBounds(-hx, hy, hz), std::logic_error);
  // Test exception negative y
  BOOST_CHECK_THROW(CuboidVolumeBounds(hx, -hy, hz), std::logic_error);
  // Test exception negative z
  BOOST_CHECK_THROW(CuboidVolumeBounds(hx, hy, -hz), std::logic_error);
  // Other iterations 0
  BOOST_CHECK_THROW(CuboidVolumeBounds(-hx, hy, -hz), std::logic_error);
  // Other iterations 1
  BOOST_CHECK_THROW(CuboidVolumeBounds(-hx, -hy, hz), std::logic_error);
  // Other iterations 2
  BOOST_CHECK_THROW(CuboidVolumeBounds(hx, -hy, -hz), std::logic_error);
  // Other iterations : all
  BOOST_CHECK_THROW(CuboidVolumeBounds(-hx, -hy, -hz), std::logic_error);
}

BOOST_AUTO_TEST_CASE(CuboidVolumeProperties) {
  CuboidVolumeBounds box(hx, hy, hz);
  // Test the type
  BOOST_CHECK_EQUAL(box.type(), VolumeBounds::eCuboid);
  // Test the halflength x
  CHECK_CLOSE_ABS(box.get(CuboidVolumeBounds::eHalfLengthX), hx, s_epsilon);
  // Test the halflength y
  CHECK_CLOSE_ABS(box.get(CuboidVolumeBounds::eHalfLengthY), hy, s_epsilon);
  // Test the halflength z
  CHECK_CLOSE_ABS(box.get(CuboidVolumeBounds::eHalfLengthZ), hz, s_epsilon);
  // Test the streaming
  std::vector<double> actvalues = box.values();
  std::vector<double> refvalues = {hx, hy, hz};
  BOOST_CHECK_EQUAL_COLLECTIONS(actvalues.begin(), actvalues.end(),
                                refvalues.begin(), refvalues.end());

  // Inside position
  Vector3 inside({5., 10., 8.});
  // Outside positions  in x, y, z
  std::vector<Vector3> outsides = {
      {20., 1., -2.}, {1., -30., 2.}, {-1., 2., 100.}};

  // Inside position
  BOOST_CHECK(box.inside(inside, s_onSurfaceTolerance));

  // Outside position
  for (const auto& outside : outsides) {
    BOOST_CHECK(!box.inside(outside, s_onSurfaceTolerance));
  }

  // Check the binning value positions
  CHECK_CLOSE_ABS(box.binningBorder(Acts::BinningValue::binX), hx, s_epsilon);
  CHECK_CLOSE_ABS(box.binningBorder(Acts::BinningValue::binY), hy, s_epsilon);
  CHECK_CLOSE_ABS(box.binningBorder(Acts::BinningValue::binZ), hz, s_epsilon);
  CHECK_CLOSE_ABS(box.binningBorder(Acts::BinningValue::binR),
                  std::sqrt(hx * hx + hy * hy), s_epsilon);
}

BOOST_AUTO_TEST_CASE(CuboidVolumeBoundarySurfaces) {
  CuboidVolumeBounds box(5, 8, 7);
  auto cvbOrientedSurfaces = box.orientedSurfaces(Transform3::Identity());

  BOOST_CHECK_EQUAL(cvbOrientedSurfaces.size(), 6);

  auto geoCtx = GeometryContext();

  for (auto& os : cvbOrientedSurfaces) {
    auto osCenter = os.surface->center(geoCtx);
    const auto* pSurface =
        dynamic_cast<const Acts::PlaneSurface*>(os.surface.get());
    BOOST_REQUIRE_MESSAGE(pSurface != nullptr,
                          "The surface is not a plane surface");
    auto osNormal = pSurface->normal(geoCtx);
    // Check if you step inside the volume with the oriented normal
    Vector3 insideBox = osCenter + os.direction * osNormal;
    Vector3 outsideBox = osCenter - os.direction * osNormal;
    BOOST_CHECK(box.inside(insideBox));
    BOOST_CHECK(!box.inside(outsideBox));
  }

  Vector3 xaxis(1., 0., 0.);
  Vector3 yaxis(0., 1., 0.);
  Vector3 zaxis(0., 0., 1.);

  // Test the orientation of the boundary surfaces
  auto nFaceXY =
      cvbOrientedSurfaces[negativeFaceXY].surface->transform(geoCtx).rotation();
  BOOST_CHECK(nFaceXY.col(0).isApprox(xaxis));
  BOOST_CHECK(nFaceXY.col(1).isApprox(yaxis));
  BOOST_CHECK(nFaceXY.col(2).isApprox(zaxis));

  auto pFaceXY =
      cvbOrientedSurfaces[positiveFaceXY].surface->transform(geoCtx).rotation();
  BOOST_CHECK(pFaceXY.col(0).isApprox(xaxis));
  BOOST_CHECK(pFaceXY.col(1).isApprox(yaxis));
  BOOST_CHECK(pFaceXY.col(2).isApprox(zaxis));

  auto nFaceYZ =
      cvbOrientedSurfaces[negativeFaceYZ].surface->transform(geoCtx).rotation();
  BOOST_CHECK(nFaceYZ.col(0).isApprox(yaxis));
  BOOST_CHECK(nFaceYZ.col(1).isApprox(zaxis));
  BOOST_CHECK(nFaceYZ.col(2).isApprox(xaxis));

  auto pFaceYZ =
      cvbOrientedSurfaces[positiveFaceYZ].surface->transform(geoCtx).rotation();
  BOOST_CHECK(pFaceYZ.col(0).isApprox(yaxis));
  BOOST_CHECK(pFaceYZ.col(1).isApprox(zaxis));
  BOOST_CHECK(pFaceYZ.col(2).isApprox(xaxis));

  auto nFaceZX =
      cvbOrientedSurfaces[negativeFaceZX].surface->transform(geoCtx).rotation();
  BOOST_CHECK(nFaceZX.col(0).isApprox(zaxis));
  BOOST_CHECK(nFaceZX.col(1).isApprox(xaxis));
  BOOST_CHECK(nFaceZX.col(2).isApprox(yaxis));

  auto pFaceZX =
      cvbOrientedSurfaces[positiveFaceZX].surface->transform(geoCtx).rotation();
  BOOST_CHECK(pFaceZX.col(0).isApprox(zaxis));
  BOOST_CHECK(pFaceZX.col(1).isApprox(xaxis));
  BOOST_CHECK(pFaceZX.col(2).isApprox(yaxis));
}

BOOST_AUTO_TEST_CASE(CuboidVolumeBoundsSetValues) {
  CuboidVolumeBounds box(5, 8, 7);

  for (auto bValue :
       {CuboidVolumeBounds::eHalfLengthX, CuboidVolumeBounds::eHalfLengthY,
        CuboidVolumeBounds::eHalfLengthZ}) {
    ActsScalar target = 0.5 * box.get(bValue);
    ActsScalar previous = box.get(bValue);
    BOOST_CHECK_THROW(box.set(bValue, -1), std::logic_error);
    BOOST_CHECK_EQUAL(box.get(bValue), previous);
    box.set(bValue, target);
    BOOST_CHECK_EQUAL(box.get(bValue), target);
  }

  auto previous = box.values();

  BOOST_CHECK_THROW(box.set({
                        {CuboidVolumeBounds::eHalfLengthX, -1},
                        {CuboidVolumeBounds::eHalfLengthY, 1},
                    }),
                    std::logic_error);
  auto act = box.values();
  BOOST_CHECK_EQUAL_COLLECTIONS(previous.begin(), previous.end(), act.begin(),
                                act.end());
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
