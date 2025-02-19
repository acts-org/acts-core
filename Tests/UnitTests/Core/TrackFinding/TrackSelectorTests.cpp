// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackStateType.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/TrackFinding/TrackSelector.hpp"
#include "Acts/Utilities/AngleHelpers.hpp"

#include <limits>
#include <numbers>

using namespace Acts;
namespace bdata = boost::unit_test::data;

struct MockTrack {
  static constexpr bool ReadOnly = true;
  using Container = VectorTrackContainer;
  using Trajectory = VectorMultiTrajectory;
  using IndexType = TrackIndexType;

  double m_theta;
  double m_phi;
  double m_pt;
  double m_loc0;
  double m_loc1;
  double m_time;
  std::size_t m_nMeasurements;
  std::size_t m_nHoles;
  std::size_t m_nOutliers;
  std::size_t m_nSharedHits;
  float m_chi2;

  bool hasReferenceSurface() const { return true; }
  double theta() const { return m_theta; }
  double phi() const { return m_phi; }
  double transverseMomentum() const { return m_pt; }
  double loc0() const { return m_loc0; }
  double loc1() const { return m_loc1; }
  double time() const { return m_time; }
  std::size_t nMeasurements() const { return m_nMeasurements; }
  std::size_t nHoles() const { return m_nHoles; }
  std::size_t nOutliers() const { return m_nOutliers; }
  std::size_t nSharedHits() const { return m_nSharedHits; }
  float chi2() const { return m_chi2; }

  // To comply with concept, not actually used
 private:
  struct MockTrackState {
    const Surface& referenceSurface() const {
      static const auto srf =
          CurvilinearSurface(Vector3::Zero(), Vector3::UnitZ()).planeSurface();
      return *srf;
    }

    ConstTrackStateType typeFlags() const {
      static const ConstTrackStateType::raw_type raw{0};
      return {raw};
    }
  };

  struct TrackStateRange {
    auto begin() const { return m_trackStates.begin(); }
    auto end() const { return m_trackStates.end(); }

   private:
    std::vector<MockTrackState> m_trackStates;
  };

 public:
  TrackStateRange trackStatesReversed() const { return {}; }
};

BOOST_AUTO_TEST_SUITE(TrackSelectorTests)

std::vector<double> etaValues{-5.0, -4.5, -4.0, -3.5, -3.0, -2.5, -2.0, -1.5,
                              -1.0, -0.5, 0.0,  0.5,  1.0,  1.5,  2.0,  2.5,
                              3.0,  3.5,  4.0,  4.5,  5.0,  1.0};

