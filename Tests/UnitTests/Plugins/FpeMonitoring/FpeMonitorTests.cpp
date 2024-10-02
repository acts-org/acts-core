// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/FpeMonitoring/FpeMonitor.hpp"

#include <cmath>
#include <optional>

namespace {

__attribute__((noinline)) void divbyzero() {
  volatile float j = 0.0;
  volatile float r = 123 / j;
  (void)r;
}

__attribute__((noinline)) void overflow() {
  std::cout << "PRE OVERFLOW" << std::endl;
  volatile float j = std::numeric_limits<float>::max();
  volatile float r = j * j;
  (void)r;
  std::cout << "POST OVERFLOW" << std::endl;
}

__attribute__((noinline)) void invalid() {
  volatile float j = -1;
  volatile float r = std::sqrt(j);
  (void)r;
}

__attribute__((noinline)) void invalid2() {
  volatile float k = 0;
  volatile float p = k / 0.0;
  (void)p;
}

}  // namespace

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(FpeMonitorTest)

BOOST_AUTO_TEST_CASE(Invalid) {
  FpeMonitor mon;
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTDIV));

  invalid();

  BOOST_CHECK(mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTDIV));
}

BOOST_AUTO_TEST_CASE(DivByZero) {
  FpeMonitor mon;
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTDIV));

  divbyzero();

  BOOST_CHECK(!mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(mon.result().encountered(FpeType::FLTDIV));
}

BOOST_AUTO_TEST_CASE(Overflow) {
  FpeMonitor mon;
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTDIV));

  overflow();
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTDIV));
}

BOOST_AUTO_TEST_CASE(Combinations) {
  FpeMonitor mon;
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTDIV));

  invalid();
  BOOST_CHECK(mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTDIV));

  overflow();
  BOOST_CHECK(mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTDIV));

  divbyzero();

  BOOST_CHECK(mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(mon.result().encountered(FpeType::FLTDIV));
}

BOOST_AUTO_TEST_CASE(ClearOnEnter) {
  invalid();
  divbyzero();
  overflow();

  FpeMonitor mon;
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTDIV));
}

BOOST_AUTO_TEST_CASE(CheckRearmCount) {
  FpeMonitor mon;
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTINV), 0);
  BOOST_CHECK(mon.result().stackTraces().empty());

  invalid();
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTINV), 1);
  BOOST_CHECK_EQUAL(mon.result().stackTraces().size(), 1);

  invalid();
  // We can't observe this again because it's masked!
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTINV), 1);
  BOOST_CHECK_EQUAL(mon.result().stackTraces().size(), 1);

  mon.rearm();
  invalid();
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTINV), 2);
  BOOST_CHECK_EQUAL(mon.result().stackTraces().size(),
                    1);  // still at one because we deduplicated

  mon.rearm();
  invalid2();
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTINV), 3);
  BOOST_CHECK_EQUAL(mon.result().stackTraces().size(), 2);
  mon.result().deduplicate();  // doesn't do anything here actually
  BOOST_CHECK_EQUAL(mon.result().stackTraces().size(), 2);
}

BOOST_AUTO_TEST_CASE(Scoping) {
  FpeMonitor mon;
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTDIV));

  invalid();

  {
    FpeMonitor mon2;
    BOOST_CHECK(!mon2.result().encountered(FpeType::FLTINV));
    BOOST_CHECK(!mon2.result().encountered(FpeType::FLTOVF));
    BOOST_CHECK(!mon2.result().encountered(FpeType::FLTDIV));

    overflow();

    {
      FpeMonitor mon3;
      BOOST_CHECK(!mon3.result().encountered(FpeType::FLTINV));
      BOOST_CHECK(!mon3.result().encountered(FpeType::FLTOVF));
      BOOST_CHECK(!mon3.result().encountered(FpeType::FLTDIV));

      divbyzero();

      BOOST_CHECK(!mon3.result().encountered(FpeType::FLTINV));
      BOOST_CHECK(!mon3.result().encountered(FpeType::FLTOVF));
      BOOST_CHECK(mon3.result().encountered(FpeType::FLTDIV));

      // Test merging here
      auto merged = mon.result().merged(mon2.result());
      BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTINV), 1);
      BOOST_CHECK_EQUAL(mon2.result().count(FpeType::FLTOVF), 1);
      BOOST_CHECK_EQUAL(merged.count(FpeType::FLTINV), 1);
      BOOST_CHECK_EQUAL(merged.count(FpeType::FLTOVF), 1);
      BOOST_CHECK_EQUAL(merged.numStackTraces(), 2);
      merged = merged.merged(mon3.result());
      BOOST_CHECK_EQUAL(merged.count(FpeType::FLTINV), 1);
      BOOST_CHECK_EQUAL(merged.count(FpeType::FLTOVF), 1);
      BOOST_CHECK_EQUAL(merged.count(FpeType::FLTDIV), 1);
      BOOST_CHECK_EQUAL(merged.numStackTraces(), 3);
    }

    BOOST_CHECK(!mon2.result().encountered(FpeType::FLTINV));
    BOOST_CHECK(mon2.result().encountered(FpeType::FLTOVF));
    BOOST_CHECK(!mon2.result().encountered(FpeType::FLTDIV));
  }

  BOOST_CHECK(mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTDIV));
}

BOOST_AUTO_TEST_CASE(MergeDeduplication) {
  FpeMonitor mon;
  invalid();
  {
    FpeMonitor mon2;
    invalid();

    auto merged = mon.result().merged(mon2.result());
    BOOST_CHECK_EQUAL(merged.count(FpeType::FLTINV), 2);
    BOOST_CHECK_EQUAL(merged.stackTraces().size(), 1);
  }
}

BOOST_AUTO_TEST_CASE(ScopedSuppression) {
  FpeMonitor mon;
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTINV));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTOVF));
  BOOST_CHECK(!mon.result().encountered(FpeType::FLTDIV));

  invalid();

  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTINV), 1);
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTOVF), 0);
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTDIV), 0);

  {
    FpeMonitor mon2{0};  // disable all
    BOOST_CHECK(!mon2.result().encountered(FpeType::FLTINV));
    BOOST_CHECK(!mon2.result().encountered(FpeType::FLTOVF));
    BOOST_CHECK(!mon2.result().encountered(FpeType::FLTDIV));
    invalid();
    divbyzero();
    overflow();
    // were not registered in inner scope
    BOOST_CHECK(!mon2.result().encountered(FpeType::FLTINV));
    BOOST_CHECK(!mon2.result().encountered(FpeType::FLTOVF));
    BOOST_CHECK(!mon2.result().encountered(FpeType::FLTDIV));
  }

  // outer scope is also unchanged
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTINV), 1);
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTOVF), 0);
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTDIV), 0);

  invalid();

  // outer scope gets signal after being restored on the stack
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTINV), 2);
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTOVF), 0);
  BOOST_CHECK_EQUAL(mon.result().count(FpeType::FLTDIV), 0);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
