// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/TypeList.hpp"

#include <type_traits>
#include <typeinfo>

BOOST_AUTO_TEST_SUITE(Utilities)

BOOST_AUTO_TEST_CASE(TypeListCreation) {
  struct A {};

  using MyList = Acts::TypeList<float, int, double, A>;

  bool frontIsFloat = std::is_same_v<Acts::Types::front<MyList>, float>;
  BOOST_CHECK(frontIsFloat);

  bool backIsA = std::is_same_v<Acts::Types::back<MyList>, A>;
  BOOST_CHECK(backIsA);

  BOOST_CHECK_EQUAL(Acts::Types::size<MyList>, 4);
}

BOOST_AUTO_TEST_CASE(TypeListPushFront) {
  class A {};
  class B {};
  class C {};

  using BcList = Acts::TypeList<B, C>;
  auto abc = Acts::Types::push_front<BcList, A>{};
  bool frontIsA = std::is_same_v<Acts::Types::front<decltype(abc)>, A>;
  BOOST_CHECK(frontIsA);
}

BOOST_AUTO_TEST_CASE(TypeListPushBack) {
  class A {};
  class B {};
  class C {};

  using AbList = Acts::TypeList<A, B>;
  auto abc = Acts::Types::push_back<AbList, C>{};
  bool backIsC = std::is_same_v<Acts::Types::back<decltype(abc)>, C>;
  BOOST_CHECK(backIsC);
}

template <typename Head, typename... Tail>
void printTypes([[maybe_unused]] const Acts::TypeList<Head, Tail...>& t) {
  std::cout << typeid(Head).name() << '\n';
  if constexpr (sizeof...(Tail) > 0) {
    Acts::TypeList<Tail...> remainingTypes;
    printTypes(remainingTypes);
  }
}

BOOST_AUTO_TEST_CASE(TypeListPrintType) {
  class A {};
  class B {};
  class C {};

  using AbcList = Acts::TypeList<A, B, C>;
  AbcList a{};
  printTypes(a);
}

BOOST_AUTO_TEST_SUITE_END()