BOOST_DATA_TEST_CASE(TestSingleBinCase, bdata::make(etaValues), eta) {
  TrackSelector::EtaBinnedConfig cfgBase;

  MockTrack baseTrack{};
  baseTrack.m_theta = AngleHelpers::thetaFromEta(eta);
  baseTrack.m_phi = 0.5;
  baseTrack.m_pt = 0.5;
  baseTrack.m_loc0 = 0.5;
  baseTrack.m_loc1 = 0.5;
  baseTrack.m_time = 0.5;
  baseTrack.m_nMeasurements = 1;
  baseTrack.m_nHoles = 0;
  baseTrack.m_nOutliers = 0;
  baseTrack.m_nSharedHits = 0;
  baseTrack.m_chi2 = 0.0;

  {
    TrackSelector selector{cfgBase};
    // should select anything
    BOOST_CHECK(selector.isValidTrack(baseTrack));
  }

  auto check = [&](const auto& var, const auto& minPtr, const auto& maxPtr,
                   const auto& propPtr) {
    BOOST_TEST_INFO_SCOPE("Testing " << var);
    MockTrack track = baseTrack;

    auto cfgMinOnly = cfgBase;
    auto cfgMaxOnly = cfgBase;
    auto cfgMinMax = cfgBase;

    cfgMinOnly.cutSets.at(0).*minPtr = -1;
    cfgMinMax.cutSets.at(0).*minPtr = -1;
    cfgMaxOnly.cutSets.at(0).*maxPtr = 1;
    cfgMinMax.cutSets.at(0).*maxPtr = 1;

    TrackSelector minOnly{cfgMinOnly};
    TrackSelector maxOnly{cfgMaxOnly};
    TrackSelector minMax{cfgMinMax};

    BOOST_CHECK(minOnly.isValidTrack(track));
    BOOST_CHECK(maxOnly.isValidTrack(track));
    BOOST_CHECK(minMax.isValidTrack(track));

    // push track outside of minimum
    track.*propPtr = -1.1;

    BOOST_CHECK(!minOnly.isValidTrack(track));
    BOOST_CHECK(maxOnly.isValidTrack(track));
    BOOST_CHECK(!minMax.isValidTrack(track));

    // push track outside of maximum
    track.*propPtr = 1.1;

    BOOST_CHECK(minOnly.isValidTrack(track));
    BOOST_CHECK(!maxOnly.isValidTrack(track));
    BOOST_CHECK(!minMax.isValidTrack(track));
  };

  check("loc0", &TrackSelector::Config::loc0Min,
        &TrackSelector::Config::loc0Max, &MockTrack::m_loc0);

  check("loc1", &TrackSelector::Config::loc1Min,
        &TrackSelector::Config::loc1Max, &MockTrack::m_loc1);

  check("phi", &TrackSelector::Config::phiMin, &TrackSelector::Config::phiMax,
        &MockTrack::m_phi);

  check("time", &TrackSelector::Config::timeMin,
        &TrackSelector::Config::timeMax, &MockTrack::m_time);

  {
    BOOST_TEST_INFO_SCOPE("pt min");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).ptMin = {0.2};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_pt = 0.1;
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("pt max");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).ptMax = {1.0};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_pt = 1.1;
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("pt min max");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).ptMin = {0.2};
    cfg.cutSets.at(0).ptMax = {1.0};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_pt = 0.1;
    BOOST_CHECK(!selector.isValidTrack(track));
    track.m_pt = 1.1;
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("eta min");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).etaMin = {-1.0};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    track.m_theta = AngleHelpers::thetaFromEta(0.5);
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_theta = AngleHelpers::thetaFromEta(-1.1);
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("eta max");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).etaMax = {1.0};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    track.m_theta = AngleHelpers::thetaFromEta(0.5);
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_theta = AngleHelpers::thetaFromEta(1.1);
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("eta min max");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).etaMin = {-1.0};
    cfg.cutSets.at(0).etaMax = {1.0};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    track.m_theta = AngleHelpers::thetaFromEta(0.5);
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_theta = AngleHelpers::thetaFromEta(-1.1);
    BOOST_CHECK(!selector.isValidTrack(track));
    track.m_theta = AngleHelpers::thetaFromEta(1.1);
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("abs eta min");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).absEtaMin = {0.2};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    track.m_theta = AngleHelpers::thetaFromEta(0.5);
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_theta = AngleHelpers::thetaFromEta(-0.5);
    BOOST_CHECK(selector.isValidTrack(track));

    track.m_theta = AngleHelpers::thetaFromEta(0.1);
    BOOST_CHECK(!selector.isValidTrack(track));
    track.m_theta = AngleHelpers::thetaFromEta(-0.1);
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("abs eta max");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).absEtaMax = {1.0};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    track.m_theta = AngleHelpers::thetaFromEta(0.5);
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_theta = AngleHelpers::thetaFromEta(-0.5);
    BOOST_CHECK(selector.isValidTrack(track));

    track.m_theta = AngleHelpers::thetaFromEta(1.1);
    BOOST_CHECK(!selector.isValidTrack(track));
    track.m_theta = AngleHelpers::thetaFromEta(-1.1);
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("abs eta min max");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).absEtaMin = {0.2};
    cfg.cutSets.at(0).absEtaMax = {1.0};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    track.m_theta = AngleHelpers::thetaFromEta(0.5);
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_theta = AngleHelpers::thetaFromEta(-0.5);
    BOOST_CHECK(selector.isValidTrack(track));

    track.m_theta = AngleHelpers::thetaFromEta(0.1);
    BOOST_CHECK(!selector.isValidTrack(track));
    track.m_theta = AngleHelpers::thetaFromEta(-0.1);
    BOOST_CHECK(!selector.isValidTrack(track));

    track.m_theta = AngleHelpers::thetaFromEta(1.1);
    BOOST_CHECK(!selector.isValidTrack(track));
    track.m_theta = AngleHelpers::thetaFromEta(-1.1);
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("nMeas min");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).minMeasurements = {1};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    track.m_nMeasurements = {2};
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_nMeasurements = {1};
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_nMeasurements = {0};
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("nHoles max");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).maxHoles = {3};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    track.m_nHoles = {2};
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_nHoles = {3};
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_nHoles = {4};
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("nOutliers max");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).maxOutliers = {3};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    track.m_nOutliers = {2};
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_nOutliers = {3};
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_nOutliers = {4};
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("nSharedHits max");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).maxSharedHits = {3};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    track.m_nSharedHits = {2};
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_nSharedHits = {3};
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_nSharedHits = {4};
    BOOST_CHECK(!selector.isValidTrack(track));
  }

  {
    BOOST_TEST_INFO_SCOPE("nSharedHits max");
    auto cfg = cfgBase;
    cfg.cutSets.at(0).maxChi2 = {3};
    TrackSelector selector{cfg};
    MockTrack track = baseTrack;
    track.m_chi2 = {2};
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_chi2 = {3};
    BOOST_CHECK(selector.isValidTrack(track));
    track.m_chi2 = {4};
    BOOST_CHECK(!selector.isValidTrack(track));
  }
}

