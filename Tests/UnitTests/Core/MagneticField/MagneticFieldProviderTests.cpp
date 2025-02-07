// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"

namespace Acts::Test {

// Create a test context
MagneticFieldContext mfContext = MagneticFieldContext();

BOOST_AUTO_TEST_CASE(TypeErasedCacheType) {
  bool constructor_called = false;
  bool destructor_called = false;

  struct MyCache {
    MyCache(int value, bool* ctor, bool* dtor) : m_value{value}, m_dtor{dtor} {
      (*ctor) = true;
    }
    ~MyCache() { (*m_dtor) = true; }
    int m_value;
    bool* m_dtor;
  };

  BOOST_CHECK(!constructor_called);
  BOOST_CHECK(!destructor_called);

  {
    MagneticFieldProvider::Cache cache{
        MagneticFieldProvider::Cache(std::in_place_type<MyCache>, 42,
                                     &constructor_called, &destructor_called)};
    BOOST_CHECK(constructor_called);
    BOOST_CHECK(!destructor_called);

    MyCache& v = cache.as<MyCache>();
    BOOST_CHECK_EQUAL(v.m_value, 42);
    v.m_value = 65;

    MyCache& v2 = cache.as<MyCache>();
    BOOST_CHECK_EQUAL(v2.m_value, 65);
  }

  BOOST_CHECK(constructor_called);
  BOOST_CHECK(destructor_called);
}

}  // namespace Acts::Test
