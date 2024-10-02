// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/TransformRange.hpp"

using namespace Acts;

BOOST_AUTO_TEST_SUITE(TransformRangeTests)

auto checkSameAddresses = [](const auto& orig, auto& act) {
  BOOST_CHECK_EQUAL(orig.size(), act.size());
  auto it = act.begin();
  for (std::size_t i = 0; i < orig.size(); ++i) {
    BOOST_CHECK_EQUAL(orig.at(i).get(), &act.at(i));
    BOOST_CHECK_EQUAL(orig.at(i).get(), &act[i]);
    BOOST_CHECK_EQUAL(*orig.at(i), act.at(i));

    BOOST_CHECK_EQUAL(orig.at(i).get(), &*it);
    BOOST_CHECK_EQUAL(*orig.at(i).get(), *it);

    ++it;
  }

  BOOST_CHECK(it == act.end());
};

BOOST_AUTO_TEST_CASE(TransformRangeDeref) {
  {
    std::vector<std::unique_ptr<int>> v;
    v.push_back(std::make_unique<int>(1));
    v.push_back(std::make_unique<int>(2));
    v.push_back(std::make_unique<int>(3));

    {
      auto r = detail::TransformRange{detail::Dereference{}, v};
      static_assert(std::is_same_v<decltype(r)::value_type, int>);
      static_assert(std::is_same_v<decltype(r)::reference, int&>);
      static_assert(std::is_same_v<decltype(r)::const_reference, const int&>);
      static_assert(std::is_same_v<decltype(r.at(0)), int&>);
      static_assert(std::is_same_v<decltype(r[0]), int&>);
      static_assert(std::is_same_v<decltype(*r.begin()), int&>);
      static_assert(std::is_same_v<decltype(*r.cbegin()), const int&>);

      r.at(2) = 4;
      BOOST_CHECK_EQUAL(*v.at(2), 4);

      checkSameAddresses(v, r);

      const auto& r_cref = r;
      static_assert(std::is_same_v<decltype(r_cref.at(0)), const int&>);
      static_assert(std::is_same_v<decltype(r_cref[0]), const int&>);
      static_assert(std::is_same_v<decltype(*r_cref.begin()), const int&>);
      static_assert(std::is_same_v<decltype(*(++r_cref.begin())), const int&>);
      checkSameAddresses(v, r_cref);

      auto cr = detail::TransformRange{detail::ConstDereference{}, v};
      static_assert(std::is_same_v<decltype(cr)::value_type, const int>);
      static_assert(std::is_same_v<decltype(cr)::reference, const int&>);
      static_assert(std::is_same_v<decltype(cr)::const_reference, const int&>);
      static_assert(std::is_same_v<decltype(cr.at(0)), const int&>);
      static_assert(std::is_same_v<decltype(cr[0]), const int&>);
      static_assert(std::is_same_v<decltype(*cr.begin()), const int&>);
      static_assert(std::is_same_v<decltype(*(++cr.begin())), const int&>);
      checkSameAddresses(v, cr);

      const auto& cr_cref = cr;
      static_assert(std::is_same_v<decltype(cr_cref.at(0)), const int&>);
      static_assert(std::is_same_v<decltype(cr_cref[0]), const int&>);
      static_assert(std::is_same_v<decltype(*cr_cref.begin()), const int&>);
      static_assert(std::is_same_v<decltype(*(++cr_cref.begin())), const int&>);
      checkSameAddresses(v, cr_cref);

      std::vector<int> act;
      std::copy(cr_cref.begin(), cr_cref.end(), std::back_inserter(act));
      std::vector<int> exp = {1, 2, 4};
      BOOST_CHECK_EQUAL_COLLECTIONS(exp.begin(), exp.end(), act.begin(),
                                    act.end());
    }

    std::vector<int*> raw_v;
    for (auto& ptr : v) {
      raw_v.push_back(ptr.get());
    }
    {
      auto r = detail::TransformRange{detail::Dereference{}, raw_v};
      static_assert(std::is_same_v<decltype(r)::value_type, int>);
      static_assert(std::is_same_v<decltype(r)::reference, int&>);
      static_assert(std::is_same_v<decltype(r)::const_reference, const int&>);
      static_assert(std::is_same_v<decltype(r.at(0)), int&>);
      static_assert(std::is_same_v<decltype(r[0]), int&>);
      static_assert(std::is_same_v<decltype(*r.begin()), int&>);
      static_assert(std::is_same_v<decltype(*(++r.begin())), int&>);
      checkSameAddresses(v, r);

      auto cr = detail::TransformRange{detail::ConstDereference{}, raw_v};
      static_assert(std::is_same_v<decltype(cr)::value_type, const int>);
      static_assert(std::is_same_v<decltype(cr)::reference, const int&>);
      static_assert(std::is_same_v<decltype(cr)::const_reference, const int&>);
      static_assert(std::is_same_v<decltype(cr.at(0)), const int&>);
      static_assert(std::is_same_v<decltype(cr[0]), const int&>);
      static_assert(std::is_same_v<decltype(*cr.begin()), const int&>);
      static_assert(std::is_same_v<decltype(*(++cr.begin())), const int&>);
      checkSameAddresses(v, r);
    }
  }

  {
    std::vector<std::unique_ptr<const int>> v;
    v.push_back(std::make_unique<const int>(1));
    v.push_back(std::make_unique<const int>(2));
    v.push_back(std::make_unique<const int>(3));

    {
      auto r = detail::TransformRange{detail::Dereference{}, v};
      static_assert(std::is_same_v<decltype(r)::value_type, const int>);
      static_assert(std::is_same_v<decltype(r)::reference, const int&>);
      static_assert(std::is_same_v<decltype(r)::const_reference, const int&>);
      static_assert(std::is_same_v<decltype(r.at(0)), const int&>);
      static_assert(std::is_same_v<decltype(r[0]), const int&>);
      static_assert(std::is_same_v<decltype(*r.begin()), const int&>);
      static_assert(std::is_same_v<decltype(*(++r.begin())), const int&>);
      checkSameAddresses(v, r);
    }

    std::vector<const int*> raw_v;
    for (auto& ptr : v) {
      raw_v.push_back(ptr.get());
    }
    {
      auto r = detail::TransformRange{detail::Dereference{}, raw_v};
      static_assert(std::is_same_v<decltype(r)::value_type, const int>);
      static_assert(std::is_same_v<decltype(r)::reference, const int&>);
      static_assert(std::is_same_v<decltype(r)::const_reference, const int&>);
      static_assert(std::is_same_v<decltype(r.at(0)), const int&>);
      static_assert(std::is_same_v<decltype(r[0]), const int&>);
      static_assert(std::is_same_v<decltype(*r.begin()), const int&>);
      static_assert(std::is_same_v<decltype(*(++r.begin())), const int&>);
      checkSameAddresses(v, r);
    }
  }
}