BOOST_AUTO_TEST_CASE(TestSingleBinEtaCutByBinEdge) {
  TrackSelector selector{TrackSelector::EtaBinnedConfig(1.0).addCuts(2.0)};

  BOOST_TEST_INFO_SCOPE(selector.config());

  MockTrack track{};
  track.m_theta = AngleHelpers::thetaFromEta(0.0);
  BOOST_CHECK(!selector.isValidTrack(track));

  track.m_theta = AngleHelpers::thetaFromEta(0.5);
  BOOST_CHECK(!selector.isValidTrack(track));

  // cannot easily check on-edge behavior because of floating point arithmetic
  // (it won't be exactly 1.0 in selector)
  track.m_theta = AngleHelpers::thetaFromEta(1.01);
  BOOST_CHECK(selector.isValidTrack(track));

  track.m_theta = AngleHelpers::thetaFromEta(1.5);
  BOOST_CHECK(selector.isValidTrack(track));

  track.m_theta = AngleHelpers::thetaFromEta(2.0);
  BOOST_CHECK(!selector.isValidTrack(track));
}

BOOST_AUTO_TEST_CASE(TestMultiBinCuts) {
  MockTrack baseTrack{};
  baseTrack.m_theta = AngleHelpers::thetaFromEta(1.0);
  baseTrack.m_phi = 0.5;
  baseTrack.m_pt = 0.5;
  baseTrack.m_loc0 = 0.5;
  baseTrack.m_loc1 = 0.5;
  baseTrack.m_time = 0.5;
  baseTrack.m_nMeasurements = 1;
  baseTrack.m_nHoles = 0;
  baseTrack.m_nOutliers = 0;
  baseTrack.m_nSharedHits = 0;
  baseTrack.m_chi2 = 0.0;

  using Config = TrackSelector::Config;

  using factory_ptr_t = Config& (Config::*)(double, double);
  using prop_ptr_t = double MockTrack::*;

  auto check = [&](const char* name, const factory_ptr_t& factory,
                   const prop_ptr_t& prop) {
    BOOST_TEST_CONTEXT(name) {
      auto cfg = TrackSelector::EtaBinnedConfig{0.0};

      cfg.addCuts(2.0, [&](auto& c) { (c.*factory)(-1.0, 1.0); })
          .addCuts([&](auto& c) { (c.*factory)(-2.0, 2.0); });

      TrackSelector selector{cfg};

      BOOST_TEST_INFO_SCOPE(cfg);

      {
        // exactly at zero
        MockTrack track = baseTrack;
        track.m_theta = AngleHelpers::thetaFromEta(0.0);

        BOOST_CHECK(selector.isValidTrack(track));

        track.*prop = -1.1;
        BOOST_CHECK(!selector.isValidTrack(track));

        track.*prop = 1.1;
        BOOST_CHECK(!selector.isValidTrack(track));
      }

      {
        // first bin
        MockTrack track = baseTrack;
        track.m_theta = AngleHelpers::thetaFromEta(1.0);

        BOOST_CHECK(selector.isValidTrack(track));

        track.*prop = -1.1;
        BOOST_CHECK(!selector.isValidTrack(track));

        track.*prop = 1.1;
        BOOST_CHECK(!selector.isValidTrack(track));
      }

      {
        // first bin edge
        MockTrack track = baseTrack;
        track.m_theta = AngleHelpers::thetaFromEta(
            2.0 - std::numeric_limits<double>::epsilon());

        BOOST_CHECK(selector.isValidTrack(track));

        track.*prop = -1.1;
        BOOST_CHECK(!selector.isValidTrack(track));

        track.*prop = 1.1;
        BOOST_CHECK(!selector.isValidTrack(track));
      }

      {
        // second bin lower edge
        MockTrack track = baseTrack;
        track.m_theta = AngleHelpers::thetaFromEta(2.0);

        BOOST_CHECK(selector.isValidTrack(track));

        track.*prop = -1.1;
        BOOST_CHECK(selector.isValidTrack(track));

        track.*prop = 1.1;
        BOOST_CHECK(selector.isValidTrack(track));

        track.*prop = -2.1;
        BOOST_CHECK(!selector.isValidTrack(track));

        track.*prop = 2.1;
        BOOST_CHECK(!selector.isValidTrack(track));
      }

      {
        // second bin
        MockTrack track = baseTrack;
        track.m_theta = AngleHelpers::thetaFromEta(10.0);

        track.*prop = -1.1;
        BOOST_CHECK(selector.isValidTrack(track));

        track.*prop = 1.1;
        BOOST_CHECK(selector.isValidTrack(track));

        track.*prop = -2.1;
        BOOST_CHECK(!selector.isValidTrack(track));

        track.*prop = 2.1;
        BOOST_CHECK(!selector.isValidTrack(track));
      }
    }
  };

  check("loc0", &Config::loc0, &MockTrack::m_loc0);
  check("loc1", &Config::loc1, &MockTrack::m_loc1);
  check("time", &Config::time, &MockTrack::m_time);
  check("phi", &Config::phi, &MockTrack::m_phi);
  check("pt", &Config::pt, &MockTrack::m_pt);
}

