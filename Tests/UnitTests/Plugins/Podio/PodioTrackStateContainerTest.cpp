// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/EventData/detail/MultiTrajectoryTestsCommon.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/Podio/PodioTrackStateContainer.hpp"
#include "Acts/Plugins/Podio/PodioUtil.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "ActsPodioEdm/BoundParametersCollection.h"
#include "ActsPodioEdm/JacobianCollection.h"
#include "ActsPodioEdm/TrackStateCollection.h"

#include <filesystem>

#include <podio/UserDataCollection.h>

namespace {

using namespace Acts;
using namespace Acts::UnitLiterals;
using namespace Acts::detail::Test;
namespace bd = boost::unit_test::data;

std::default_random_engine rng(31415);

class NullHelper : public PodioUtil::ConversionHelper {
 public:
  std::optional<PodioUtil::Identifier> surfaceToIdentifier(
      const Surface& /*surface*/) const override {
    return {};
  }
  const Surface* identifierToSurface(
      PodioUtil::Identifier /*identifier*/) const override {
    return nullptr;
  }

  SourceLink identifierToSourceLink(
      PodioUtil::Identifier /*identifier*/) const override {
    return SourceLink{0};
  }

  PodioUtil::Identifier sourceLinkToIdentifier(
      const SourceLink& /*sourceLink*/) override {
    return 0;
  }
};

struct MapHelper : public NullHelper {
  std::optional<PodioUtil::Identifier> surfaceToIdentifier(
      const Surface& surface) const override {
    for (auto&& [id, srf] : surfaces) {
      if (srf == &surface) {
        return id;
      }
    }
    return {};
  }
  const Surface* identifierToSurface(PodioUtil::Identifier id) const override {
    auto it = surfaces.find(id);
    if (it == surfaces.end()) {
      return nullptr;
    }

    return it->second;
  }

  PodioUtil::Identifier sourceLinkToIdentifier(const SourceLink& sl) override {
    sourceLinks.push_back(sl);
    return sourceLinks.size() - 1;
  }

  SourceLink identifierToSourceLink(PodioUtil::Identifier id) const override {
    return sourceLinks.at(id);
  }

  std::unordered_map<PodioUtil::Identifier, const Surface*> surfaces;
  std::vector<SourceLink> sourceLinks;
};

struct Factory {
  using trajectory_t = MutablePodioTrackStateContainer;
  using const_trajectory_t = ConstPodioTrackStateContainer;

  MapHelper m_helper;

  MutablePodioTrackStateContainer create() { return {m_helper}; }
};

using CommonTests = MultiTrajectoryTestsCommon<Factory>;

}  // namespace

BOOST_AUTO_TEST_SUITE(PodioTrackStateContainerTest)

BOOST_AUTO_TEST_CASE(Build) {
  CommonTests ct;
  ct.testBuild();
}

