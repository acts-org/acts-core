// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Propagator/ConstrainedStep.hpp"

#include <limits>

namespace Acts::Test {

// This tests the implementation of the AbortList
// and the standard aborters
BOOST_AUTO_TEST_CASE(ConstrainedStepTest) {
  // forward stepping test
  ConstrainedStep stepSize_p(0.25);

  // All of the types should be 0.25 now
  BOOST_CHECK_EQUAL(stepSize_p.accuracy(), std::numeric_limits<double>::max());
  BOOST_CHECK_EQUAL(stepSize_p.value(ConstrainedStep::Type::Navigator),
                    std::numeric_limits<double>::max());
  BOOST_CHECK_EQUAL(stepSize_p.value(ConstrainedStep::Type::Actor),
                    std::numeric_limits<double>::max());
  BOOST_CHECK_EQUAL(stepSize_p.value(ConstrainedStep::Type::User), 0.25);

  // Check the cast operation to double
  BOOST_CHECK_EQUAL(stepSize_p.value(), 0.25);

  // now we set the accuracy
  stepSize_p.setAccuracy(0.1);
  BOOST_CHECK_EQUAL(stepSize_p.value(), 0.1);

  // now we update the actor to smaller
  stepSize_p.update(0.05, ConstrainedStep::Type::Navigator);
  BOOST_CHECK_EQUAL(stepSize_p.value(), 0.05);
  // we increase the actor, but do not release the step size
  stepSize_p.update(0.15, ConstrainedStep::Type::Navigator);
  BOOST_CHECK_EQUAL(stepSize_p.value(), 0.05);
  // we increase the actor, but now DO release the step size
  // it falls back to the accuracy
  stepSize_p.release(ConstrainedStep::Type::Navigator);
  stepSize_p.update(0.15, ConstrainedStep::Type::Navigator);
  BOOST_CHECK_EQUAL(stepSize_p.value(), 0.1);

  // now set two and update them
  stepSize_p.update(0.05, ConstrainedStep::Type::User);
  stepSize_p.setAccuracy(0.03);
  BOOST_CHECK_EQUAL(stepSize_p.value(), 0.03);

  // now we release the accuracy - to the highest available value
  stepSize_p.releaseAccuracy();
  BOOST_CHECK_EQUAL(stepSize_p.accuracy(), std::numeric_limits<double>::max());
  BOOST_CHECK_EQUAL(stepSize_p.value(), 0.05);
}

}  // namespace Acts::Test