BOOST_AUTO_TEST_CASE(TestBinSelection) {
  using EtaBinnedConfig = TrackSelector::EtaBinnedConfig;
  constexpr double inf = std::numeric_limits<double>::infinity();

  {
    EtaBinnedConfig cfg{std::vector<double>{0, inf}};
    for (int i = -1; i <= 1; i = i + 2) {
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 0.0), 0);
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 1.0), 0);
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 2.0), 0);
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 3.0), 0);
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 10.0), 0);
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 1000.0), 0);
    }
  }

  {
    EtaBinnedConfig cfg{std::vector<double>{0, 0.5, 1.5, 2.5, 3.0, inf}};
    for (int i = -1; i <= 1; i = i + 2) {
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 0.0), 0);
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 1.0), 1);
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 2.0), 2);
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 3.0), 4);
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 10.0), 4);
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 1000.0), 4);
    }
  }

  {
    EtaBinnedConfig cfg{std::vector<double>{0, 1, 2}};
    for (int i = -1; i <= 1; i = i + 2) {
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 0.0), 0);
      BOOST_CHECK_EQUAL(cfg.binIndex(i * 1.0), 1);
      BOOST_CHECK_EQUAL(
          cfg.binIndex(i * (2.0 - std::numeric_limits<double>::epsilon())), 1);
      BOOST_CHECK_THROW(cfg.binIndex(i * 2.0), std::invalid_argument);
    }
  }
}