BOOST_AUTO_TEST_CASE(ConstCorrectness) {
  // @TODO: Const version can only be non-owning!
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

BOOST_AUTO_TEST_CASE(WriteToPodioFrame) {
  using namespace HashedStringLiteral;

  MapHelper helper;

  auto tmp_path = std::filesystem::temp_directory_path();
  auto outfile = tmp_path / "trackstates.root";

  BoundVector tv1;
  tv1 << 1, 1, 1, 1, 1, 1;

  BoundVector tv2 = tv1 * 2;
  BoundVector tv3 = tv1 * 3;
  BoundVector tv4 = tv1 * 4;

  BoundMatrix cov1;
  cov1.setOnes();

  BoundMatrix cov2 = cov1 * 2;
  BoundMatrix cov3 = cov1 * 3;
  BoundMatrix cov4 = cov1 * 4;

  auto rBounds = std::make_shared<RectangleBounds>(15, 20);
  auto trf = Transform3::Identity();
  trf.translation().setRandom();
  auto free = Acts::Surface::makeShared<PlaneSurface>(trf, rBounds);
  auto reg = Acts::Surface::makeShared<PlaneSurface>(trf, rBounds);

  helper.surfaces[666] = reg.get();

  podio::Frame frame;

  MutablePodioTrackStateContainer c{helper};
  BOOST_CHECK(!c.hasColumn("int_column"_hash));
  BOOST_CHECK(!c.hasColumn("float_column"_hash));
  c.addColumn<std::int32_t>("int_column");
  c.addColumn<float>("float_column");
  BOOST_CHECK(c.hasColumn("int_column"_hash));
  BOOST_CHECK(c.hasColumn("float_column"_hash));

  {
    auto t1 = c.makeTrackState(TrackStatePropMask::Predicted);
    t1.predicted() = tv1;
    t1.predictedCovariance() = cov1;

    t1.setReferenceSurface(free);

    auto t2 = c.makeTrackState(TrackStatePropMask::All, t1.index());
    t2.predicted() = tv2;
    t2.predictedCovariance() = cov2;

    t2.filtered() = tv3;
    t2.filteredCovariance() = cov3;

    t2.smoothed() = tv4;
    t2.smoothedCovariance() = cov4;

    t2.jacobian() = cov2;

    auto t3 = c.makeTrackState();
    t3.setReferenceSurface(reg);

    t1.component<std::int32_t, "int_column"_hash>() = -11;
    t2.component<std::int32_t, "int_column"_hash>() = 42;
    t3.component<std::int32_t, "int_column"_hash>() = -98;

    t1.component<float, "float_column"_hash>() = -11.2f;
    t2.component<float, "float_column"_hash>() = 42.4f;
    t3.component<float, "float_column"_hash>() = -98.9f;
  }

  c.releaseInto(frame, "test");

  BOOST_CHECK_EQUAL(frame.get("trackStates_test")->size(), 3);
  BOOST_CHECK_EQUAL(frame.get("trackStateParameters_test")->size(), 7);
  BOOST_CHECK_EQUAL(frame.get("trackStateJacobians_test")->size(), 2);
  BOOST_CHECK_NE(frame.get("trackStates_test_extra__int_column"), nullptr);
  BOOST_CHECK_NE(frame.get("trackStates_test_extra__float_column"), nullptr);

  ConstPodioTrackStateContainer cc{helper, frame, "test"};

  BOOST_CHECK_EQUAL(cc.size(), 3);
  BOOST_CHECK(cc.hasColumn("int_column"_hash));
  BOOST_CHECK(cc.hasColumn("float_column"_hash));

  auto t1 = cc.getTrackState(0);
  auto t2 = cc.getTrackState(1);
  auto t3 = cc.getTrackState(2);

  BOOST_CHECK_EQUAL(t2.previous(), 0);

  BOOST_CHECK(t1.hasReferenceSurface());
  BOOST_CHECK(!t2.hasReferenceSurface());
  BOOST_CHECK(t3.hasReferenceSurface());

  Acts::GeometryContext gctx;

  const auto& ext = t1.referenceSurface();
  BOOST_CHECK_NE(&ext, free.get());
  BOOST_CHECK_EQUAL(trf.matrix(), ext.transform(gctx).matrix());
  BOOST_CHECK_EQUAL(free->bounds().type(), ext.bounds().type());
  BOOST_CHECK_EQUAL(free->type(), ext.type());
  const auto* rBounds2 = dynamic_cast<const RectangleBounds*>(&ext.bounds());
  BOOST_REQUIRE_NE(rBounds2, nullptr);
  BOOST_CHECK_EQUAL(rBounds->halfLengthX(), rBounds2->halfLengthX());
  BOOST_CHECK_EQUAL(rBounds->halfLengthY(), rBounds2->halfLengthY());

  BOOST_CHECK_EQUAL(t1.predicted(), tv1);
  BOOST_CHECK_EQUAL(t1.predictedCovariance(), cov1);

  BOOST_CHECK_EQUAL(t2.predicted(), tv2);
  BOOST_CHECK_EQUAL(t2.predictedCovariance(), cov2);
  BOOST_CHECK_EQUAL(t2.filtered(), tv3);
  BOOST_CHECK_EQUAL(t2.filteredCovariance(), cov3);
  BOOST_CHECK_EQUAL(t2.smoothed(), tv4);
  BOOST_CHECK_EQUAL(t2.smoothedCovariance(), cov4);

  BOOST_CHECK_EQUAL(t2.jacobian(), cov2);

  BOOST_CHECK_EQUAL(&t3.referenceSurface(), reg.get());

  BOOST_CHECK_EQUAL((t1.component<std::int32_t, "int_column"_hash>()), -11);
  BOOST_CHECK_EQUAL((t2.component<std::int32_t, "int_column"_hash>()), 42);
  BOOST_CHECK_EQUAL((t3.component<std::int32_t, "int_column"_hash>()), -98);

  BOOST_CHECK_EQUAL((t1.component<float, "float_column"_hash>()), -11.2f);
  BOOST_CHECK_EQUAL((t2.component<float, "float_column"_hash>()), 42.4f);
  BOOST_CHECK_EQUAL((t3.component<float, "float_column"_hash>()), -98.9f);
}

BOOST_AUTO_TEST_SUITE_END()
