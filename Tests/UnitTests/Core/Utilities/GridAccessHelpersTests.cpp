// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/GridAccessHelpers.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"

using namespace Acts;

BOOST_AUTO_TEST_SUITE(GridAccessHelpersTests)

BOOST_AUTO_TEST_CASE(Grid1DAccess) {
  Acts::GridAxisGenerators::EqBound eqBound{{0., 10.}, 10};
  using GridType = Acts::GridAxisGenerators::EqBound::grid_type<std::size_t>;
  using PointType = GridType::point_t;
  auto grid = GridType(eqBound());

  for (std::size_t i = 0; i < 10; ++i) {
    grid.atPosition(PointType{i + 0.5}) = i;
  }

  // Local access
  std::vector<std::size_t> fAccessor = {0u};
  std::vector<std::size_t> sAccessor = {1u};

  Vector2 lPosition{3.5, 6.5};
  auto flAccess =
      GridAccessHelpers::accessLocal<GridType>(lPosition, fAccessor);
  auto slAccess =
      GridAccessHelpers::accessLocal<GridType>(lPosition, sAccessor);

  // This should take out local 1D either first or second
  BOOST_CHECK_EQUAL(grid.atPosition(flAccess), 3u);
  BOOST_CHECK_EQUAL(grid.atPosition(slAccess), 6u);

  // Global access
  Vector3 gPosition{0.5, 3.5, 6.5};
  std::vector<BinningValue> fCast = {Acts::BinningValue::binX};
  std::vector<BinningValue> sCast = {Acts::BinningValue::binY};
  std::vector<BinningValue> tCast = {Acts::BinningValue::binZ};

  auto fgAccess = GridAccessHelpers::castPosition<GridType>(gPosition, fCast);
  auto sgAccess = GridAccessHelpers::castPosition<GridType>(gPosition, sCast);
  auto tgAccess = GridAccessHelpers::castPosition<GridType>(gPosition, tCast);
  BOOST_CHECK_EQUAL(grid.atPosition(fgAccess), 0u);
  BOOST_CHECK_EQUAL(grid.atPosition(sgAccess), 3u);
  BOOST_CHECK_EQUAL(grid.atPosition(tgAccess), 6u);

  // Can this go into a delegate?
  auto gsu = std::make_unique<
      const Acts::GridAccess::GlobalSubspace<BinningValue::binX>>();
  Acts::GridAccess::GlobalToGridLocal1DimDelegate gsuDelegate;
  gsuDelegate.connect<
      &Acts::GridAccess::GlobalSubspace<BinningValue::binX>::toGridLocal>(
      std::move(gsu));

  BOOST_CHECK(gsuDelegate.connected());
}

BOOST_AUTO_TEST_CASE(Grid2DAccess) {
  Acts::GridAxisGenerators::EqBoundEqBound eqeqBound{
      {0., 10.}, 10, {0., 10.}, 10};
  using GridType =
      Acts::GridAxisGenerators::EqBoundEqBound::grid_type<std::size_t>;
  using PointType = GridType::point_t;
  auto grid = GridType(eqeqBound());
  for (std::size_t j = 0; j < 10u; ++j) {
    for (std::size_t i = 0; i < 10u; ++i) {
      grid.atPosition(PointType{i + 0.5, j + 0.5}) = j * 100 + i;
    }
  }

  // Local access
  std::vector<std::size_t> fAccessor = {0u, 1u};
  Vector2 lPosition{3.5, 6.5};
  auto flAccess =
      GridAccessHelpers::accessLocal<GridType>(lPosition, fAccessor);
  BOOST_CHECK_EQUAL(grid.atPosition(flAccess), 603u);

  // Global access
  Vector3 gPosition{0.5, 3.5, 6.5};
  std::vector<BinningValue> fCast = {Acts::BinningValue::binX,
                                     Acts::BinningValue::binY};
  auto fgAccess = GridAccessHelpers::castPosition<GridType>(gPosition, fCast);
  BOOST_CHECK_EQUAL(grid.atPosition(fgAccess), 300u);
}

BOOST_AUTO_TEST_CASE(GlobalToGridLocalTests) {
  Acts::GridAccess::GlobalSubspace<BinningValue::binX, BinningValue::binY>
      gssXY;

  auto xy = gssXY.toGridLocal(Vector3{1., 2., 3.});
  BOOST_CHECK_EQUAL(xy[0], 1.);
  BOOST_CHECK_EQUAL(xy[1], 2.);

  Acts::GridAccess::GlobalSubspace<BinningValue::binZ> gssZ;
  auto z = gssZ.toGridLocal(Vector3{1., 2., 3.});
  BOOST_CHECK_EQUAL(z[0], 3.);

  Acts::GridAccess::Affine3Transformed<
      Acts::GridAccess::GlobalSubspace<BinningValue::binZ>>
      gssZT(gssZ, Acts::Transform3{Acts::Transform3::Identity()}.pretranslate(
                      Vector3{0., 0., 100.}));

  auto zt = gssZT.toGridLocal(Vector3{1., 2., 3.});
  BOOST_CHECK_EQUAL(zt[0], 103.);
}

BOOST_AUTO_TEST_CASE(BoundToGridLocalTests) {
  Acts::GridAccess::LocalSubspace<0u, 1u> bssXY;
  auto xy = bssXY.toGridLocal(Vector2{
      1.,
      2.,
  });

  BOOST_CHECK_EQUAL(xy[0], 1.);
  BOOST_CHECK_EQUAL(xy[1], 2.);
}

BOOST_AUTO_TEST_CASE(BoundCylinderToZPhiTests) {
  Acts::ActsScalar radius = 100.;
  Acts::ActsScalar shift = 0.;
  Acts::GridAccess::BoundCylinderToZPhi bctzp(radius, shift);

  auto zphi = bctzp.toGridLocal(Vector2{0.25 * radius, 52.});

  CHECK_CLOSE_ABS(zphi[0], 52., 1.e-6);
  CHECK_CLOSE_ABS(zphi[1], 0.25, 1.e-6);
}

BOOST_AUTO_TEST_SUITE_END()