BOOST_AUTO_TEST_CASE(TestConstructor) {
  // valid multi bin construction
  {
    TrackSelector::EtaBinnedConfig cfg{std::vector<double>{0, 1, 4}};
    cfg.cutSets.at(0).ptMin = 0.9;
    cfg.cutSets.at(1).ptMin = 0.4;
    TrackSelector{cfg};
  }

  {
    // Track selector config with 2 eta bins
    TrackSelector::EtaBinnedConfig cfg{std::vector<double>{0, 1, 4}};

    // just the right amount!
    TrackSelector{cfg};

    // not enough cut cets
    cfg.cutSets.resize(1);
    BOOST_CHECK_THROW(TrackSelector{cfg}, std::invalid_argument);

    // too many cut sets
    cfg.cutSets.resize(3);
    BOOST_CHECK_THROW(TrackSelector{cfg}, std::invalid_argument);
  }

  // Constructor straight from cut config
  TrackSelector::Config cuts;
  TrackSelector selector{cuts};
  BOOST_CHECK_EQUAL(selector.config().absEtaEdges.size(), 2);

  {
    // Invalid sequence of chained construction
    auto cfg = TrackSelector::EtaBinnedConfig(0);
    cfg.addCuts(2.0, [](auto& c) { c.loc0(-1.0, 1.0); });
    BOOST_CHECK_THROW(cfg.addCuts(1.0), std::invalid_argument);
    BOOST_CHECK_THROW(TrackSelector::EtaBinnedConfig(0).addCuts(-2.0),
                      std::invalid_argument);
  }

  {
    auto cfg = TrackSelector::EtaBinnedConfig(1.0);

    cfg.addCuts(2.0, [](auto& c) { c.loc0(-1.0, 1.0); });
    BOOST_CHECK_EQUAL(cfg.nEtaBins(), 1);
    BOOST_CHECK_EQUAL(cfg.getCuts(1.5).loc0Min, -1.0);
    BOOST_CHECK_EQUAL(cfg.getCuts(1.5).loc0Max, 1.0);

    cfg.addCuts(3.0, [](auto& c) { c.loc0(-2.0, 2.0); });
    BOOST_CHECK_EQUAL(cfg.nEtaBins(), 2);
    BOOST_CHECK_EQUAL(cfg.getCuts(2.5).loc0Min, -2.0);
    BOOST_CHECK_EQUAL(cfg.getCuts(2.5).loc0Max, 2.0);

    cfg.addCuts(4.0, [](auto& c) { c.loc0(-3.0, 3.0); });
    BOOST_CHECK_EQUAL(cfg.nEtaBins(), 3);
    BOOST_CHECK_EQUAL(cfg.getCuts(3.5).loc0Min, -3.0);
    BOOST_CHECK_EQUAL(cfg.getCuts(3.5).loc0Max, 3.0);
  }
}

