// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/detail/Extendable.hpp"

#include <tuple>
#include <type_traits>

namespace Acts::Test {

// This tests the implementation of the ActionList
// and the standard aborters
BOOST_AUTO_TEST_CASE(Extendable_) {
  struct TypeA {
    double vaA = 0.;
  };

  struct TypeB {
    int vaB = 0;
  };

  struct TypeC {
    char vaC = '0';
  };

  // Test the empty list
  detail::Extendable<> nullist{};
  (void)nullist;
  BOOST_CHECK_EQUAL(std::tuple_size_v<std::tuple<>>, 0u);

  detail::Extendable<TypeA> alist;
  auto& a0_object = alist.get<TypeA>();
  a0_object.vaA = 1.;
  BOOST_CHECK_EQUAL(alist.get<TypeA>().vaA, 1.);

  detail::Extendable<TypeA, TypeB> ablist;
  auto& a1_object = ablist.get<TypeA>();
  a1_object.vaA = 2.;
  auto& b1_object = ablist.get<TypeB>();
  b1_object.vaB = 3;
  BOOST_CHECK_EQUAL(ablist.get<TypeA>().vaA, 2.);
  BOOST_CHECK_EQUAL(ablist.get<TypeB>().vaB, 3);

  TypeC c;
  c.vaC = '4';
  detail::Extendable<TypeA, TypeB, TypeC> abcList = ablist.append<TypeC>(c);
  BOOST_CHECK_EQUAL(abcList.get<TypeA>().vaA, 2.);
  BOOST_CHECK_EQUAL(abcList.get<TypeB>().vaB, 3);
  BOOST_CHECK_EQUAL(abcList.get<TypeC>().vaC, '4');
}

}  // namespace Acts::Test