BOOST_AUTO_TEST_CASE(TransformRangeFromConstRef) {
  std::vector<std::unique_ptr<int>> v;
  v.push_back(std::make_unique<int>(1));
  v.push_back(std::make_unique<int>(2));
  v.push_back(std::make_unique<int>(3));

  const auto& cv = v;

  {
    auto r_cv = detail::TransformRange{detail::Dereference{}, cv};
    static_assert(std::is_same_v<decltype(r_cv)::value_type, const int>);
    static_assert(std::is_same_v<decltype(r_cv)::reference, const int&>);
    static_assert(std::is_same_v<decltype(r_cv)::const_reference, const int&>);
    static_assert(std::is_same_v<decltype(r_cv.at(0)), const int&>);
    static_assert(std::is_same_v<decltype(r_cv[0]), const int&>);
    static_assert(std::is_same_v<decltype(*r_cv.begin()), const int&>);
    static_assert(std::is_same_v<decltype(*(++r_cv.begin())), const int&>);

    checkSameAddresses(v, r_cv);
    checkSameAddresses(cv, r_cv);

    std::vector<int> act;
    std::copy(r_cv.begin(), r_cv.end(), std::back_inserter(act));
    std::vector<int> exp = {1, 2, 3};
    BOOST_CHECK_EQUAL_COLLECTIONS(exp.begin(), exp.end(), act.begin(),
                                  act.end());
  }

  {
    auto r_cv = detail::TransformRange{detail::ConstDereference{}, cv};
    static_assert(std::is_same_v<decltype(r_cv)::value_type, const int>);
    static_assert(std::is_same_v<decltype(r_cv)::reference, const int&>);
    static_assert(std::is_same_v<decltype(r_cv)::const_reference, const int&>);
    static_assert(std::is_same_v<decltype(r_cv.at(0)), const int&>);
    static_assert(std::is_same_v<decltype(r_cv[0]), const int&>);
    static_assert(std::is_same_v<decltype(*r_cv.begin()), const int&>);
    static_assert(std::is_same_v<decltype(*(++r_cv.begin())), const int&>);

    checkSameAddresses(v, r_cv);
    checkSameAddresses(cv, r_cv);

    std::vector<int> act;
    std::copy(r_cv.begin(), r_cv.end(), std::back_inserter(act));
    std::vector<int> exp = {1, 2, 3};
    BOOST_CHECK_EQUAL_COLLECTIONS(exp.begin(), exp.end(), act.begin(),
                                  act.end());
  }
}

BOOST_AUTO_TEST_CASE(TransformRangeReferenceWrappers) {
  int a = 5;
  int b = 6;
  int c = 7;

  std::vector<std::reference_wrapper<int>> v = {a, b, c};
  BOOST_CHECK_EQUAL(&v[0].get(), &a);

  auto r = detail::TransformRange{detail::DotGet{}, v};
  static_assert(std::is_same_v<decltype(r)::value_type, int>);
  static_assert(std::is_same_v<decltype(r)::reference, int&>);
  static_assert(std::is_same_v<decltype(r)::const_reference, const int&>);
  static_assert(std::is_same_v<decltype(r.at(0)), int&>);
  static_assert(std::is_same_v<decltype(r[0]), int&>);
  static_assert(std::is_same_v<decltype(*r.begin()), int&>);
  static_assert(std::is_same_v<decltype(*(++r.begin())), int&>);

  BOOST_CHECK_EQUAL(r.at(0), 5);
  BOOST_CHECK_EQUAL(&r.at(0), &a);
  BOOST_CHECK_EQUAL(r[0], 5);
  BOOST_CHECK_EQUAL(&r[0], &a);
  BOOST_CHECK_EQUAL(*r.begin(), 5);
  BOOST_CHECK_EQUAL(&*r.begin(), &a);

  std::vector<int> act;
  std::copy(r.begin(), r.end(), std::back_inserter(act));
  std::vector<int> exp = {5, 6, 7};
  BOOST_CHECK_EQUAL_COLLECTIONS(exp.begin(), exp.end(), act.begin(), act.end());
}

BOOST_AUTO_TEST_SUITE_END()
