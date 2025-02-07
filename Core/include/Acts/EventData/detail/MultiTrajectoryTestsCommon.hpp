// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/detail/TestSourceLink.hpp"
#include "Acts/EventData/detail/TestTrackState.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"
#include "Acts/Utilities/HashedString.hpp"

#include <random>
#include <stdexcept>

namespace Acts::detail::Test {

template <typename factory_t>
class MultiTrajectoryTestsCommon {
  using ParametersVector = BoundTrackParameters::ParametersVector;
  using CovarianceMatrix = BoundTrackParameters::CovarianceMatrix;
  using Jacobian = BoundMatrix;

  using trajectory_t = typename factory_t::trajectory_t;
  using const_trajectory_t = typename factory_t::const_trajectory_t;

 private:
  factory_t m_factory;

 public:
  void testBuild() {
    constexpr TrackStatePropMask kMask = TrackStatePropMask::Predicted;

    // construct trajectory w/ multiple components
    trajectory_t t = m_factory.create();

    auto i0 = t.addTrackState(kMask);
    // trajectory bifurcates here into multiple hypotheses
    auto i1a = t.addTrackState(kMask, i0);
    auto i1b = t.addTrackState(kMask, i0);
    auto i2a = t.addTrackState(kMask, i1a);
    auto i2b = t.addTrackState(kMask, i1b);

    // print each trajectory component
    std::vector<std::size_t> act;
    auto collect = [&](auto p) {
      act.push_back(p.index());
      BOOST_CHECK(!p.hasCalibrated());
      BOOST_CHECK(!p.hasFiltered());
      BOOST_CHECK(!p.hasSmoothed());
      BOOST_CHECK(!p.hasJacobian());
      BOOST_CHECK(!p.hasProjector());
    };

    std::vector<std::size_t> exp = {i2a, i1a, i0};
    t.visitBackwards(i2a, collect);
    BOOST_CHECK_EQUAL_COLLECTIONS(act.begin(), act.end(), exp.begin(),
                                  exp.end());

    act.clear();
    for (const auto& p : t.reverseTrackStateRange(i2a)) {
      act.push_back(p.index());
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(act.begin(), act.end(), exp.begin(),
                                  exp.end());

    act.clear();
    exp = {i2b, i1b, i0};
    t.visitBackwards(i2b, collect);
    BOOST_CHECK_EQUAL_COLLECTIONS(act.begin(), act.end(), exp.begin(),
                                  exp.end());

    act.clear();
    for (const auto& p : t.reverseTrackStateRange(i2b)) {
      act.push_back(p.index());
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(act.begin(), act.end(), exp.begin(),
                                  exp.end());

    act.clear();
    t.applyBackwards(i2b, collect);
    BOOST_CHECK_EQUAL_COLLECTIONS(act.begin(), act.end(), exp.begin(),
                                  exp.end());

    auto r = t.reverseTrackStateRange(i2b);
    BOOST_CHECK_EQUAL(std::distance(r.begin(), r.end()), 3);

    // check const-correctness
    const auto& ct = t;
    std::vector<BoundVector> predicteds;
    // mutation in this loop works!
    for (auto p : t.reverseTrackStateRange(i2b)) {
      predicteds.push_back(BoundVector::Random());
      p.predicted() = predicteds.back();
    }
    std::vector<BoundVector> predictedsAct;
    for (const auto& p : ct.reverseTrackStateRange(i2b)) {
      predictedsAct.push_back(p.predicted());
      // mutation in this loop doesn't work: does not compile
      // p.predicted() = BoundVector::Random();
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(predictedsAct.begin(), predictedsAct.end(),
                                  predicteds.begin(), predicteds.end());

    {
      trajectory_t t2 = m_factory.create();
      auto ts = t2.makeTrackState(kMask);
      BOOST_CHECK_EQUAL(t2.size(), 1);
      auto ts2 = t2.makeTrackState(kMask, ts.index());
      BOOST_CHECK_EQUAL(t2.size(), 2);
      BOOST_CHECK_EQUAL(ts.previous(), MultiTrajectoryTraits::kInvalid);
      BOOST_CHECK_EQUAL(ts2.previous(), ts.index());
    }
  }

  void testClear() {
    constexpr TrackStatePropMask kMask = TrackStatePropMask::Predicted;
    trajectory_t t = m_factory.create();
    BOOST_CHECK_EQUAL(t.size(), 0);

    auto i0 = t.addTrackState(kMask);
    // trajectory bifurcates here into multiple hypotheses
    auto i1a = t.addTrackState(kMask, i0);
    auto i1b = t.addTrackState(kMask, i0);
    t.addTrackState(kMask, i1a);
    t.addTrackState(kMask, i1b);

    BOOST_CHECK_EQUAL(t.size(), 5);
    t.clear();
    BOOST_CHECK_EQUAL(t.size(), 0);
  }

  void testApplyWithAbort() {
    constexpr TrackStatePropMask kMask = TrackStatePropMask::Predicted;

    // construct trajectory with three components
    trajectory_t t = m_factory.create();
    auto i0 = t.addTrackState(kMask);
    auto i1 = t.addTrackState(kMask, i0);
    auto i2 = t.addTrackState(kMask, i1);

    std::size_t n = 0;
    t.applyBackwards(i2, [&](const auto&) {
      n++;
      return false;
    });
    BOOST_CHECK_EQUAL(n, 1u);

    n = 0;
    t.applyBackwards(i2, [&](const auto& ts) {
      n++;
      if (ts.index() == i1) {
        return false;
      }
      return true;
    });
    BOOST_CHECK_EQUAL(n, 2u);

    n = 0;
    t.applyBackwards(i2, [&](const auto&) {
      n++;
      return true;
    });
    BOOST_CHECK_EQUAL(n, 3u);
  }

  void testAddTrackStateWithBitMask() {
    using PM = TrackStatePropMask;
    using namespace Acts::HashedStringLiteral;

    trajectory_t t = m_factory.create();

    auto alwaysPresent = [](auto& ts) {
      BOOST_CHECK(ts.template has<"referenceSurface"_hash>());
      BOOST_CHECK(ts.template has<"measdim"_hash>());
      BOOST_CHECK(ts.template has<"chi2"_hash>());
      BOOST_CHECK(ts.template has<"pathLength"_hash>());
      BOOST_CHECK(ts.template has<"typeFlags"_hash>());
    };

    auto ts = t.getTrackState(t.addTrackState(PM::All));
    BOOST_CHECK(ts.hasPredicted());
    BOOST_CHECK(ts.hasFiltered());
    BOOST_CHECK(ts.hasSmoothed());
    BOOST_CHECK(!ts.hasCalibrated());
    BOOST_CHECK(ts.hasProjector());
    BOOST_CHECK(ts.hasJacobian());
    alwaysPresent(ts);
    ts.allocateCalibrated(5);
    BOOST_CHECK(ts.hasCalibrated());
    BOOST_CHECK_EQUAL(ts.template calibrated<5>(), ActsVector<5>::Zero());
    BOOST_CHECK_EQUAL(ts.template calibratedCovariance<5>(),
                      ActsSquareMatrix<5>::Zero());

    ts = t.getTrackState(t.addTrackState(PM::None));
    BOOST_CHECK(!ts.hasPredicted());
    BOOST_CHECK(!ts.hasFiltered());
    BOOST_CHECK(!ts.hasSmoothed());
    BOOST_CHECK(!ts.hasCalibrated());
    BOOST_CHECK(!ts.hasProjector());
    BOOST_CHECK(!ts.hasJacobian());
    alwaysPresent(ts);

    ts = t.getTrackState(t.addTrackState(PM::Predicted));
    BOOST_CHECK(ts.hasPredicted());
    BOOST_CHECK(!ts.hasFiltered());
    BOOST_CHECK(!ts.hasSmoothed());
    BOOST_CHECK(!ts.hasCalibrated());
    BOOST_CHECK(!ts.hasProjector());
    BOOST_CHECK(!ts.hasJacobian());
    alwaysPresent(ts);

    ts = t.getTrackState(t.addTrackState(PM::Filtered));
    BOOST_CHECK(!ts.hasPredicted());
    BOOST_CHECK(ts.hasFiltered());
    BOOST_CHECK(!ts.hasSmoothed());
    BOOST_CHECK(!ts.hasCalibrated());
    BOOST_CHECK(!ts.hasProjector());
    BOOST_CHECK(!ts.hasJacobian());
    alwaysPresent(ts);

    ts = t.getTrackState(t.addTrackState(PM::Smoothed));
    BOOST_CHECK(!ts.hasPredicted());
    BOOST_CHECK(!ts.hasFiltered());
    BOOST_CHECK(ts.hasSmoothed());
    BOOST_CHECK(!ts.hasCalibrated());
    BOOST_CHECK(!ts.hasProjector());
    BOOST_CHECK(!ts.hasJacobian());
    alwaysPresent(ts);

    ts = t.getTrackState(t.addTrackState(PM::Calibrated));
    BOOST_CHECK(!ts.hasPredicted());
    BOOST_CHECK(!ts.hasFiltered());
    BOOST_CHECK(!ts.hasSmoothed());
    BOOST_CHECK(!ts.hasCalibrated());
    BOOST_CHECK(ts.hasProjector());
    BOOST_CHECK(!ts.hasJacobian());
    ts.allocateCalibrated(5);
    BOOST_CHECK(ts.hasCalibrated());
    BOOST_CHECK_EQUAL(ts.template calibrated<5>(), ActsVector<5>::Zero());
    BOOST_CHECK_EQUAL(ts.template calibratedCovariance<5>(),
                      ActsSquareMatrix<5>::Zero());

    ts = t.getTrackState(t.addTrackState(PM::Jacobian));
    BOOST_CHECK(!ts.hasPredicted());
    BOOST_CHECK(!ts.hasFiltered());
    BOOST_CHECK(!ts.hasSmoothed());
    BOOST_CHECK(!ts.hasCalibrated());
    BOOST_CHECK(!ts.hasProjector());
    BOOST_CHECK(ts.hasJacobian());
    alwaysPresent(ts);
  }

  void testAddTrackStateComponents() {
    using PM = TrackStatePropMask;

    trajectory_t t = m_factory.create();

    auto ts = t.makeTrackState(PM::None);
    BOOST_CHECK(!ts.hasPredicted());
    BOOST_CHECK(!ts.hasFiltered());
    BOOST_CHECK(!ts.hasSmoothed());
    BOOST_CHECK(!ts.hasCalibrated());
    BOOST_CHECK(!ts.hasJacobian());

    ts.addComponents(PM::None);
    BOOST_CHECK(!ts.hasPredicted());
    BOOST_CHECK(!ts.hasFiltered());
    BOOST_CHECK(!ts.hasSmoothed());
    BOOST_CHECK(!ts.hasCalibrated());
    BOOST_CHECK(!ts.hasJacobian());

    ts.addComponents(PM::Predicted);
    BOOST_CHECK(ts.hasPredicted());
    BOOST_CHECK(!ts.hasFiltered());
    BOOST_CHECK(!ts.hasSmoothed());
    BOOST_CHECK(!ts.hasCalibrated());
    BOOST_CHECK(!ts.hasJacobian());

    ts.addComponents(PM::Filtered);
    BOOST_CHECK(ts.hasPredicted());
    BOOST_CHECK(ts.hasFiltered());
    BOOST_CHECK(!ts.hasSmoothed());
    BOOST_CHECK(!ts.hasCalibrated());
    BOOST_CHECK(!ts.hasJacobian());

    ts.addComponents(PM::Smoothed);
    BOOST_CHECK(ts.hasPredicted());
    BOOST_CHECK(ts.hasFiltered());
    BOOST_CHECK(ts.hasSmoothed());
    BOOST_CHECK(!ts.hasCalibrated());
    BOOST_CHECK(!ts.hasJacobian());

    ts.addComponents(PM::Calibrated);
    ts.allocateCalibrated(5);
    BOOST_CHECK(ts.hasPredicted());
    BOOST_CHECK(ts.hasFiltered());
    BOOST_CHECK(ts.hasSmoothed());
    BOOST_CHECK(ts.hasCalibrated());
    BOOST_CHECK(!ts.hasJacobian());
    BOOST_CHECK_EQUAL(ts.template calibrated<5>(), ActsVector<5>::Zero());
    BOOST_CHECK_EQUAL(ts.template calibratedCovariance<5>(),
                      ActsSquareMatrix<5>::Zero());

    ts.addComponents(PM::Jacobian);
    BOOST_CHECK(ts.hasPredicted());
    BOOST_CHECK(ts.hasFiltered());
    BOOST_CHECK(ts.hasSmoothed());
    BOOST_CHECK(ts.hasCalibrated());
    BOOST_CHECK(ts.hasJacobian());

    ts.addComponents(PM::All);
    BOOST_CHECK(ts.hasPredicted());
    BOOST_CHECK(ts.hasFiltered());
    BOOST_CHECK(ts.hasSmoothed());
    BOOST_CHECK(ts.hasCalibrated());
    BOOST_CHECK(ts.hasJacobian());
  }

  void testTrackStateProxyCrossTalk(std::default_random_engine& rng) {
    TestTrackState pc(rng, 2u);

    // multi trajectory w/ a single, fully set, track state
    trajectory_t traj = m_factory.create();
    std::size_t index = traj.addTrackState();
    {
      auto ts = traj.getTrackState(index);
      fillTrackState<trajectory_t>(pc, TrackStatePropMask::All, ts);
    }
    // get two TrackStateProxies that reference the same data
    auto tsa = traj.getTrackState(index);
    auto tsb = traj.getTrackState(index);
    // then modify one and check that the other was modified as well
    {
      auto [par, cov] = generateBoundParametersCovariance(rng, {});
      tsb.predicted() = par;
      tsb.predictedCovariance() = cov;
      BOOST_CHECK_EQUAL(tsa.predicted(), par);
      BOOST_CHECK_EQUAL(tsa.predictedCovariance(), cov);
      BOOST_CHECK_EQUAL(tsb.predicted(), par);
      BOOST_CHECK_EQUAL(tsb.predictedCovariance(), cov);
    }
    {
      auto [par, cov] = generateBoundParametersCovariance(rng, {});
      tsb.filtered() = par;
      tsb.filteredCovariance() = cov;
      BOOST_CHECK_EQUAL(tsa.filtered(), par);
      BOOST_CHECK_EQUAL(tsa.filteredCovariance(), cov);
      BOOST_CHECK_EQUAL(tsb.filtered(), par);
      BOOST_CHECK_EQUAL(tsb.filteredCovariance(), cov);
    }
    {
      auto [par, cov] = generateBoundParametersCovariance(rng, {});
      tsb.smoothed() = par;
      tsb.smoothedCovariance() = cov;
      BOOST_CHECK_EQUAL(tsa.smoothed(), par);
      BOOST_CHECK_EQUAL(tsa.smoothedCovariance(), cov);
      BOOST_CHECK_EQUAL(tsb.smoothed(), par);
      BOOST_CHECK_EQUAL(tsb.smoothedCovariance(), cov);
    }
    {
      // create a new (invalid) source link
      TestSourceLink invalid;
      invalid.sourceId = -1;
      BOOST_CHECK_NE(
          tsa.getUncalibratedSourceLink().template get<TestSourceLink>(),
          invalid);
      BOOST_CHECK_NE(
          tsb.getUncalibratedSourceLink().template get<TestSourceLink>(),
          invalid);
      tsb.setUncalibratedSourceLink(SourceLink{invalid});
      BOOST_CHECK_EQUAL(
          tsa.getUncalibratedSourceLink().template get<TestSourceLink>(),
          invalid);
      BOOST_CHECK_EQUAL(
          tsb.getUncalibratedSourceLink().template get<TestSourceLink>(),
          invalid);
    }
    {
      // reset measurements w/ full parameters
      auto [measPar, measCov] = generateBoundParametersCovariance(rng, {});
      // Explicitly unset to avoid error below
      tsb.unset(TrackStatePropMask::Calibrated);
      tsb.allocateCalibrated(eBoundSize);
      BOOST_CHECK_EQUAL(tsb.template calibrated<eBoundSize>(),
                        BoundVector::Zero());
      BOOST_CHECK_EQUAL(tsb.template calibratedCovariance<eBoundSize>(),
                        BoundMatrix::Zero());
      tsb.template calibrated<eBoundSize>() = measPar;
      tsb.template calibratedCovariance<eBoundSize>() = measCov;
      BOOST_CHECK_EQUAL(tsa.template calibrated<eBoundSize>(), measPar);
      BOOST_CHECK_EQUAL(tsa.template calibratedCovariance<eBoundSize>(),
                        measCov);
      BOOST_CHECK_EQUAL(tsb.template calibrated<eBoundSize>(), measPar);
      BOOST_CHECK_EQUAL(tsb.template calibratedCovariance<eBoundSize>(),
                        measCov);
    }
    {
      // reset only the effective measurements
      auto [measPar, measCov] = generateBoundParametersCovariance(rng, {});
      std::size_t nMeasurements = tsb.effectiveCalibrated().rows();
      auto effPar = measPar.head(nMeasurements);
      auto effCov = measCov.topLeftCorner(nMeasurements, nMeasurements);
      tsb.allocateCalibrated(
          eBoundSize);  // no allocation, but we expect it to be reset to zero
                        // with this overload
      BOOST_CHECK_EQUAL(tsa.effectiveCalibrated(), BoundVector::Zero());
      BOOST_CHECK_EQUAL(tsa.effectiveCalibratedCovariance(),
                        BoundMatrix::Zero());
      BOOST_CHECK_EQUAL(tsa.effectiveCalibrated(), BoundVector::Zero());
      BOOST_CHECK_EQUAL(tsa.effectiveCalibratedCovariance(),
                        BoundMatrix::Zero());
      tsb.effectiveCalibrated() = effPar;
      tsb.effectiveCalibratedCovariance() = effCov;
      BOOST_CHECK_EQUAL(tsa.effectiveCalibrated(), effPar);
      BOOST_CHECK_EQUAL(tsa.effectiveCalibratedCovariance(), effCov);
      BOOST_CHECK_EQUAL(tsb.effectiveCalibrated(), effPar);
      BOOST_CHECK_EQUAL(tsb.effectiveCalibratedCovariance(), effCov);
    }
    {
      Jacobian jac = Jacobian::Identity();
      BOOST_CHECK_NE(tsa.jacobian(), jac);
      BOOST_CHECK_NE(tsb.jacobian(), jac);
      tsb.jacobian() = jac;
      BOOST_CHECK_EQUAL(tsa.jacobian(), jac);
      BOOST_CHECK_EQUAL(tsb.jacobian(), jac);
    }
    {
      tsb.chi2() = 98.0;
      BOOST_CHECK_EQUAL(tsa.chi2(), 98.0);
      BOOST_CHECK_EQUAL(tsb.chi2(), 98.0);
    }
    {
      tsb.pathLength() = 66.0;
      BOOST_CHECK_EQUAL(tsa.pathLength(), 66.0);
      BOOST_CHECK_EQUAL(tsb.pathLength(), 66.0);
    }
  }

  void testTrackStateReassignment(std::default_random_engine& rng) {
    TestTrackState pc(rng, 1u);

    trajectory_t t = m_factory.create();
    std::size_t index = t.addTrackState();
    auto ts = t.getTrackState(index);
    fillTrackState<trajectory_t>(pc, TrackStatePropMask::All, ts);

    // assert contents of original measurement (just to be safe)
    BOOST_CHECK_EQUAL(ts.calibratedSize(), 1u);
    BOOST_CHECK_EQUAL(ts.effectiveCalibrated(),
                      (pc.sourceLink.parameters.head<1>()));
    BOOST_CHECK_EQUAL(ts.effectiveCalibratedCovariance(),
                      (pc.sourceLink.covariance.topLeftCorner<1, 1>()));

    // use temporary measurement to reset calibrated data
    TestTrackState ttsb(rng, 2u);
    Acts::GeometryContext gctx;
    Acts::CalibrationContext cctx;
    BOOST_CHECK_EQUAL(
        ts.getUncalibratedSourceLink().template get<TestSourceLink>().sourceId,
        pc.sourceLink.sourceId);
    // Explicitly unset to avoid error below
    ts.unset(TrackStatePropMask::Calibrated);
    testSourceLinkCalibrator<trajectory_t>(gctx, cctx,
                                           SourceLink{ttsb.sourceLink}, ts);
    BOOST_CHECK_EQUAL(
        ts.getUncalibratedSourceLink().template get<TestSourceLink>().sourceId,
        ttsb.sourceLink.sourceId);

    BOOST_CHECK_EQUAL(ts.calibratedSize(), 2);
    BOOST_CHECK_EQUAL(ts.effectiveCalibrated(), ttsb.sourceLink.parameters);
    BOOST_CHECK_EQUAL(ts.effectiveCalibratedCovariance(),
                      ttsb.sourceLink.covariance);
  }

  void testTrackStateProxyStorage(std::default_random_engine& rng,
                                  std::size_t nMeasurements) {
    TestTrackState pc(rng, nMeasurements);

    // create trajectory with a single fully-filled random track state
    trajectory_t t = m_factory.create();
    std::size_t index = t.addTrackState();
    auto ts = t.getTrackState(index);
    fillTrackState<trajectory_t>(pc, TrackStatePropMask::All, ts);

    // check that the surface is correctly set
    BOOST_CHECK_EQUAL(&ts.referenceSurface(), pc.surface.get());
    BOOST_CHECK_EQUAL(ts.referenceSurface().geometryId(),
                      pc.sourceLink.m_geometryId);

    // check that the track parameters are set
    BOOST_CHECK(ts.hasPredicted());
    BOOST_CHECK_EQUAL(ts.predicted(), pc.predicted.parameters());
    BOOST_CHECK(pc.predicted.covariance().has_value());
    BOOST_CHECK_EQUAL(ts.predictedCovariance(), *pc.predicted.covariance());
    BOOST_CHECK(ts.hasFiltered());
    BOOST_CHECK_EQUAL(ts.filtered(), pc.filtered.parameters());
    BOOST_CHECK(pc.filtered.covariance().has_value());
    BOOST_CHECK_EQUAL(ts.filteredCovariance(), *pc.filtered.covariance());
    BOOST_CHECK(ts.hasSmoothed());
    BOOST_CHECK_EQUAL(ts.smoothed(), pc.smoothed.parameters());
    BOOST_CHECK(pc.smoothed.covariance().has_value());
    BOOST_CHECK_EQUAL(ts.smoothedCovariance(), *pc.smoothed.covariance());

    // check that the jacobian is set
    BOOST_CHECK(ts.hasJacobian());
    BOOST_CHECK_EQUAL(ts.jacobian(), pc.jacobian);
    BOOST_CHECK_EQUAL(ts.pathLength(), pc.pathLength);
    // check that chi2 is set
    BOOST_CHECK_EQUAL(ts.chi2(), static_cast<float>(pc.chi2));

    // check that the uncalibratedSourceLink source link is set
    BOOST_CHECK_EQUAL(
        ts.getUncalibratedSourceLink().template get<TestSourceLink>(),
        pc.sourceLink);

    // check that the calibrated measurement is set
    BOOST_CHECK(ts.hasCalibrated());
    BOOST_CHECK_EQUAL(ts.effectiveCalibrated(),
                      pc.sourceLink.parameters.head(nMeasurements));
    BOOST_CHECK_EQUAL(
        ts.effectiveCalibratedCovariance(),
        pc.sourceLink.covariance.topLeftCorner(nMeasurements, nMeasurements));
    {
      ParametersVector mParFull = ParametersVector::Zero();
      CovarianceMatrix mCovFull = CovarianceMatrix::Zero();
      mParFull.head(nMeasurements) =
          pc.sourceLink.parameters.head(nMeasurements);
      mCovFull.topLeftCorner(nMeasurements, nMeasurements) =
          pc.sourceLink.covariance.topLeftCorner(nMeasurements, nMeasurements);

      auto expMeas = pc.sourceLink.parameters.head(nMeasurements);
      auto expCov =
          pc.sourceLink.covariance.topLeftCorner(nMeasurements, nMeasurements);

      visit_measurement(ts.calibratedSize(), [&](auto N) {
        constexpr std::size_t measdim = decltype(N)::value;
        BOOST_CHECK_EQUAL(ts.template calibrated<measdim>(), expMeas);
        BOOST_CHECK_EQUAL(ts.template calibratedCovariance<measdim>(), expCov);
      });
    }
  }

  void testTrackStateProxyAllocations(std::default_random_engine& rng) {
    using namespace Acts::HashedStringLiteral;

    TestTrackState pc(rng, 2u);

    // this should allocate for all components in the trackstate, plus filtered
    trajectory_t t = m_factory.create();
    std::size_t i = t.addTrackState(TrackStatePropMask::Predicted |
                                    TrackStatePropMask::Filtered |
                                    TrackStatePropMask::Jacobian);
    auto tso = t.getTrackState(i);
    fillTrackState<trajectory_t>(pc, TrackStatePropMask::Predicted, tso);
    fillTrackState<trajectory_t>(pc, TrackStatePropMask::Filtered, tso);
    fillTrackState<trajectory_t>(pc, TrackStatePropMask::Jacobian, tso);

    BOOST_CHECK(tso.hasPredicted());
    BOOST_CHECK(tso.hasFiltered());
    BOOST_CHECK(!tso.hasSmoothed());
    BOOST_CHECK(!tso.hasCalibrated());
    BOOST_CHECK(tso.hasJacobian());

    auto tsnone = t.getTrackState(t.addTrackState(TrackStatePropMask::None));
    BOOST_CHECK(!tsnone.template has<"predicted"_hash>());
    BOOST_CHECK(!tsnone.template has<"filtered"_hash>());
    BOOST_CHECK(!tsnone.template has<"smoothed"_hash>());
    BOOST_CHECK(!tsnone.template has<"jacobian"_hash>());
    BOOST_CHECK(!tsnone.template has<"calibrated"_hash>());
    BOOST_CHECK(!tsnone.template has<"projector"_hash>());
    BOOST_CHECK(
        !tsnone.template has<"uncalibratedSourceLink"_hash>());  // separate
                                                                 // optional
                                                                 // mechanism
    BOOST_CHECK(tsnone.template has<"referenceSurface"_hash>());
    BOOST_CHECK(tsnone.template has<"measdim"_hash>());
    BOOST_CHECK(tsnone.template has<"chi2"_hash>());
    BOOST_CHECK(tsnone.template has<"pathLength"_hash>());
    BOOST_CHECK(tsnone.template has<"typeFlags"_hash>());

    auto tsall = t.getTrackState(t.addTrackState(TrackStatePropMask::All));
    BOOST_CHECK(tsall.template has<"predicted"_hash>());
    BOOST_CHECK(tsall.template has<"filtered"_hash>());
    BOOST_CHECK(tsall.template has<"smoothed"_hash>());
    BOOST_CHECK(tsall.template has<"jacobian"_hash>());
    BOOST_CHECK(!tsall.template has<"calibrated"_hash>());
    tsall.allocateCalibrated(5);
    BOOST_CHECK(tsall.template has<"calibrated"_hash>());
    BOOST_CHECK(tsall.template has<"projector"_hash>());
    BOOST_CHECK(!tsall.template has<
                 "uncalibratedSourceLink"_hash>());  // separate optional
                                                     // mechanism: nullptr
    BOOST_CHECK(tsall.template has<"referenceSurface"_hash>());
    BOOST_CHECK(tsall.template has<"measdim"_hash>());
    BOOST_CHECK(tsall.template has<"chi2"_hash>());
    BOOST_CHECK(tsall.template has<"pathLength"_hash>());
    BOOST_CHECK(tsall.template has<"typeFlags"_hash>());

    tsall.unset(TrackStatePropMask::Predicted);
    BOOST_CHECK(!tsall.template has<"predicted"_hash>());
    tsall.unset(TrackStatePropMask::Filtered);
    BOOST_CHECK(!tsall.template has<"filtered"_hash>());
    tsall.unset(TrackStatePropMask::Smoothed);
    BOOST_CHECK(!tsall.template has<"smoothed"_hash>());
    tsall.unset(TrackStatePropMask::Jacobian);
    BOOST_CHECK(!tsall.template has<"jacobian"_hash>());
    tsall.unset(TrackStatePropMask::Calibrated);
    BOOST_CHECK(!tsall.template has<"calibrated"_hash>());
  }

  void testTrackStateProxyGetMask() {
    using PM = TrackStatePropMask;

    std::array<PM, 5> values{PM::Predicted, PM::Filtered, PM::Smoothed,
                             PM::Jacobian, PM::Calibrated};
    PM all = std::accumulate(values.begin(), values.end(), PM::None,
                             [](auto a, auto b) { return a | b; });

    trajectory_t mj = m_factory.create();
    {
      auto ts = mj.getTrackState(mj.addTrackState(PM::All));
      // Calibrated is ignored because we haven't allocated yet
      BOOST_CHECK_EQUAL(ts.getMask(), (all & ~PM::Calibrated));
      ts.allocateCalibrated(4);
      BOOST_CHECK_EQUAL(ts.getMask(), all);
    }
    {
      auto ts =
          mj.getTrackState(mj.addTrackState(PM::Filtered | PM::Calibrated));
      // Calibrated is ignored because we haven't allocated yet
      BOOST_CHECK_EQUAL(ts.getMask(), PM::Filtered);
      ts.allocateCalibrated(4);
      BOOST_CHECK_EQUAL(ts.getMask(), (PM::Filtered | PM::Calibrated));
    }
    {
      auto ts = mj.getTrackState(
          mj.addTrackState(PM::Filtered | PM::Smoothed | PM::Predicted));
      BOOST_CHECK_EQUAL(ts.getMask(),
                        (PM::Filtered | PM::Smoothed | PM::Predicted));
    }
    {
      for (PM mask : values) {
        auto ts = mj.getTrackState(mj.addTrackState(mask));
        // Calibrated is ignored because we haven't allocated yet
        BOOST_CHECK_EQUAL(ts.getMask(), (mask & ~PM::Calibrated));
      }
    }
  }

  void testTrackStateProxyCopy(std::default_random_engine& rng) {
    using PM = TrackStatePropMask;

    std::array<PM, 4> values{PM::Predicted, PM::Filtered, PM::Smoothed,
                             PM::Jacobian};

    trajectory_t mj = m_factory.create();
    auto mkts = [&](PM mask) {
      auto r = mj.getTrackState(mj.addTrackState(mask));
      return r;
    };

    // orthogonal ones
    for (PM a : values) {
      for (PM b : values) {
        auto tsa = mkts(a);
        auto tsb = mkts(b);
        // doesn't work
        if (a != b) {
          BOOST_CHECK_THROW(tsa.copyFrom(tsb), std::runtime_error);
          BOOST_CHECK_THROW(tsb.copyFrom(tsa), std::runtime_error);
        } else {
          tsa.copyFrom(tsb);
          tsb.copyFrom(tsa);
        }
      }
    }

    {
      BOOST_TEST_CHECKPOINT("Calib auto alloc");
      auto tsa = mkts(PM::All);
      auto tsb = mkts(PM::All);
      tsb.allocateCalibrated(5);
      tsb.template calibrated<5>().setRandom();
      tsb.template calibratedCovariance<5>().setRandom();
      tsa.copyFrom(tsb, PM::All);
      BOOST_CHECK_EQUAL(tsa.template calibrated<5>(),
                        tsb.template calibrated<5>());
      BOOST_CHECK_EQUAL(tsa.template calibratedCovariance<5>(),
                        tsb.template calibratedCovariance<5>());
    }

    {
      BOOST_TEST_CHECKPOINT("Copy none");
      auto tsa = mkts(PM::All);
      auto tsb = mkts(PM::All);
      tsa.copyFrom(tsb, PM::None);
    }

    auto ts1 = mkts(PM::Filtered | PM::Predicted);  // this has both
    ts1.filtered().setRandom();
    ts1.filteredCovariance().setRandom();
    ts1.predicted().setRandom();
    ts1.predictedCovariance().setRandom();

    // ((src XOR dst) & src) == 0
    auto ts2 = mkts(PM::Predicted);
    ts2.predicted().setRandom();
    ts2.predictedCovariance().setRandom();

    // they are different before
    BOOST_CHECK_NE(ts1.predicted(), ts2.predicted());
    BOOST_CHECK_NE(ts1.predictedCovariance(), ts2.predictedCovariance());

    // ts1 -> ts2 fails
    BOOST_CHECK_THROW(ts2.copyFrom(ts1), std::runtime_error);
    BOOST_CHECK_NE(ts1.predicted(), ts2.predicted());
    BOOST_CHECK_NE(ts1.predictedCovariance(), ts2.predictedCovariance());

    // ts2 -> ts1 is ok
    ts1.copyFrom(ts2);
    BOOST_CHECK_EQUAL(ts1.predicted(), ts2.predicted());
    BOOST_CHECK_EQUAL(ts1.predictedCovariance(), ts2.predictedCovariance());

    std::size_t i0 = mj.addTrackState();
    std::size_t i1 = mj.addTrackState();
    ts1 = mj.getTrackState(i0);
    ts2 = mj.getTrackState(i1);
    TestTrackState rts1(rng, 1u);
    TestTrackState rts2(rng, 2u);
    fillTrackState<trajectory_t>(rts1, TrackStatePropMask::All, ts1);
    fillTrackState<trajectory_t>(rts2, TrackStatePropMask::All, ts2);

    auto ots1 = mkts(PM::All);
    auto ots2 = mkts(PM::All);
    // make full copy for later. We prove full copy works right below
    ots1.copyFrom(ts1);
    ots2.copyFrom(ts2);

    BOOST_CHECK_NE(ts1.predicted(), ts2.predicted());
    BOOST_CHECK_NE(ts1.predictedCovariance(), ts2.predictedCovariance());
    BOOST_CHECK_NE(ts1.filtered(), ts2.filtered());
    BOOST_CHECK_NE(ts1.filteredCovariance(), ts2.filteredCovariance());
    BOOST_CHECK_NE(ts1.smoothed(), ts2.smoothed());
    BOOST_CHECK_NE(ts1.smoothedCovariance(), ts2.smoothedCovariance());

    BOOST_CHECK_NE(
        ts1.getUncalibratedSourceLink().template get<TestSourceLink>(),
        ts2.getUncalibratedSourceLink().template get<TestSourceLink>());

    visit_measurement(ts1.calibratedSize(), [&](auto N) {
      constexpr std::size_t measdim = decltype(N)::value;
      BOOST_CHECK_NE(ts1.template calibrated<measdim>(),
                     ts2.template calibrated<measdim>());
      BOOST_CHECK_NE(ts1.template calibratedCovariance<measdim>(),
                     ts2.template calibratedCovariance<measdim>());
    });

    BOOST_CHECK_NE(ts1.calibratedSize(), ts2.calibratedSize());
    BOOST_CHECK(ts1.projectorSubspaceIndices() !=
                ts2.projectorSubspaceIndices());

    BOOST_CHECK_NE(ts1.jacobian(), ts2.jacobian());
    BOOST_CHECK_NE(ts1.chi2(), ts2.chi2());
    BOOST_CHECK_NE(ts1.pathLength(), ts2.pathLength());
    BOOST_CHECK_NE(&ts1.referenceSurface(), &ts2.referenceSurface());

    // Explicitly unset to avoid error below
    ts1.unset(TrackStatePropMask::Calibrated);
    ts1.copyFrom(ts2);

    BOOST_CHECK_EQUAL(ts1.predicted(), ts2.predicted());
    BOOST_CHECK_EQUAL(ts1.predictedCovariance(), ts2.predictedCovariance());
    BOOST_CHECK_EQUAL(ts1.filtered(), ts2.filtered());
    BOOST_CHECK_EQUAL(ts1.filteredCovariance(), ts2.filteredCovariance());
    BOOST_CHECK_EQUAL(ts1.smoothed(), ts2.smoothed());
    BOOST_CHECK_EQUAL(ts1.smoothedCovariance(), ts2.smoothedCovariance());

    BOOST_CHECK_EQUAL(
        ts1.getUncalibratedSourceLink().template get<TestSourceLink>(),
        ts2.getUncalibratedSourceLink().template get<TestSourceLink>());

    visit_measurement(ts1.calibratedSize(), [&](auto N) {
      constexpr std::size_t measdim = decltype(N)::value;
      BOOST_CHECK_EQUAL(ts1.template calibrated<measdim>(),
                        ts2.template calibrated<measdim>());
      BOOST_CHECK_EQUAL(ts1.template calibratedCovariance<measdim>(),
                        ts2.template calibratedCovariance<measdim>());
    });

    BOOST_CHECK_EQUAL(ts1.calibratedSize(), ts2.calibratedSize());
    BOOST_CHECK(ts1.projectorSubspaceIndices() ==
                ts2.projectorSubspaceIndices());

    BOOST_CHECK_EQUAL(ts1.jacobian(), ts2.jacobian());
    BOOST_CHECK_EQUAL(ts1.chi2(), ts2.chi2());
    BOOST_CHECK_EQUAL(ts1.pathLength(), ts2.pathLength());
    BOOST_CHECK_EQUAL(&ts1.referenceSurface(), &ts2.referenceSurface());

    // full copy proven to work. now let's do partial copy
    ts2 = mkts(PM::Predicted | PM::Jacobian | PM::Calibrated);
    ts2.copyFrom(ots2, PM::Predicted | PM::Jacobian | PM::Calibrated);
    // copy into empty ts, only copy some
    // explicitly unset to avoid error below
    ts1.unset(TrackStatePropMask::Calibrated);
    ts1.copyFrom(ots1);  // reset to original
    // is different again
    BOOST_CHECK_NE(ts1.predicted(), ts2.predicted());
    BOOST_CHECK_NE(ts1.predictedCovariance(), ts2.predictedCovariance());

    visit_measurement(ts1.calibratedSize(), [&](auto N) {
      constexpr std::size_t measdim = decltype(N)::value;
      BOOST_CHECK_NE(ts1.template calibrated<measdim>(),
                     ts2.template calibrated<measdim>());
      BOOST_CHECK_NE(ts1.template calibratedCovariance<measdim>(),
                     ts2.template calibratedCovariance<measdim>());
    });

    BOOST_CHECK_NE(ts1.calibratedSize(), ts2.calibratedSize());
    BOOST_CHECK(ts1.projectorSubspaceIndices() !=
                ts2.projectorSubspaceIndices());

    BOOST_CHECK_NE(ts1.jacobian(), ts2.jacobian());
    BOOST_CHECK_NE(ts1.chi2(), ts2.chi2());
    BOOST_CHECK_NE(ts1.pathLength(), ts2.pathLength());
    BOOST_CHECK_NE(&ts1.referenceSurface(), &ts2.referenceSurface());

    // Explicitly unset to avoid error below
    ts1.unset(TrackStatePropMask::Calibrated);
    ts1.copyFrom(ts2);

    // some components are same now
    BOOST_CHECK_EQUAL(ts1.predicted(), ts2.predicted());
    BOOST_CHECK_EQUAL(ts1.predictedCovariance(), ts2.predictedCovariance());

    visit_measurement(ts1.calibratedSize(), [&](auto N) {
      constexpr std::size_t measdim = decltype(N)::value;
      BOOST_CHECK_EQUAL(ts1.template calibrated<measdim>(),
                        ts2.template calibrated<measdim>());
      BOOST_CHECK_EQUAL(ts1.template calibratedCovariance<measdim>(),
                        ts2.template calibratedCovariance<measdim>());
    });

    BOOST_CHECK_EQUAL(ts1.calibratedSize(), ts2.calibratedSize());
    BOOST_CHECK(ts1.projectorSubspaceIndices() ==
                ts2.projectorSubspaceIndices());

    BOOST_CHECK_EQUAL(ts1.jacobian(), ts2.jacobian());
    BOOST_CHECK_EQUAL(ts1.chi2(), ts2.chi2());              // always copied
    BOOST_CHECK_EQUAL(ts1.pathLength(), ts2.pathLength());  // always copied
    BOOST_CHECK_EQUAL(&ts1.referenceSurface(),
                      &ts2.referenceSurface());  // always copied
  }

  void testTrackStateCopyDynamicColumns() {
    // mutable source
    trajectory_t mtj = m_factory.create();
    mtj.template addColumn<std::uint64_t>("counter");
    mtj.template addColumn<std::uint8_t>("odd");

    trajectory_t mtj2 = m_factory.create();
    // doesn't have the dynamic column

    trajectory_t mtj3 = m_factory.create();
    mtj3.template addColumn<std::uint64_t>("counter");
    mtj3.template addColumn<std::uint8_t>("odd");

    for (MultiTrajectoryTraits::IndexType i = 0; i < 10; i++) {
      auto ts =
          mtj.getTrackState(mtj.addTrackState(TrackStatePropMask::All, i));
      ts.template component<std::uint64_t>("counter") = i;
      ts.template component<std::uint8_t>("odd") = i % 2 == 0;

      auto ts2 =
          mtj2.getTrackState(mtj2.addTrackState(TrackStatePropMask::All, i));
      BOOST_CHECK_THROW(ts2.copyFrom(ts),
                        std::invalid_argument);  // this should fail

      auto ts3 =
          mtj3.getTrackState(mtj3.addTrackState(TrackStatePropMask::All, i));
      ts3.copyFrom(ts);  // this should work

      BOOST_CHECK_NE(ts3.index(), MultiTrajectoryTraits::kInvalid);

      BOOST_CHECK_EQUAL(ts.template component<std::uint64_t>("counter"),
                        ts3.template component<std::uint64_t>("counter"));
      BOOST_CHECK_EQUAL(ts.template component<std::uint8_t>("odd"),
                        ts3.template component<std::uint8_t>("odd"));
    }

    std::size_t before = mtj.size();
    const_trajectory_t cmtj{mtj};

    BOOST_REQUIRE_EQUAL(cmtj.size(), before);

    VectorMultiTrajectory mtj5;
    mtj5.addColumn<std::uint64_t>("counter");
    mtj5.addColumn<std::uint8_t>("odd");

    for (std::size_t i = 0; i < 10; i++) {
      auto ts4 = cmtj.getTrackState(i);  // const source!

      auto ts5 =
          mtj5.getTrackState(mtj5.addTrackState(TrackStatePropMask::All, 0));
      ts5.copyFrom(ts4);  // this should work

      BOOST_CHECK_NE(ts5.index(), MultiTrajectoryTraits::kInvalid);

      BOOST_CHECK_EQUAL(ts4.template component<std::uint64_t>("counter"),
                        ts5.template component<std::uint64_t>("counter"));
      BOOST_CHECK_EQUAL(ts4.template component<std::uint8_t>("odd"),
                        ts5.template component<std::uint8_t>("odd"));
    }
  }

  void testTrackStateProxyCopyDiffMTJ() {
    using PM = TrackStatePropMask;

    std::array<PM, 4> values{PM::Predicted, PM::Filtered, PM::Smoothed,
                             PM::Jacobian};

    trajectory_t mj = m_factory.create();
    trajectory_t mj2 = m_factory.create();
    auto mkts = [&](PM mask) {
      auto r = mj.getTrackState(mj.addTrackState(mask));
      return r;
    };
    auto mkts2 = [&](PM mask) {
      auto r = mj2.getTrackState(mj2.addTrackState(mask));
      return r;
    };

    // orthogonal ones
    for (PM a : values) {
      for (PM b : values) {
        auto tsa = mkts(a);
        auto tsb = mkts2(b);
        // doesn't work
        if (a != b) {
          BOOST_CHECK_THROW(tsa.copyFrom(tsb), std::runtime_error);
          BOOST_CHECK_THROW(tsb.copyFrom(tsa), std::runtime_error);
        } else {
          tsa.copyFrom(tsb);
          tsb.copyFrom(tsa);
        }
      }
    }

    // make sure they are actually on different MultiTrajectories
    BOOST_CHECK_EQUAL(mj.size(), values.size() * values.size());
    BOOST_CHECK_EQUAL(mj2.size(), values.size() * values.size());

    auto ts1 = mkts(PM::Filtered | PM::Predicted);  // this has both
    ts1.filtered().setRandom();
    ts1.filteredCovariance().setRandom();
    ts1.predicted().setRandom();
    ts1.predictedCovariance().setRandom();

    // ((src XOR dst) & src) == 0
    auto ts2 = mkts2(PM::Predicted);
    ts2.predicted().setRandom();
    ts2.predictedCovariance().setRandom();

    // they are different before
    BOOST_CHECK_NE(ts1.predicted(), ts2.predicted());
    BOOST_CHECK_NE(ts1.predictedCovariance(), ts2.predictedCovariance());

    // ts1 -> ts2 fails
    BOOST_CHECK_THROW(ts2.copyFrom(ts1), std::runtime_error);
    BOOST_CHECK_NE(ts1.predicted(), ts2.predicted());
    BOOST_CHECK_NE(ts1.predictedCovariance(), ts2.predictedCovariance());

    // ts2 -> ts1 is ok
    ts1.copyFrom(ts2);
    BOOST_CHECK_EQUAL(ts1.predicted(), ts2.predicted());
    BOOST_CHECK_EQUAL(ts1.predictedCovariance(), ts2.predictedCovariance());

    {
      BOOST_TEST_CHECKPOINT("Calib auto alloc");
      auto tsa = mkts(PM::All);
      auto tsb = mkts(PM::All);
      tsb.allocateCalibrated(5);
      tsb.template calibrated<5>().setRandom();
      tsb.template calibratedCovariance<5>().setRandom();
      tsa.copyFrom(tsb, PM::All);
      BOOST_CHECK_EQUAL(tsa.template calibrated<5>(),
                        tsb.template calibrated<5>());
      BOOST_CHECK_EQUAL(tsa.template calibratedCovariance<5>(),
                        tsb.template calibratedCovariance<5>());
    }

    {
      BOOST_TEST_CHECKPOINT("Copy none");
      auto tsa = mkts(PM::All);
      auto tsb = mkts(PM::All);
      tsa.copyFrom(tsb, PM::None);
    }
  }

  void testProxyAssignment() {
    constexpr TrackStatePropMask kMask = TrackStatePropMask::Predicted;
    trajectory_t t = m_factory.create();
    auto i0 = t.addTrackState(kMask);

    typename trajectory_t::TrackStateProxy tp = t.getTrackState(i0);  // mutable
    typename trajectory_t::TrackStateProxy tp2{tp};       // mutable to mutable
    typename trajectory_t::ConstTrackStateProxy tp3{tp};  // mutable to const
    // const to mutable: this won't compile
    // MultiTrajectory::TrackStateProxy tp4{tp3};
  }

  void testCopyFromConst() {
    // Check if the copy from const does compile, assume the copy is done
    // correctly

    using PM = TrackStatePropMask;
    trajectory_t mj = m_factory.create();

    const auto idx_a = mj.addTrackState(PM::All);
    const auto idx_b = mj.addTrackState(PM::All);

    typename trajectory_t::TrackStateProxy mutableProxy =
        mj.getTrackState(idx_a);

    const trajectory_t& cmj = mj;
    typename trajectory_t::ConstTrackStateProxy constProxy =
        cmj.getTrackState(idx_b);

    mutableProxy.copyFrom(constProxy);

    // copy mutable to const: this won't compile
    // constProxy.copyFrom(mutableProxy);
  }

  void testTrackStateProxyShare(std::default_random_engine& rng) {
    TestTrackState pc(rng, 2u);

    {
      trajectory_t traj = m_factory.create();
      std::size_t ia = traj.addTrackState(TrackStatePropMask::All);
      std::size_t ib = traj.addTrackState(TrackStatePropMask::None);

      auto tsa = traj.getTrackState(ia);
      auto tsb = traj.getTrackState(ib);

      fillTrackState<trajectory_t>(pc, TrackStatePropMask::All, tsa);

      BOOST_CHECK(tsa.hasPredicted());
      BOOST_CHECK(!tsb.hasPredicted());
      tsb.shareFrom(tsa, TrackStatePropMask::Predicted);
      BOOST_CHECK(tsa.hasPredicted());
      BOOST_CHECK(tsb.hasPredicted());
      BOOST_CHECK_EQUAL(tsa.predicted(), tsb.predicted());
      BOOST_CHECK_EQUAL(tsa.predictedCovariance(), tsb.predictedCovariance());

      BOOST_CHECK(tsa.hasFiltered());
      BOOST_CHECK(!tsb.hasFiltered());
      tsb.shareFrom(tsa, TrackStatePropMask::Filtered);
      BOOST_CHECK(tsa.hasFiltered());
      BOOST_CHECK(tsb.hasFiltered());
      BOOST_CHECK_EQUAL(tsa.filtered(), tsb.filtered());
      BOOST_CHECK_EQUAL(tsa.filteredCovariance(), tsb.filteredCovariance());

      BOOST_CHECK(tsa.hasSmoothed());
      BOOST_CHECK(!tsb.hasSmoothed());
      tsb.shareFrom(tsa, TrackStatePropMask::Smoothed);
      BOOST_CHECK(tsa.hasSmoothed());
      BOOST_CHECK(tsb.hasSmoothed());
      BOOST_CHECK_EQUAL(tsa.smoothed(), tsb.smoothed());
      BOOST_CHECK_EQUAL(tsa.smoothedCovariance(), tsb.smoothedCovariance());

      BOOST_CHECK(tsa.hasJacobian());
      BOOST_CHECK(!tsb.hasJacobian());
      tsb.shareFrom(tsa, TrackStatePropMask::Jacobian);
      BOOST_CHECK(tsa.hasJacobian());
      BOOST_CHECK(tsb.hasJacobian());
      BOOST_CHECK_EQUAL(tsa.jacobian(), tsb.jacobian());
    }

    {
      trajectory_t traj = m_factory.create();
      std::size_t i = traj.addTrackState(TrackStatePropMask::All &
                                         ~TrackStatePropMask::Filtered &
                                         ~TrackStatePropMask::Smoothed);

      auto ts = traj.getTrackState(i);

      BOOST_CHECK(ts.hasPredicted());
      BOOST_CHECK(!ts.hasFiltered());
      BOOST_CHECK(!ts.hasSmoothed());
      ts.predicted().setRandom();
      ts.predictedCovariance().setRandom();

      ts.shareFrom(TrackStatePropMask::Predicted, TrackStatePropMask::Filtered);
      BOOST_CHECK(ts.hasPredicted());
      BOOST_CHECK(ts.hasFiltered());
      BOOST_CHECK(!ts.hasSmoothed());
      BOOST_CHECK_EQUAL(ts.predicted(), ts.filtered());
      BOOST_CHECK_EQUAL(ts.predictedCovariance(), ts.filteredCovariance());

      ts.shareFrom(TrackStatePropMask::Predicted, TrackStatePropMask::Smoothed);
      BOOST_CHECK(ts.hasPredicted());
      BOOST_CHECK(ts.hasFiltered());
      BOOST_CHECK(ts.hasSmoothed());
      BOOST_CHECK_EQUAL(ts.predicted(), ts.filtered());
      BOOST_CHECK_EQUAL(ts.predicted(), ts.smoothed());
      BOOST_CHECK_EQUAL(ts.predictedCovariance(), ts.filteredCovariance());
      BOOST_CHECK_EQUAL(ts.predictedCovariance(), ts.smoothedCovariance());
    }
  }

  void testMultiTrajectoryExtraColumns() {
    using namespace HashedStringLiteral;

    auto test = [&](const std::string& col, auto value) {
      using T = decltype(value);
      std::string col2 = col + "_2";
      HashedString h{hashStringDynamic(col)};
      HashedString h2{hashStringDynamic(col2)};

      trajectory_t traj = m_factory.create();
      BOOST_CHECK(!traj.hasColumn(h));
      traj.template addColumn<T>(col);
      BOOST_CHECK(traj.hasColumn(h));

      BOOST_CHECK(!traj.hasColumn(h2));
      traj.template addColumn<T>(col2);
      BOOST_CHECK(traj.hasColumn(h2));

      auto ts1 = traj.getTrackState(traj.addTrackState());
      auto ts2 = traj.getTrackState(
          traj.addTrackState(TrackStatePropMask::All, ts1.index()));
      auto ts3 = traj.getTrackState(
          traj.addTrackState(TrackStatePropMask::All, ts2.index()));

      BOOST_CHECK(ts1.has(h));
      BOOST_CHECK(ts2.has(h));
      BOOST_CHECK(ts3.has(h));

      BOOST_CHECK(ts1.has(h2));
      BOOST_CHECK(ts2.has(h2));
      BOOST_CHECK(ts3.has(h2));

      ts1.template component<T>(col) = value;
      BOOST_CHECK_EQUAL(ts1.template component<T>(col), value);
    };

    test("std_uint32_t", std::uint32_t{1});
    test("std_uint64_t", std::uint64_t{2});
    test("std_int32_t", std::int32_t{-3});
    test("std_int64_t", std::int64_t{-4});
    test("float", float{8.9});
    test("double", double{656.2});

    trajectory_t traj = m_factory.create();
    traj.template addColumn<int>("extra_column");
    traj.template addColumn<float>("another_column");

    auto ts1 = traj.getTrackState(traj.addTrackState());
    auto ts2 = traj.getTrackState(
        traj.addTrackState(TrackStatePropMask::All, ts1.index()));
    auto ts3 = traj.getTrackState(
        traj.addTrackState(TrackStatePropMask::All, ts2.index()));

    BOOST_CHECK(ts1.template has<"extra_column"_hash>());
    BOOST_CHECK(ts2.template has<"extra_column"_hash>());
    BOOST_CHECK(ts3.template has<"extra_column"_hash>());

    BOOST_CHECK(ts1.template has<"another_column"_hash>());
    BOOST_CHECK(ts2.template has<"another_column"_hash>());
    BOOST_CHECK(ts3.template has<"another_column"_hash>());

    ts2.template component<int, "extra_column"_hash>() = 6;

    BOOST_CHECK_EQUAL((ts2.template component<int, "extra_column"_hash>()), 6);

    ts3.template component<float, "another_column"_hash>() = 7.2f;
    BOOST_CHECK_EQUAL((ts3.template component<float, "another_column"_hash>()),
                      7.2f);
  }

  void testMultiTrajectoryExtraColumnsRuntime() {
    auto runTest = [&](auto&& fn) {
      trajectory_t mt = m_factory.create();
      std::vector<std::string> columns = {"one", "two", "three", "four"};
      for (const auto& c : columns) {
        BOOST_CHECK(!mt.hasColumn(fn(c)));
        mt.template addColumn<int>(c);
        BOOST_CHECK(mt.hasColumn(fn(c)));
      }
      for (const auto& c : columns) {
        auto ts1 = mt.getTrackState(mt.addTrackState());
        auto ts2 = mt.getTrackState(mt.addTrackState());
        BOOST_CHECK(ts1.has(fn(c)));
        BOOST_CHECK(ts2.has(fn(c)));
        ts1.template component<int>(fn(c)) = 674;
        ts2.template component<int>(fn(c)) = 421;
        BOOST_CHECK_EQUAL(ts1.template component<int>(fn(c)), 674);
        BOOST_CHECK_EQUAL(ts2.template component<int>(fn(c)), 421);
      }
    };

    runTest([](const std::string& c) { return hashStringDynamic(c.c_str()); });
    // runTest([](const std::string& c) { return c.c_str(); });
    // runTest([](const std::string& c) { return c; });
    // runTest([](std::string_view c) { return c; });
  }

  void testMultiTrajectoryAllocateCalibratedInit(
      std::default_random_engine& rng) {
    trajectory_t traj = m_factory.create();
    auto ts = traj.makeTrackState(TrackStatePropMask::All);

    BOOST_CHECK_EQUAL(ts.calibratedSize(), MultiTrajectoryTraits::kInvalid);

    auto [par, cov] = generateBoundParametersCovariance(rng, {});

    ts.allocateCalibrated(par.head<3>(), cov.topLeftCorner<3, 3>());

    BOOST_CHECK_EQUAL(ts.calibratedSize(), 3);
    BOOST_CHECK_EQUAL(ts.template calibrated<3>(), par.head<3>());
    BOOST_CHECK_EQUAL(ts.template calibratedCovariance<3>(),
                      (cov.topLeftCorner<3, 3>()));

    auto [par2, cov2] = generateBoundParametersCovariance(rng, {});

    ts.allocateCalibrated(3);
    BOOST_CHECK_EQUAL(ts.template calibrated<3>(), Vector3::Zero());
    BOOST_CHECK_EQUAL(ts.template calibratedCovariance<3>(),
                      ActsSquareMatrix<3>::Zero());

    ts.allocateCalibrated(par2.head<3>(), cov2.topLeftCorner<3, 3>());
    BOOST_CHECK_EQUAL(ts.calibratedSize(), 3);
    // The values are re-assigned
    BOOST_CHECK_EQUAL(ts.template calibrated<3>(), par2.head<3>());
    BOOST_CHECK_EQUAL(ts.template calibratedCovariance<3>(),
                      (cov2.topLeftCorner<3, 3>()));

    // Re-allocation with a different measurement dimension is an error
    BOOST_CHECK_THROW(
        ts.allocateCalibrated(par2.head<4>(), cov2.topLeftCorner<4, 4>()),
        std::invalid_argument);
  }
};
}  // namespace Acts::detail::Test
