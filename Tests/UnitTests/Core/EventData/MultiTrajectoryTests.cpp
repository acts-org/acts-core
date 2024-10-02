// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/tools/context.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/ProxyAccessor.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/detail/MultiTrajectoryTestsCommon.hpp"
#include "Acts/EventData/detail/TestSourceLink.hpp"
#include "Acts/EventData/detail/TestTrackState.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <algorithm>
#include <array>
#include <ostream>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {

using namespace Acts;
using namespace Acts::UnitLiterals;
using namespace Acts::Test;
using namespace Acts::detail::Test;
using namespace Acts::HashedStringLiteral;
namespace bd = boost::unit_test::data;

using ParametersVector = BoundTrackParameters::ParametersVector;
using CovarianceMatrix = BoundTrackParameters::CovarianceMatrix;
using Jacobian = BoundMatrix;

const GeometryContext gctx;
// fixed seed for reproducible tests
std::default_random_engine rng(31415);

struct Factory {
  using trajectory_t = VectorMultiTrajectory;
  using const_trajectory_t = ConstVectorMultiTrajectory;

  VectorMultiTrajectory create() { return {}; }
  ConstVectorMultiTrajectory createConst() { return {}; }
};

using CommonTests = MultiTrajectoryTestsCommon<Factory>;

}  // namespace

BOOST_AUTO_TEST_SUITE(EventDataMultiTrajectory)

BOOST_AUTO_TEST_CASE(Build) {
  CommonTests ct;
  ct.testBuild();
}

BOOST_AUTO_TEST_CASE(ConstCorrectness) {
  // make mutable
  VectorMultiTrajectory t;
  auto i0 = t.addTrackState();

  BOOST_CHECK(!IsReadOnlyMultiTrajectory<decltype(t)>::value);

  {
    VectorMultiTrajectory::TrackStateProxy tsp = t.getTrackState(i0);
    static_cast<void>(tsp);
    VectorMultiTrajectory::ConstTrackStateProxy ctsp = t.getTrackState(i0);
    static_cast<void>(ctsp);

    tsp.predicted().setRandom();
    // const auto& tsp_const = tsp;
    // tsp_const.predicted().setRandom();
    // ctsp.predicted().setRandom();
  }

  // is this something we actually want?
  ConstVectorMultiTrajectory ct = t;
  BOOST_CHECK_EQUAL(ct.size(), t.size());

  ConstVectorMultiTrajectory ctm{std::move(t)};
  BOOST_CHECK_EQUAL(ctm.size(), ct.size());

  {
    static_assert(
        std::is_same_v<ConstVectorMultiTrajectory::ConstTrackStateProxy,
                       decltype(ctm.getTrackState(i0))>,
        "Got mutable track state proxy");
    ConstVectorMultiTrajectory::ConstTrackStateProxy ctsp =
        ctm.getTrackState(i0);
    static_cast<void>(ctsp);

    // doesn't compile:
    // ctsp.predictedCovariance().setIdentity();
  }

  // doesn't compile:
  // ct.clear();
  // ct.addTrackState();
}

BOOST_AUTO_TEST_CASE(Clear) {
  CommonTests ct;
  ct.testClear();
}

BOOST_AUTO_TEST_CASE(ApplyWithAbort) {
  CommonTests ct;
  ct.testApplyWithAbort();
}

BOOST_AUTO_TEST_CASE(AddTrackStateWithBitMask) {
  CommonTests ct;
  ct.testAddTrackStateWithBitMask();
}

BOOST_AUTO_TEST_CASE(AddTrackStateComponents) {
  CommonTests ct;
  ct.testAddTrackStateComponents();
}

// assert expected "cross-talk" between trackstate proxies
BOOST_AUTO_TEST_CASE(TrackStateProxyCrossTalk) {
  CommonTests ct;
  ct.testTrackStateProxyCrossTalk(rng);
}

BOOST_AUTO_TEST_CASE(TrackStateReassignment) {
  CommonTests ct;
  ct.testTrackStateReassignment(rng);
}

BOOST_DATA_TEST_CASE(TrackStateProxyStorage, bd::make({1u, 2u}),
                     nMeasurements) {
  CommonTests ct;
  ct.testTrackStateProxyStorage(rng, nMeasurements);
}

BOOST_AUTO_TEST_CASE(TrackStateProxyAllocations) {
  CommonTests ct;
  ct.testTrackStateProxyAllocations(rng);
}

BOOST_AUTO_TEST_CASE(TrackStateProxyGetMask) {
  CommonTests ct;
  ct.testTrackStateProxyGetMask();
}

BOOST_AUTO_TEST_CASE(TrackStateProxyCopy) {
  CommonTests ct;
  ct.testTrackStateProxyCopy(rng);
}