BOOST_AUTO_TEST_CASE(SubsetHitCountCut) {
  auto makeSurface = [](GeometryIdentifier id) {
    auto srf =
        CurvilinearSurface(Vector3::Zero(), Vector3::UnitZ()).planeSurface();

    srf->assignGeometryId(id);
    return srf;
  };

  auto addTrackState = [](auto& track, const auto& surface,
                          TrackStateFlag flag) {
    auto ts = track.appendTrackState();
    ts.setReferenceSurface(surface);
    ts.typeFlags().set(flag);
    return ts;
  };

  auto addMeasurement = [&](auto& track, const auto& surface) {
    return addTrackState(track, surface, TrackStateFlag::MeasurementFlag);
  };

  auto addMaterial = [&](auto& track, const auto& surface) {
    return addTrackState(track, surface, TrackStateFlag::MaterialFlag);
  };

  TrackContainer tc{VectorTrackContainer{}, VectorMultiTrajectory{}};

  auto makeTrack = [&]() {
    auto track = tc.makeTrack();

    using namespace Acts::UnitLiterals;
    track.parameters() << 0, 0, std::numbers::pi / 2., std::numbers::pi / 2.,
        1 / 1_GeV, 0;
    auto perigee = Surface::makeShared<PerigeeSurface>(Vector3::Zero());
    track.setReferenceSurface(perigee);
    return track;
  };

  auto vol7_lay3_sen2 = makeSurface(
      GeometryIdentifier{}.withVolume(7).withLayer(3).withSensitive(2));
  auto vol7_lay4 = makeSurface(GeometryIdentifier{}.withVolume(7).withLayer(4));
  auto vol7_lay3_sen8 = makeSurface(
      GeometryIdentifier{}.withVolume(7).withLayer(3).withSensitive(8));
  auto vol7_lay5_sen11 = makeSurface(
      GeometryIdentifier{}.withVolume(7).withLayer(5).withSensitive(11));
  auto vol7_lay5_sen12 = makeSurface(
      GeometryIdentifier{}.withVolume(7).withLayer(5).withSensitive(12));
  auto vol7_lay6_sen3 = makeSurface(
      GeometryIdentifier{}.withVolume(7).withLayer(6).withSensitive(3));

  auto vol8_lay8_sen1 = makeSurface(
      GeometryIdentifier{}.withVolume(8).withLayer(8).withSensitive(1));
  auto vol8_lay8_sen2 = makeSurface(
      GeometryIdentifier{}.withVolume(8).withLayer(8).withSensitive(2));
  auto vol8_lay9_sen1 = makeSurface(
      GeometryIdentifier{}.withVolume(8).withLayer(9).withSensitive(1));

  TrackSelector::Config cfgVol7;
  cfgVol7.measurementCounter.addCounter({GeometryIdentifier{}.withVolume(7)},
                                        3);
  TrackSelector selectorVol7{cfgVol7};

  auto trackVol7 = makeTrack();

  BOOST_CHECK(!selectorVol7.isValidTrack(trackVol7));

  // 1 hit in vol7
  addMeasurement(trackVol7, vol7_lay3_sen2);
  addMaterial(trackVol7, vol7_lay4);

  BOOST_CHECK(!selectorVol7.isValidTrack(trackVol7));
  addMeasurement(trackVol7, vol7_lay5_sen11);
  BOOST_CHECK(!selectorVol7.isValidTrack(trackVol7));

  // Now we should have enough hits
  addMeasurement(trackVol7, vol7_lay6_sen3);
  BOOST_CHECK(selectorVol7.isValidTrack(trackVol7));

  TrackSelector::Config cfgVol8;
  cfgVol8.measurementCounter.addCounter({GeometryIdentifier{}.withVolume(8)},
                                        2);
  TrackSelector selectorVol8{cfgVol8};

  // Previous trackVol7 has no measurements in volume 8
  BOOST_CHECK(!selectorVol8.isValidTrack(trackVol7));

  auto trackVol8 = makeTrack();
  BOOST_CHECK(!selectorVol8.isValidTrack(trackVol8));

  addMeasurement(trackVol8, vol8_lay8_sen1);
  BOOST_CHECK(!selectorVol8.isValidTrack(trackVol8));
  addMeasurement(trackVol8, vol8_lay8_sen2);
  BOOST_CHECK(selectorVol8.isValidTrack(trackVol8));
  addMeasurement(trackVol8, vol8_lay9_sen1);
  BOOST_CHECK(selectorVol8.isValidTrack(trackVol8));

  TrackSelector::Config cfgVol7Lay5;
  cfgVol7Lay5.measurementCounter.addCounter(
      {GeometryIdentifier{}.withVolume(7).withLayer(5)}, 2);
  TrackSelector selectorVol7Lay5{cfgVol7Lay5};

  // Only one hit on volume 7 layer 5
  BOOST_CHECK(!selectorVol7Lay5.isValidTrack(trackVol7));
  addMeasurement(trackVol7, vol7_lay5_sen12);
  BOOST_CHECK(selectorVol7Lay5.isValidTrack(trackVol7));

  // Check requirement on volume 7 OR 8
  TrackSelector::Config cfgVol7Or8;
  cfgVol7Or8.measurementCounter.addCounter(
      {GeometryIdentifier{}.withVolume(7), GeometryIdentifier{}.withVolume(8)},
      4);
  TrackSelector selectorVol7Or8{cfgVol7Or8};

  // threshold is 4
  // this track has enough hits in volume 7 only
  BOOST_CHECK(selectorVol7Or8.isValidTrack(trackVol7));
  // this track does not have enough hits in volume 8 only
  BOOST_CHECK(!selectorVol7Or8.isValidTrack(trackVol8));

  // add 1 hit in volume 7 to push it over the threshold
  addMeasurement(trackVol8, vol7_lay3_sen8);
  // now it passes
  BOOST_CHECK(selectorVol7Or8.isValidTrack(trackVol8));

  TrackSelector::Config cfgVol7And8;
  cfgVol7And8.measurementCounter.addCounter(
      {GeometryIdentifier{}.withVolume(7)}, 4);
  cfgVol7And8.measurementCounter.addCounter(
      {GeometryIdentifier{}.withVolume(8)}, 2);
  TrackSelector selectorVol7And8{cfgVol7And8};

  // this track has enough hits in vol 7 but not enough in vol 8
  BOOST_CHECK(!selectorVol7And8.isValidTrack(trackVol7));

  addMeasurement(trackVol7, vol8_lay8_sen1);
  addMeasurement(trackVol7, vol8_lay8_sen2);

  BOOST_CHECK(selectorVol7And8.isValidTrack(trackVol7));
}

BOOST_AUTO_TEST_SUITE_END()
