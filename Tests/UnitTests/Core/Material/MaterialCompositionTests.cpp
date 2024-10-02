// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Material/MaterialComposition.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(material_composition)

constexpr float eps = 1.0f / 255u;

BOOST_AUTO_TEST_CASE(construct_element_fraction) {
  // carbon parameters, atomic charge is Z
  unsigned int carbonZ = 12u;
  // a fraction between 0 and 255
  unsigned int carbonWeight = 46u;
  float carbonFraction = static_cast<float>(carbonWeight) / 255u;

  ElementFraction a(carbonZ, carbonFraction);
  BOOST_CHECK_EQUAL(a.element(), carbonZ);
  CHECK_CLOSE_REL(a.fraction(), carbonFraction, eps);

  ElementFraction b(carbonZ, carbonWeight);
  BOOST_CHECK_EQUAL(b.element(), carbonZ);
  CHECK_CLOSE_REL(b.fraction(), carbonFraction, eps);
}

BOOST_AUTO_TEST_CASE(construct_with_fractions) {
  ElementFraction carbon(12u, 0.45f);
  ElementFraction silicon(14u, 0.125f);
  ElementFraction titanium(22u, 0.25f);
  ElementFraction copper(29u, 0.175f);

  MaterialComposition compound({silicon, carbon, titanium, copper});
  BOOST_CHECK(!!compound);
  BOOST_CHECK_EQUAL(compound.size(), 4u);

  float totalFraction = 0.0f;
  for (const auto& eFraction : compound) {
    totalFraction += eFraction.fraction();
  }
  // to better fit we need to implement some proper weight scaling
  CHECK_CLOSE_REL(totalFraction, 1.0f, compound.size() * eps);

  // input order should not matter
  MaterialComposition shuffled({carbon, silicon, titanium, copper});
  // check if the sorting worked
  BOOST_CHECK_EQUAL(compound.size(), shuffled.size());
  BOOST_CHECK_EQUAL(compound, shuffled);
}

BOOST_AUTO_TEST_CASE(construct_with_weights) {
  ElementFraction carbon(12u, 128u);
  ElementFraction silicon(14u, 64u);
  ElementFraction titanium(22u, 32u);
  ElementFraction copper(29u, 31u);

  MaterialComposition compound({silicon, carbon, titanium, copper});
  BOOST_CHECK(!!compound);
  BOOST_CHECK_EQUAL(compound.size(), 4u);

  float totalFraction = 0.0f;
  for (const auto& eFraction : compound) {
    totalFraction += eFraction.fraction();
  }
  // to better fit we need to implement some proper weight scaling
  CHECK_CLOSE_REL(totalFraction, 1.0f, compound.size() * eps);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
