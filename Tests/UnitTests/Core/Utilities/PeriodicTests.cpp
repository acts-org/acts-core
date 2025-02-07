// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <cmath>
#include <limits>
#include <numbers>
#include <tuple>
#include <utility>

using Acts::detail::difference_periodic;
using Acts::detail::normalizePhiTheta;
using Acts::detail::radian_pos;
using Acts::detail::radian_sym;
namespace bd = boost::unit_test::data;

namespace {
constexpr auto tol = std::numeric_limits<double>::epsilon();
}

namespace {
// Test dataset for periodic difference calculation, each entry is
//
//     lhs, rhs, periodic range, expected difference
//
constexpr std::tuple<double, double, double, double>
    kDifferencePeriodicDataset[] = {
        // lhs,rhs are exactly n periods apart
        {0.0, 1.0, 1.0, 0.0},
        {0.5, 1.5, 1.0, 0.0},
        {-1.5, 1.5, 1.0, 0.0},
        {0.0, 4.0, 1.0, 0.0},
        {0.5, 3.5, 1.0, 0.0},
        {-1.5, -125.5, 1.0, 0.0},
        // lhs,rhs are within one period and close together
        {0.75, 1.25, 2.0, -0.5},
        {4.25, 4.75, 2.0, -0.5},
        // lhs,rhs are within one period but far apart
        {0.25, 1.75, 2.0, 0.5},
        {-0.75, 0.75, 2.0, 0.5},
        // lhs,rhs are not within one period and close together
        {0.75, 5.25, 2.0, -0.5},
        {1.25, -2.5, 2.0, -0.25},
        // one of lhs,rhs is over the edge and close together
        {-0.25, +0.25, 2 * std::numbers::pi, -0.5},
        {+0.25, -0.25, 2 * std::numbers::pi, +0.5},
        {2 * std::numbers::pi - 0.25, 2 * std::numbers::pi + 0.25,
         2 * std::numbers::pi, -0.5},
};
}  // namespace
BOOST_DATA_TEST_CASE(DifferencePeriodic, bd::make(kDifferencePeriodicDataset),
                     lhs, rhs, range, diff) {
  CHECK_CLOSE_ABS(difference_periodic(lhs, rhs, range), diff, tol);
  CHECK_CLOSE_ABS(difference_periodic(rhs, lhs, range), -diff, tol);
}

BOOST_DATA_TEST_CASE(RadianPos, bd::xrange(0.25, std::numbers::pi, 0.5),
                     delta) {
  // above upper limit folds back just above lower limit
  CHECK_CLOSE_ABS(radian_pos(0 - delta), 2 * std::numbers::pi - delta, tol);
  // below lower limit folds back just below upper limit
  CHECK_CLOSE_ABS(radian_pos(2 * std::numbers::pi + delta), delta, tol);
  // same as above but with additional 2pi shifts
  CHECK_CLOSE_ABS(radian_pos(-2 * std::numbers::pi - delta),
                  2 * std::numbers::pi - delta, tol);
  CHECK_CLOSE_ABS(radian_pos(4 * std::numbers::pi + delta), delta, tol);
}

BOOST_DATA_TEST_CASE(RadianSym, bd::xrange(0.25, std::numbers::pi, 0.5),
                     delta) {
  // above upper limit folds back just above lower limit
  CHECK_CLOSE_ABS(radian_sym(-std::numbers::pi - delta),
                  std::numbers::pi - delta, tol);
  // below lower limit folds back just below upper limit
  CHECK_CLOSE_ABS(radian_sym(std::numbers::pi + delta),
                  -std::numbers::pi + delta, tol);
  // same as above but with additional 2pi shifts
  CHECK_CLOSE_ABS(radian_sym(-std::numbers::pi - delta - 2 * std::numbers::pi),
                  std::numbers::pi - delta, tol);
  CHECK_CLOSE_ABS(radian_sym(std::numbers::pi + delta + 2 * std::numbers::pi),
                  -std::numbers::pi + delta, tol);
}

BOOST_DATA_TEST_CASE(NormalizePhiThetaInBounds,
                     bd::xrange(-std::numbers::pi, std::numbers::pi,
                                std::numbers::pi / 2.) *
                         bd::xrange(0., std::numbers::pi,
                                    std::numbers::pi / 4.),
                     phix, thetax) {
  // both phi and theta are in bounds and should remain unchanged
  auto [phi, theta] = normalizePhiTheta(phix, thetax);
  CHECK_CLOSE_ABS(phi, phix, tol);
  CHECK_CLOSE_ABS(theta, thetax, tol);
}

BOOST_DATA_TEST_CASE(NormalizePhiThetaCyclicPhi,
                     bd::xrange(0.25, std::numbers::pi, 0.5) *
                         bd::xrange(0., std::numbers::pi,
                                    std::numbers::pi / 4.),
                     deltaPhi, thetax) {
  // phi is outside bounds, but theta is within and should remain unchanged
  {
    // phi is too large
    auto [phi, theta] = normalizePhiTheta(std::numbers::pi + deltaPhi, thetax);
    CHECK_CLOSE_ABS(phi, -std::numbers::pi + deltaPhi, tol);
    CHECK_CLOSE_ABS(theta, thetax, tol);
  }
  {
    // phi is too small
    auto [phi, theta] = normalizePhiTheta(-std::numbers::pi - deltaPhi, thetax);
    CHECK_CLOSE_ABS(phi, std::numbers::pi - deltaPhi, tol);
    CHECK_CLOSE_ABS(theta, thetax, tol);
  }
}

BOOST_DATA_TEST_CASE(NormalizePhiThetaOutOfBoundsTheta,
                     bd::xrange(0., std::numbers::pi, 1.) *
                         bd::xrange(0.25, std::numbers::pi, 1.),
                     positivePhi, deltaTheta) {
  // theta is outside bounds, both phi and theta are updated
  {
    // theta is too large
    auto [phi, theta] =
        normalizePhiTheta(positivePhi, std::numbers::pi + deltaTheta);
    CHECK_CLOSE_ABS(phi, -std::numbers::pi + positivePhi, tol);
    CHECK_CLOSE_ABS(theta, std::numbers::pi - deltaTheta, tol);
  }
  {
    // theta is too small
    auto [phi, theta] = normalizePhiTheta(positivePhi, 0. - deltaTheta);
    CHECK_CLOSE_ABS(phi, -std::numbers::pi + positivePhi, tol);
    CHECK_CLOSE_ABS(theta, deltaTheta, tol);
  }
}