BOOST_AUTO_TEST_CASE(TrackStateCopyDynamicColumns) {
  CommonTests ct;
  ct.testTrackStateCopyDynamicColumns();
}

BOOST_AUTO_TEST_CASE(TrackStateProxyCopyDiffMTJ) {
  CommonTests ct;
  ct.testTrackStateProxyCopyDiffMTJ();
}

BOOST_AUTO_TEST_CASE(ProxyAssignment) {
  CommonTests ct;
  ct.testProxyAssignment();
}

BOOST_AUTO_TEST_CASE(CopyFromConst) {
  CommonTests ct;
  ct.testCopyFromConst();
}

BOOST_AUTO_TEST_CASE(TrackStateProxyShare) {
  CommonTests ct;
  ct.testTrackStateProxyShare(rng);
}

BOOST_AUTO_TEST_CASE(MultiTrajectoryExtraColumns) {
  CommonTests ct;
  ct.testMultiTrajectoryExtraColumns();
}

BOOST_AUTO_TEST_CASE(MultiTrajectoryExtraColumnsRuntime) {
  CommonTests ct;
  ct.testMultiTrajectoryExtraColumnsRuntime();
}

BOOST_AUTO_TEST_CASE(MemoryStats) {
  using namespace boost::histogram;
  using cat = axis::category<std::string>;

  VectorMultiTrajectory mt;

  auto stats = mt.statistics();

  std::stringstream ss;
  stats.toStream(ss);
  std::string out = ss.str();
  BOOST_CHECK(!out.empty());
  BOOST_CHECK_NE(out.find("total"), std::string::npos);

  const auto& h = stats.hist;

  auto column_axis = axis::get<cat>(h.axis(0));
  auto type_axis = axis::get<axis::category<>>(h.axis(1));

  for (int t = 0; t < type_axis.size(); t++) {
    for (int c = 0; c < column_axis.size(); c++) {
      double v = h.at(c, t);
      BOOST_CHECK_EQUAL(v, 0.0);
    }
  }

  TestTrackState pc(rng, 2u);
  auto ts = mt.makeTrackState();
  fillTrackState<VectorMultiTrajectory>(pc, TrackStatePropMask::All, ts);

  stats = mt.statistics();

  for (int t = 0; t < type_axis.size(); t++) {
    BOOST_TEST_CONTEXT((type_axis.bin(t) == 1 ? "meas" : "other"))
    for (int c = 0; c < column_axis.size(); c++) {
      std::string key = column_axis.bin(c);
      BOOST_TEST_CONTEXT("column: " << key) {
        double v = h.at(c, t);
        if (t == 0) {
          BOOST_CHECK_NE(v, 0.0);
        } else {
          BOOST_CHECK_EQUAL(v, 0.0);
        }
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(Accessors) {
  VectorMultiTrajectory mtj;
  mtj.addColumn<unsigned int>("ndof");
  mtj.addColumn<double>("super_chi2");

  auto ts = mtj.makeTrackState();

  ProxyAccessor<unsigned int> ndof("ndof");
  ConstProxyAccessor<unsigned int> ndofConst("ndof");
  ProxyAccessor<double> superChi2("super_chi2");
  ConstProxyAccessor<double> superChi2Const("super_chi2");

  ndof(ts) = 65;
  BOOST_CHECK_EQUAL((ts.component<unsigned int, "ndof"_hash>()), 65);
  BOOST_CHECK_EQUAL(ndofConst(ts), 65);

  // should not compile
  // ndofConst(ts) = 66;

  superChi2(ts) = 123.45;
  BOOST_CHECK_EQUAL((ts.component<double, "super_chi2"_hash>()), 123.45);
  BOOST_CHECK_EQUAL(superChi2Const(ts), 123.45);

  // should not compile
  // superChi2Const(ts) = 66.66;
}

BOOST_AUTO_TEST_CASE(ChangeSourceLinkType) {
  VectorMultiTrajectory mtj;
  auto ts = mtj.makeTrackState();

  int value = 5;
  ts.setUncalibratedSourceLink(SourceLink{value});

  BOOST_CHECK_EQUAL(ts.getUncalibratedSourceLink().get<int>(), value);
  BOOST_CHECK_THROW(ts.getUncalibratedSourceLink().get<double>(),
                    std::bad_any_cast);

  double otherValue = 42.42;

  // this changes the stored type
  ts.setUncalibratedSourceLink(SourceLink{otherValue});
  BOOST_CHECK_EQUAL(ts.getUncalibratedSourceLink().get<double>(), otherValue);
  BOOST_CHECK_THROW(ts.getUncalibratedSourceLink().get<int>(),
                    std::bad_any_cast);
}

BOOST_AUTO_TEST_SUITE_END()
