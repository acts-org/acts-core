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
#include "Acts/Surfaces/ConeBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

/* Note on nomenclature:
  alpha = cone opening half angle
  z is the axis of symmetry
  zmin, zmax define limits for truncated cone
  phi is clock angle around cone, with x axis corresponding to phi=0
  Cone segments may be defined with the avphi (central position of segment) and
    halfphi (extent in phi of cone segment either side of the avphi)
  Local coords are z, rphi
*/
namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Surfaces)

/// Unit test for creating compliant/non-compliant ConeBounds object
BOOST_AUTO_TEST_CASE(ConeBoundsConstruction) {
  // test default construction
  // ConeBounds defaultConstructedConeBounds;  // deleted
  double alpha(M_PI / 8.0), zMin(3.), zMax(6.), halfPhi(M_PI / 4.0),
      averagePhi(0.);
  const bool symmetric(false);
  BOOST_TEST_CHECKPOINT("Four parameter constructor (last two at default)");
  ConeBounds defaultConeBounds(alpha, symmetric);
  BOOST_CHECK_EQUAL(defaultConeBounds.type(), SurfaceBounds::eCone);
  BOOST_TEST_CHECKPOINT("Four parameter constructor");
  ConeBounds fourParameterConstructed(alpha, symmetric, halfPhi, averagePhi);
  BOOST_CHECK_EQUAL(fourParameterConstructed.type(), SurfaceBounds::eCone);
  BOOST_TEST_CHECKPOINT("Five parameter constructor (last two at default)");
  ConeBounds defaulted5ParamConeBounds(alpha, zMin, zMax);
  BOOST_CHECK_EQUAL(defaulted5ParamConeBounds.type(), SurfaceBounds::eCone);
  BOOST_TEST_CHECKPOINT("Five parameter constructor)");
  ConeBounds fiveParamConstructedConeBounds(alpha, zMin, zMax, halfPhi,
                                            averagePhi);
  BOOST_CHECK_EQUAL(fiveParamConstructedConeBounds.type(),
                    SurfaceBounds::eCone);
  BOOST_TEST_CHECKPOINT("Copy constructor");
  ConeBounds copyConstructedConeBounds(fiveParamConstructedConeBounds);
  BOOST_CHECK_EQUAL(copyConstructedConeBounds, fiveParamConstructedConeBounds);
}

// Streaning and recreation test
BOOST_AUTO_TEST_CASE(ConeBoundsRecreation) {
  double alpha(M_PI / 8.0), zMin(3.), zMax(6.), halfPhi(M_PI / 4.0),
      averagePhi(0.);
  // const bool symmetric(false);
  ConeBounds original(alpha, zMin, zMax, halfPhi, averagePhi);
  auto valvector = original.values();
  std::array<double, ConeBounds::eSize> values{};
  std::copy_n(valvector.begin(), ConeBounds::eSize, values.begin());
  ConeBounds recreated(values);
  BOOST_CHECK_EQUAL(recreated, original);
}

// Unit tests for AnnulusBounds exception throwing
BOOST_AUTO_TEST_CASE(ConeBoundsExceptions) {
  double alpha(M_PI / 8.0), zMin(3.), zMax(6.), halfPhi(M_PI / 4.0),
      averagePhi(0.);

  // Exception for opening angle smaller 0
  BOOST_CHECK_THROW(ConeBounds(-alpha, zMin, zMax, halfPhi, averagePhi),
                    std::logic_error);
  // Exception for opening angle bigger M_PI
  BOOST_CHECK_THROW(ConeBounds(M_PI, zMin, zMax, halfPhi, averagePhi),
                    std::logic_error);
  // Exception for swapped zMin and zMax
  BOOST_CHECK_THROW(ConeBounds(alpha, zMax, zMin, halfPhi, averagePhi),
                    std::logic_error);
  // Exception for negative half sector phi
  BOOST_CHECK_THROW(ConeBounds(alpha, zMin, zMax, -halfPhi, averagePhi),
                    std::logic_error);
  // Exception for out of range  phi positioning
  BOOST_CHECK_THROW(ConeBounds(alpha, zMin, zMax, halfPhi, 2 * M_PI),
                    std::logic_error);
}

/// Unit tests for properties of ConeBounds object
BOOST_AUTO_TEST_CASE(ConeBoundsProperties) {
  double alpha(M_PI / 8.0), zMin(3.), zMax(6.), halfPhi(M_PI / 4.0),
      averagePhi(0.);
  // const bool symmetric(false);
  const Vector2 origin(0, 0);
  const Vector2 somewhere(4., 4.);
  ConeBounds coneBoundsObject(alpha, zMin, zMax, halfPhi, averagePhi);
  //
  /// test for type (redundant)
  BOOST_CHECK_EQUAL(coneBoundsObject.type(), SurfaceBounds::eCone);
  //
  /// test for inside
  BOOST_CHECK(!coneBoundsObject.inside(origin));
  //
  /// test for r
  CHECK_CLOSE_REL(coneBoundsObject.r(zMin), zMin * std::tan(alpha), 1e-6);
  //
  /// test for tanAlpha
  CHECK_CLOSE_REL(coneBoundsObject.tanAlpha(), std::tan(alpha), 1e-6);
  //
  /// test for alpha
  CHECK_CLOSE_REL(coneBoundsObject.get(ConeBounds::eAlpha), alpha, 1e-6);
  //
  /// test for minZ
  CHECK_CLOSE_REL(coneBoundsObject.get(ConeBounds::eMinZ), zMin, 1e-6);
  //
  /// test for maxZ
  CHECK_CLOSE_REL(coneBoundsObject.get(ConeBounds::eMaxZ), zMax, 1e-6);
  //
  /// test for averagePhi
  CHECK_CLOSE_REL(coneBoundsObject.get(ConeBounds::eHalfPhiSector), halfPhi,
                  1e-6);
  /// test for dump
  boost::test_tools::output_test_stream dumpOuput;
  coneBoundsObject.toStream(dumpOuput);
  BOOST_CHECK(dumpOuput.is_equal(
      "Acts::ConeBounds: (tanAlpha, minZ, maxZ, halfPhiSector, averagePhi) = "
      "(0.4142136, 3.0000000, 6.0000000, 0.7853982, 0.0000000)"));
}

// Unit test for testing ConeBounds assignment
BOOST_AUTO_TEST_CASE(ConeBoundsAssignment) {
  double alpha(M_PI / 8.0), zMin(3.), zMax(6.), halfPhi(M_PI / 4.0),
      averagePhi(0.);
  // const bool symmetric(false);
  ConeBounds originalConeBounds(alpha, zMin, zMax, halfPhi, averagePhi);
  ConeBounds assignedConeBounds(0.1, 2.3, 4.5, 1.2, 2.1);
  assignedConeBounds = originalConeBounds;
  BOOST_CHECK_EQUAL(assignedConeBounds, originalConeBounds);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
