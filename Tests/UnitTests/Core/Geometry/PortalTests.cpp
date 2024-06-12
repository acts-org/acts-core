// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/Portal.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/LineSurface.hpp"
#include "Acts/Utilities/AxisFwd.hpp"

#include <iostream>
#include <stdexcept>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

auto logger = Acts::getDefaultLogger("UnitTests", Acts::Logging::VERBOSE);

struct Fixture {
  Logging::Level m_level;
  Fixture() {
    m_level = Acts::Logging::getFailureThreshold();
    Acts::Logging::setFailureThreshold(Acts::Logging::FATAL);
  }

  ~Fixture() { Acts::Logging::setFailureThreshold(m_level); }
};

BOOST_FIXTURE_TEST_SUITE(Geometry, Fixture)

BOOST_AUTO_TEST_SUITE(GridMerging)

BOOST_AUTO_TEST_CASE(Merging1dCylinder) {
  auto cyl = Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30_mm,
                                                  100_mm);

  BOOST_TEST_CONTEXT("z Binning") {
    BOOST_CHECK_THROW(
        GridPortalLink::make(*cyl, GridPortalLink::Direction::loc1,
                             Axis{AxisBound, 0, 5, 5}),
        std::invalid_argument);

    std::unique_ptr<GridPortalLink> grid1dCyl =
        GridPortalLink::make(*cyl, GridPortalLink::Direction::loc1,
                             Axis{AxisBound, -100_mm, 100_mm, 5});

    // Another cylinder, shifted in z
    auto cyl2 = Surface::makeShared<CylinderSurface>(

        Transform3{Translation3{Vector3::UnitZ() * 200_mm}}, 30_mm, 100_mm);

    std::unique_ptr<GridPortalLink> grid1dCyl2 =
        GridPortalLink::make(*cyl, GridPortalLink::Direction::loc1,
                             Axis{AxisBound, -100_mm, 100_mm, 5});
  }

  BOOST_TEST_CONTEXT("rPhi Binning") {
    BOOST_CHECK_THROW(
        GridPortalLink::make(*cyl, GridPortalLink::Direction::loc0,
                             Axis{AxisBound, 0, 5, 5}),
        std::invalid_argument);

    auto grid1dCyl =
        GridPortalLink::make(*cyl, GridPortalLink::Direction::loc0,
                             Axis{AxisBound, -M_PI * 30_mm, M_PI * 30_mm, 5});
  }
}

BOOST_AUTO_TEST_CASE(Merging2dCylinder) {
  auto cyl = Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30_mm,
                                                  100_mm);

  // z bad, rphi bad
  BOOST_CHECK_THROW(GridPortalLink::make(*cyl, Axis{AxisBound, 1, 2, 5},
                                         Axis{AxisBound, 3_mm, 4_mm, 5}),
                    std::invalid_argument);

  // z good, rphi bad
  BOOST_CHECK_THROW(
      GridPortalLink::make(*cyl, Axis{AxisBound, 0, 5, 5},
                           Axis{AxisBound, -M_PI * 30_mm, M_PI * 30_mm, 5}),
      std::invalid_argument);

  // z bad, rphi good
  BOOST_CHECK_THROW(GridPortalLink::make(*cyl, Axis{AxisBound, 1, 2, 5},
                                         Axis{AxisBound, -100_mm, 100_mm, 5}),
                    std::invalid_argument);

  auto grid2dCyl = GridPortalLink::make(
      *cyl, Axis{AxisBound, -M_PI * 30_mm, M_PI * 30_mm, 5},
      Axis{AxisBound, -100_mm, 100_mm, 5});

  std::cout << *grid2dCyl << std::endl;
}

// std::unique_ptr<PortalLinkBase> grid1d1 =
//     GridPortalLink::make(Axis{AxisBound{}, 0, 5, 5});
// std::unique_ptr<PortalLinkBase> grid1d2 =
//     GridPortalLink::make(Axis{AxisBound{}, -1, 5, 6});
//
// // @TODO Equidistance different bin widths to variable
// // @TODO Diagonal offset gives error
// // @TODO Zero offset
//
// {
//   auto mergedPtr = grid1d1->merge(*grid1d2, Vector2{5, 0}, *logger);
//   auto* merged = dynamic_cast<GridPortalLink*>(mergedPtr.get());
//   BOOST_REQUIRE_NE(merged, nullptr);
//   BOOST_CHECK_EQUAL(merged->grid().axes().size(), 1);
//   auto& axis = *merged->grid().axes().front();
//   BOOST_CHECK_EQUAL(axis.getMin(), -1);
//   BOOST_CHECK_EQUAL(axis.getMax(), 10);
// }
//
// {
//   auto mergedPtr = grid1d2->merge(*grid1d1, Vector2{6, 0}, *logger);
//   auto* merged = dynamic_cast<GridPortalLink*>(mergedPtr.get());
//   // BOOST_REQUIRE_NE(merged, nullptr);
//   // merged->grid().axes();
// }

// @TODO Check merge loc0 (closed + bound)
// @TODO Check merge loc1 (bound only)
// @TODO Check non-bound for same direction (error)
// @TODO: Check inconsistent directions (error)

// std::unique_ptr<PortalLinkBase> grid2d1 = GridPortalLink::make(
//     Axis{AxisBound{}, 0, 5, 5}, Axis{AxisBound{}, 0, 5, 5});
//
// grid1d1->merge(*grid2d1);
// grid2d1->merge(*grid1d1);

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
