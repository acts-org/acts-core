// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/MagneticField/InterpolatedBFieldMap.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "Acts/Utilities/detail/grid_helper.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

using Acts::VectorHelpers::perp;

namespace Acts::Test {

// Create a test context
MagneticFieldContext mfContext = MagneticFieldContext();

BOOST_AUTO_TEST_CASE(InterpolatedBFieldMap_rz) {
  // definition of dummy BField
  struct BField {
    static Vector3 value(const std::array<double, 2>& rz) {
      double r = rz.at(0);
      double z = rz.at(1);
      // linear in r and z so interpolation should be exact
      return Vector3(r * z, 3 * r, -2 * z);
    }
  };

  // map (x,y,z) -> (r,z)
  auto transformPos = [](const Vector3& pos) {
    return Vector2(perp(pos), pos.z());
  };

  // map (Bx,By,Bz) -> (Bx,By,Bz)
  auto transformBField = [](const Vector3& field, const Vector3&) {
    return field;
  };

  // magnetic field known on grid in (r,z)
  Axis r(0.0, 4.0, 4u);
  Axis z(-5, 7, 6u);

  Grid g(Type<Vector3>, std::move(r), std::move(z));

  using Grid_t = decltype(g);
  using BField_t = InterpolatedBFieldMap<Grid_t>;

  // set grid values
  for (std::size_t i = 1; i <= g.numLocalBins().at(0) + 1; ++i) {
    for (std::size_t j = 1; j <= g.numLocalBins().at(1) + 1; ++j) {
      Grid_t::index_t indices = {{i, j}};
      const auto& llCorner = g.lowerLeftBinEdge(indices);
      g.atLocalBins(indices) = BField::value(llCorner);
    }
  }

  // create BField service
  BField_t b{{transformPos, transformBField, std::move(g)}};

  auto bCacheAny = b.makeCache(mfContext);
  BField_t::Cache& bCache = bCacheAny.as<BField_t::Cache>();

  auto check = [&](double i) {
    BOOST_CHECK(b.isInside({0, 0, i * 4.9}));
    BOOST_CHECK(!b.isInside({0, 0, i * 5.1}));

    BOOST_CHECK(b.isInside({i * 2.9, 0, 0}));
    BOOST_CHECK(!b.isInside({i * 3.1, 0, 0}));

    BOOST_CHECK(b.isInside({0, i * 2.9, 0}));
    BOOST_CHECK(!b.isInside({0, i * 3.1, 0}));

    BOOST_CHECK(b.isInside({2, 2.2, 0}));
    BOOST_CHECK(!b.isInside({2, 3, 0}));

    BOOST_CHECK(b.isInside({i * 2, 2.2, 0}));
    BOOST_CHECK(!b.isInside({i * 2, 3, 0}));

    BOOST_CHECK(b.isInside({2, i * 2.2, 0}));
    BOOST_CHECK(!b.isInside({2, i * 3, 0}));

    BOOST_CHECK(b.isInside({i * 2, i * 2.2, 0}));
    BOOST_CHECK(!b.isInside({i * 2, i * 3, 0}));
  };

  check(1);
  check(-1);

  Vector3 pos;
  pos << -3, 2.5,
      1.7;  // this was previously checked, but is actually out of bounds
  BOOST_CHECK(!b.isInside(pos));
  BOOST_CHECK(!b.getField(pos, bCacheAny).ok());

  pos << -1.6, 2.5, 1.7;
  BOOST_CHECK(b.isInside(pos));
  CHECK_CLOSE_REL(b.getField(pos, bCacheAny).value(),
                  BField::value({{perp(pos), pos.z()}}), 1e-6);

  auto& c = *bCache.fieldCell;
  BOOST_CHECK(c.isInside(transformPos(pos)));
  CHECK_CLOSE_REL(c.getField(transformPos(pos)),
                  BField::value({{perp(pos), pos.z()}}), 1e-6);

  ActsMatrix<3, 3> deriv;

  pos << 1, 1, -5.5;  // this position is outside the grid
  BOOST_CHECK(!b.isInside(pos));
  BOOST_CHECK(!b.getField(pos, bCacheAny).ok());
  BOOST_CHECK(!b.getFieldGradient(pos, deriv, bCacheAny).ok());

  pos << 1, 6, -1.7;  // this position is outside the grid
  BOOST_CHECK(!b.isInside(pos));
  BOOST_CHECK(!b.getField(pos, bCacheAny).ok());
  BOOST_CHECK(!b.getFieldGradient(pos, deriv, bCacheAny).ok());

  pos << 0, 1.5, -2.5;
  BOOST_CHECK(b.isInside(pos));
  bCacheAny = b.makeCache(mfContext);
  BField_t::Cache& bCache2 = bCacheAny.as<BField_t::Cache>();
  CHECK_CLOSE_REL(b.getField(pos, bCacheAny).value(),
                  BField::value({{perp(pos), pos.z()}}), 1e-6);
  c = *bCache2.fieldCell;
  BOOST_CHECK(c.isInside(transformPos(pos)));
  CHECK_CLOSE_REL(c.getField(transformPos(pos)),
                  BField::value({{perp(pos), pos.z()}}), 1e-6);

  pos << 2, 3, -4;
  BOOST_CHECK(!b.isInside(pos));

  pos << 2, 2.2, -4;
  BOOST_CHECK(b.isInside(pos));
  bCacheAny = b.makeCache(mfContext);
  BField_t::Cache& bCache3 = bCacheAny.as<BField_t::Cache>();
  CHECK_CLOSE_REL(b.getField(pos, bCacheAny).value(),
                  BField::value({{perp(pos), pos.z()}}), 1e-6);
  c = *bCache3.fieldCell;
  BOOST_CHECK(c.isInside(transformPos(pos)));
  CHECK_CLOSE_REL(c.getField(transformPos(pos)),
                  BField::value({{perp(pos), pos.z()}}), 1e-6);

  // some field cell tests
  BOOST_CHECK(!c.isInside(transformPos((pos << 3, 2, -3.7).finished())));
  BOOST_CHECK(!c.isInside(transformPos((pos << -2, 3, -4.7).finished())));
  BOOST_CHECK(!c.isInside(transformPos((pos << -2, 3, 4.7).finished())));
  BOOST_CHECK(c.isInside(transformPos((pos << 0, 2, -4.7).finished())));
  BOOST_CHECK(!c.isInside(transformPos((pos << 5, 2, 14.).finished())));
}
}  // namespace Acts::Test
