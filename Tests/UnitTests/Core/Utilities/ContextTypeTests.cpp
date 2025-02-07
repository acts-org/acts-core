// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/detail/ContextType.hpp"

using namespace Acts;

BOOST_AUTO_TEST_SUITE(ContextTypeTests)

BOOST_AUTO_TEST_CASE(PackUnpack) {
  ContextType ctx;

  int v = 42;
  ctx = v;

  BOOST_CHECK_EQUAL(ctx.get<int>(), 42);
  BOOST_CHECK_THROW(ctx.get<double>(), std::bad_any_cast);
}

BOOST_AUTO_TEST_CASE(MaybeUnpack) {
  ContextType ctx;

  int v = 42;
  ctx = v;

  BOOST_CHECK_EQUAL(*ctx.maybeGet<int>(), 42);
  BOOST_CHECK_EQUAL(ctx.maybeGet<double>(), nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
