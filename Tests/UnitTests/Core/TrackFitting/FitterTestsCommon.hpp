// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/ProxyAccessor.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/EventData/detail/TestSourceLink.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Tests/CommonHelpers/CubicTrackingGeometry.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/MeasurementsCreator.hpp"
#include "Acts/TrackFitting/detail/KalmanGlobalCovariance.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <iterator>

using namespace Acts::UnitLiterals;
using namespace Acts::Test;

/// Find outliers using plain distance for testing purposes.
///
/// In a real setup, the outlier classification can be much more involved, e.g.
/// by computing the weighted distance/ local chi2 value. Here, the purpose is
/// to test that the basic principle works using simplified, synthetic data.
/// Thus, the simplest possible implementation should do.
struct TestOutlierFinder {
  double distanceMax = std::numeric_limits<double>::max();

  /// Classify a measurement as a valid one or an outlier.
  ///
  /// @tparam track_state_t Type of the track state
  /// @param state The track state to classify
  /// @retval False if the measurement is not an outlier
  /// @retval True if the measurement is an outlier
  template <typename traj_t>
  bool operator()(typename traj_t::ConstTrackStateProxy state) const {
    // can't determine an outlier w/o a measurement or predicted parameters
    if (!state.hasCalibrated() || !state.hasPredicted()) {
      return false;
    }
    auto subspaceHelper = state.projectorSubspaceHelper();
    auto projector =
        subspaceHelper.fullProjector()
            .topLeftCorner(state.calibratedSize(), Acts::eBoundSize)
            .eval();
    auto residuals =
        (state.effectiveCalibrated() - projector * state.predicted()).eval();
    auto distance = residuals.norm();
    return (distanceMax <= distance);
  }
};

/// Determine if the smoothing of a track should be done with or without reverse
/// filtering
struct TestReverseFilteringLogic {
  double momentumMax = std::numeric_limits<double>::max();

  /// Classify a measurement as a valid one or an outlier.
  ///
  /// @param trackState The trackState of the last measurement
  /// @retval False if we don't use the reverse filtering for the smoothing of the track
  /// @retval True if we use the reverse filtering for the smoothing of the track
  template <typename traj_t>
  bool operator()(typename traj_t::ConstTrackStateProxy state) const {
    // can't determine an outlier w/o a measurement or predicted parameters
    auto momentum = std::abs(1 / state.filtered()[Acts::eBoundQOverP]);
    std::cout << "momentum : " << momentum << std::endl;
    return (momentum <= momentumMax);
  }
};

// Construct a straight-line propagator.
auto makeStraightPropagator(std::shared_ptr<const Acts::TrackingGeometry> geo) {
  Acts::Navigator::Config cfg{std::move(geo)};
  cfg.resolvePassive = false;
  cfg.resolveMaterial = true;
  cfg.resolveSensitive = true;
  Acts::Navigator navigator(
      cfg, Acts::getDefaultLogger("Navigator", Acts::Logging::INFO));
  Acts::StraightLineStepper stepper;
  return Acts::Propagator<Acts::StraightLineStepper, Acts::Navigator>(
      stepper, std::move(navigator));
}

// Construct a propagator using a constant magnetic field along z.
template <typename stepper_t>
auto makeConstantFieldPropagator(
    std::shared_ptr<const Acts::TrackingGeometry> geo, double bz) {
  Acts::Navigator::Config cfg{std::move(geo)};
  cfg.resolvePassive = false;
  cfg.resolveMaterial = true;
  cfg.resolveSensitive = true;
  Acts::Navigator navigator(
      cfg, Acts::getDefaultLogger("Navigator", Acts::Logging::INFO));
  auto field =
      std::make_shared<Acts::ConstantBField>(Acts::Vector3(0.0, 0.0, bz));
  stepper_t stepper(std::move(field));
  return Acts::Propagator<decltype(stepper), Acts::Navigator>(
      std::move(stepper), std::move(navigator));
}

// Put all this in a struct to avoid that all these objects are exposed as
// global objects in the header
struct FitterTester {
  using Rng = std::default_random_engine;

  // Context objects
  Acts::GeometryContext geoCtx;
  Acts::MagneticFieldContext magCtx;
  Acts::CalibrationContext calCtx;

  // detector geometry
  CubicTrackingGeometry geometryStore{geoCtx};
  std::shared_ptr<const Acts::TrackingGeometry> geometry = geometryStore();

  Acts::detail::Test::TestSourceLink::SurfaceAccessor surfaceAccessor{
      *geometry};

  // expected number of measurements for the given detector
  constexpr static std::size_t nMeasurements = 6u;

  // detector resolutions
  MeasurementResolution resPixel = {MeasurementType::eLoc01, {25_um, 50_um}};
  MeasurementResolution resStrip0 = {MeasurementType::eLoc0, {100_um}};
  MeasurementResolution resStrip1 = {MeasurementType::eLoc1, {150_um}};
  MeasurementResolutionMap resolutions = {
      {Acts::GeometryIdentifier().setVolume(2), resPixel},
      {Acts::GeometryIdentifier().setVolume(3).setLayer(2), resStrip0},
      {Acts::GeometryIdentifier().setVolume(3).setLayer(4), resStrip1},
      {Acts::GeometryIdentifier().setVolume(3).setLayer(6), resStrip0},
      {Acts::GeometryIdentifier().setVolume(3).setLayer(8), resStrip1},
  };

  // simulation propagator
  Acts::Propagator<Acts::StraightLineStepper, Acts::Navigator> simPropagator =
      makeStraightPropagator(geometry);

  static std::vector<Acts::SourceLink> prepareSourceLinks(
      const std::vector<Acts::detail::Test::TestSourceLink>& sourceLinks) {
    std::vector<Acts::SourceLink> result;
    std::transform(sourceLinks.begin(), sourceLinks.end(),
                   std::back_inserter(result),
                   [](const auto& sl) { return Acts::SourceLink{sl}; });
    return result;
  }

  //////////////////////////
  // The testing functions
  //////////////////////////

  template <typename fitter_t, typename fitter_options_t, typename parameters_t>
  void test_ZeroFieldNoSurfaceForward(const fitter_t& fitter,
                                      fitter_options_t options,
                                      const parameters_t& start, Rng& rng,
                                      const bool expected_reversed,
                                      const bool expected_smoothed,
                                      const bool doDiag) const {
    auto measurements = createMeasurements(simPropagator, geoCtx, magCtx, start,
                                           resolutions, rng);

    auto sourceLinks = prepareSourceLinks(measurements.sourceLinks);
    BOOST_REQUIRE_EQUAL(sourceLinks.size(), nMeasurements);

    // this is the default option. set anyway for consistency
    options.referenceSurface = nullptr;

    Acts::ConstProxyAccessor<bool> reversed{"reversed"};
    Acts::ConstProxyAccessor<bool> smoothed{"smoothed"};

    auto doTest = [&](bool diag) {
      Acts::TrackContainer tracks{Acts::VectorTrackContainer{},
                                  Acts::VectorMultiTrajectory{}};
      if (diag) {
        tracks.addColumn<bool>("reversed");
        tracks.addColumn<bool>("smoothed");

        BOOST_CHECK(tracks.hasColumn("reversed"));
        BOOST_CHECK(tracks.hasColumn("smoothed"));
      }

      auto res = fitter.fit(sourceLinks.begin(), sourceLinks.end(), start,
                            options, tracks);
      BOOST_REQUIRE(res.ok());

      const auto track = res.value();
      BOOST_CHECK_NE(track.tipIndex(), Acts::MultiTrajectoryTraits::kInvalid);
      BOOST_CHECK(!track.hasReferenceSurface());
      BOOST_CHECK_EQUAL(track.nMeasurements(), sourceLinks.size());
      BOOST_CHECK_EQUAL(track.nHoles(), 0u);

      if (diag) {
        // check the output status flags
        BOOST_CHECK_EQUAL(reversed(track), expected_reversed);
        BOOST_CHECK_EQUAL(smoothed(track), expected_smoothed);
      }
    };

    if (doDiag) {
      doTest(true);
    }  // with reversed & smoothed columns
    doTest(false);  // without the extra columns
  }

  template <typename fitter_t, typename fitter_options_t, typename parameters_t>
  void test_ZeroFieldWithSurfaceForward(const fitter_t& fitter,
                                        fitter_options_t options,
                                        const parameters_t& start, Rng& rng,
                                        const bool expected_reversed,
                                        const bool expected_smoothed,
                                        const bool doDiag) const {
    auto measurements = createMeasurements(simPropagator, geoCtx, magCtx, start,
                                           resolutions, rng);
    auto sourceLinks = prepareSourceLinks(measurements.sourceLinks);
    BOOST_REQUIRE_EQUAL(sourceLinks.size(), nMeasurements);

    // initial fitter options configured for backward filtering mode
    // backward filtering requires a reference surface
    options.referenceSurface = &start.referenceSurface();
    // this is the default option. set anyway for consistency
    options.propagatorPlainOptions.direction = Acts::Direction::Forward();

    Acts::TrackContainer tracks{Acts::VectorTrackContainer{},
                                Acts::VectorMultiTrajectory{}};
    tracks.addColumn<bool>("reversed");
    tracks.addColumn<bool>("smoothed");

    auto res = fitter.fit(sourceLinks.begin(), sourceLinks.end(), start,
                          options, tracks);
    BOOST_REQUIRE(res.ok());

    const auto& track = res.value();
    BOOST_CHECK_NE(track.tipIndex(), Acts::MultiTrajectoryTraits::kInvalid);
    BOOST_CHECK(track.hasReferenceSurface());
    BOOST_CHECK_EQUAL(track.nMeasurements(), sourceLinks.size());
    BOOST_CHECK_EQUAL(track.nHoles(), 0u);

    BOOST_CHECK(tracks.hasColumn("reversed"));
    BOOST_CHECK(tracks.hasColumn("smoothed"));

    Acts::ConstProxyAccessor<bool> reversed{"reversed"};
    Acts::ConstProxyAccessor<bool> smoothed{"smoothed"};

    // check the output status flags
    if (doDiag) {
      BOOST_CHECK_EQUAL(smoothed(track), expected_smoothed);
      BOOST_CHECK_EQUAL(reversed(track), expected_reversed);
    }

    // count the number of `smoothed` states
    if (expected_reversed && expected_smoothed) {
      std::size_t nSmoothed = 0;
      for (const auto ts : track.trackStatesReversed()) {
        nSmoothed += ts.hasSmoothed();
      }
      BOOST_CHECK_EQUAL(nSmoothed, sourceLinks.size());
    }
  }

  template <typename fitter_t, typename fitter_options_t, typename parameters_t>
  void test_ZeroFieldWithSurfaceBackward(const fitter_t& fitter,
                                         fitter_options_t options,
                                         const parameters_t& start, Rng& rng,
                                         const bool expected_reversed,
                                         const bool expected_smoothed,
                                         const bool doDiag) const {
    auto measurements = createMeasurements(simPropagator, geoCtx, magCtx, start,
                                           resolutions, rng);
    auto sourceLinks = prepareSourceLinks(measurements.sourceLinks);
    BOOST_REQUIRE_EQUAL(sourceLinks.size(), nMeasurements);

    // create a track near the tracker exit for outward->inward filtering
    Acts::Vector4 posOuter = start.fourPosition(geoCtx);
    posOuter[Acts::ePos0] = 3_m;
    Acts::CurvilinearTrackParameters startOuter(
        posOuter, start.direction(), start.qOverP(), start.covariance(),
        Acts::ParticleHypothesis::pion());

    options.referenceSurface = &startOuter.referenceSurface();
    options.propagatorPlainOptions.direction = Acts::Direction::Backward();

    Acts::TrackContainer tracks{Acts::VectorTrackContainer{},
                                Acts::VectorMultiTrajectory{}};
    tracks.addColumn<bool>("reversed");
    tracks.addColumn<bool>("smoothed");

    auto res = fitter.fit(sourceLinks.begin(), sourceLinks.end(), startOuter,
                          options, tracks);
    BOOST_CHECK(res.ok());

    const auto& track = res.value();
    BOOST_CHECK_NE(track.tipIndex(), Acts::MultiTrajectoryTraits::kInvalid);
    BOOST_CHECK(track.hasReferenceSurface());
    BOOST_CHECK_EQUAL(track.nMeasurements(), sourceLinks.size());
    BOOST_CHECK_EQUAL(track.nHoles(), 0u);

    Acts::ConstProxyAccessor<bool> reversed{"reversed"};
    Acts::ConstProxyAccessor<bool> smoothed{"smoothed"};
    // check the output status flags
    if (doDiag) {
      BOOST_CHECK_EQUAL(smoothed(track), expected_smoothed);
      BOOST_CHECK_EQUAL(reversed(track), expected_reversed);
    }

    // count the number of `smoothed` states
    if (expected_reversed && expected_smoothed) {
      std::size_t nSmoothed = 0;
      for (const auto ts : track.trackStatesReversed()) {
        nSmoothed += ts.hasSmoothed();
      }
      BOOST_CHECK_EQUAL(nSmoothed, sourceLinks.size());
    }
  }

  template <typename fitter_t, typename fitter_options_t, typename parameters_t>
  void test_ZeroFieldWithSurfaceAtExit(const fitter_t& fitter,
                                       fitter_options_t options,
                                       const parameters_t& start, Rng& rng,
                                       const bool expected_reversed,
                                       const bool expected_smoothed,
                                       const bool doDiag) const {
    auto measurements = createMeasurements(simPropagator, geoCtx, magCtx, start,
                                           resolutions, rng);
    auto sourceLinks = prepareSourceLinks(measurements.sourceLinks);
    BOOST_REQUIRE_EQUAL(sourceLinks.size(), nMeasurements);

    // create a boundless target surface near the tracker exit
    Acts::Vector3 center(3._m, 0., 0.);
    Acts::Vector3 normal(1., 0., 0.);
    auto targetSurface =
        Acts::CurvilinearSurface(center, normal).planeSurface();

    options.referenceSurface = targetSurface.get();

    Acts::TrackContainer tracks{Acts::VectorTrackContainer{},
                                Acts::VectorMultiTrajectory{}};
    tracks.addColumn<bool>("reversed");
    tracks.addColumn<bool>("smoothed");

    auto res = fitter.fit(sourceLinks.begin(), sourceLinks.end(), start,
                          options, tracks);
    BOOST_REQUIRE(res.ok());

    const auto& track = res.value();
    BOOST_CHECK_NE(track.tipIndex(), Acts::MultiTrajectoryTraits::kInvalid);
    BOOST_CHECK(track.hasReferenceSurface());
    BOOST_CHECK_EQUAL(track.nMeasurements(), sourceLinks.size());
    BOOST_CHECK_EQUAL(track.nHoles(), 0u);

    Acts::ConstProxyAccessor<bool> reversed{"reversed"};
    Acts::ConstProxyAccessor<bool> smoothed{"smoothed"};

    // check the output status flags
    if (doDiag) {
      BOOST_CHECK_EQUAL(smoothed(track), expected_smoothed);
      BOOST_CHECK_EQUAL(reversed(track), expected_reversed);
    }
  }

  template <typename fitter_t, typename fitter_options_t, typename parameters_t>
  void test_ZeroFieldShuffled(const fitter_t& fitter, fitter_options_t options,
                              const parameters_t& start, Rng& rng,
                              const bool expected_reversed,
                              const bool expected_smoothed,
                              const bool doDiag) const {
    auto measurements = createMeasurements(simPropagator, geoCtx, magCtx, start,
                                           resolutions, rng);
    auto sourceLinks = prepareSourceLinks(measurements.sourceLinks);
    BOOST_REQUIRE_EQUAL(sourceLinks.size(), nMeasurements);

    options.referenceSurface = &start.referenceSurface();

    Acts::BoundVector parameters = Acts::BoundVector::Zero();

    Acts::TrackContainer tracks{Acts::VectorTrackContainer{},
                                Acts::VectorMultiTrajectory{}};
    tracks.addColumn<bool>("reversed");
    tracks.addColumn<bool>("smoothed");

    Acts::ConstProxyAccessor<bool> reversed{"reversed"};
    Acts::ConstProxyAccessor<bool> smoothed{"smoothed"};

    // fit w/ all hits in order
    {
      auto res = fitter.fit(sourceLinks.begin(), sourceLinks.end(), start,
                            options, tracks);
      BOOST_REQUIRE(res.ok());

      const auto& track = res.value();
      BOOST_CHECK_NE(track.tipIndex(), Acts::MultiTrajectoryTraits::kInvalid);
      BOOST_CHECK_EQUAL(track.nMeasurements(), sourceLinks.size());
      BOOST_REQUIRE(track.hasReferenceSurface());
      parameters = track.parameters();
      BOOST_CHECK_EQUAL(track.nHoles(), 0u);

      // check the output status flags
      if (doDiag) {
        BOOST_CHECK_EQUAL(smoothed(track), expected_smoothed);
        BOOST_CHECK_EQUAL(reversed(track), expected_reversed);
      }
    }
    // fit w/ all hits in random order
    {
      decltype(sourceLinks) shuffledSourceLinks = sourceLinks;
      std::shuffle(shuffledSourceLinks.begin(), shuffledSourceLinks.end(), rng);
      auto res = fitter.fit(shuffledSourceLinks.begin(),
                            shuffledSourceLinks.end(), start, options, tracks);
      BOOST_REQUIRE(res.ok());

      const auto& track = res.value();
      BOOST_CHECK_NE(track.tipIndex(), Acts::MultiTrajectoryTraits::kInvalid);
      BOOST_REQUIRE(track.hasReferenceSurface());
      // check consistency w/ un-shuffled measurements
      CHECK_CLOSE_ABS(track.parameters(), parameters, 1e-5);
      BOOST_CHECK_EQUAL(track.nMeasurements(), sourceLinks.size());
      // check the output status flags
      if (doDiag) {
        BOOST_CHECK_EQUAL(smoothed(track), expected_smoothed);
        BOOST_CHECK_EQUAL(reversed(track), expected_reversed);
      }
    }
  }

  template <typename fitter_t, typename fitter_options_t, typename parameters_t>
  void test_ZeroFieldWithHole(const fitter_t& fitter, fitter_options_t options,
                              const parameters_t& start, Rng& rng,
                              const bool expected_reversed,
                              const bool expected_smoothed,
                              const bool doDiag) const {
    auto measurements = createMeasurements(simPropagator, geoCtx, magCtx, start,
                                           resolutions, rng);
    auto sourceLinks = prepareSourceLinks(measurements.sourceLinks);
    BOOST_REQUIRE_EQUAL(sourceLinks.size(), nMeasurements);

    Acts::TrackContainer tracks{Acts::VectorTrackContainer{},
                                Acts::VectorMultiTrajectory{}};
    tracks.addColumn<bool>("reversed");
    tracks.addColumn<bool>("smoothed");

    Acts::ConstProxyAccessor<bool> reversed{"reversed"};
    Acts::ConstProxyAccessor<bool> smoothed{"smoothed"};

    // always keep the first and last measurement. leaving those in seems to not
    // count the respective surfaces as holes.
    for (std::size_t i = 1u; (i + 1u) < sourceLinks.size(); ++i) {
      // remove the i-th measurement
      auto withHole = sourceLinks;
      withHole.erase(std::next(withHole.begin(), i));
      BOOST_REQUIRE_EQUAL(withHole.size() + 1u, sourceLinks.size());
      BOOST_TEST_INFO("Removed measurement " << i);

      auto res =
          fitter.fit(withHole.begin(), withHole.end(), start, options, tracks);
      BOOST_REQUIRE(res.ok());

      const auto& track = res.value();
      BOOST_CHECK_NE(track.tipIndex(), Acts::MultiTrajectoryTraits::kInvalid);
      BOOST_REQUIRE(!track.hasReferenceSurface());
      BOOST_CHECK_EQUAL(track.nMeasurements(), withHole.size());
      // check the output status flags
      if (doDiag) {
        BOOST_CHECK_EQUAL(smoothed(track), expected_smoothed);
        BOOST_CHECK_EQUAL(reversed(track), expected_reversed);
      }
      BOOST_CHECK_EQUAL(track.nHoles(), 1u);
    }
    BOOST_CHECK_EQUAL(tracks.size(), sourceLinks.size() - 2);
  }

  template <typename fitter_t, typename fitter_options_t, typename parameters_t>
  void test_ZeroFieldWithOutliers(const fitter_t& fitter,
                                  fitter_options_t options,
                                  const parameters_t& start, Rng& rng,
                                  const bool expected_reversed,
                                  const bool expected_smoothed,
                                  const bool doDiag) const {
    auto measurements = createMeasurements(simPropagator, geoCtx, magCtx, start,
                                           resolutions, rng);
    auto sourceLinks = prepareSourceLinks(measurements.sourceLinks);
    auto outlierSourceLinks =
        prepareSourceLinks(measurements.outlierSourceLinks);
    BOOST_REQUIRE_EQUAL(sourceLinks.size(), nMeasurements);
    BOOST_REQUIRE_EQUAL(outlierSourceLinks.size(), nMeasurements);

    Acts::TrackContainer tracks{Acts::VectorTrackContainer{},
                                Acts::VectorMultiTrajectory{}};
    tracks.addColumn<bool>("reversed");
    tracks.addColumn<bool>("smoothed");

    Acts::ConstProxyAccessor<bool> reversed{"reversed"};
    Acts::ConstProxyAccessor<bool> smoothed{"smoothed"};

    for (std::size_t i = 0; i < sourceLinks.size(); ++i) {
      // replace the i-th measurement with an outlier
      auto withOutlier = sourceLinks;
      withOutlier[i] = outlierSourceLinks[i];
      BOOST_REQUIRE_EQUAL(withOutlier.size(), sourceLinks.size());
      BOOST_TEST_INFO("Replaced measurement " << i << " with outlier");

      auto res = fitter.fit(withOutlier.begin(), withOutlier.end(), start,
                            options, tracks);
      BOOST_REQUIRE(res.ok());

      const auto& track = res.value();
      BOOST_CHECK_NE(track.tipIndex(), Acts::MultiTrajectoryTraits::kInvalid);
      // count the number of outliers
      std::size_t nOutliers = 0;
      for (const auto state : track.trackStatesReversed()) {
        nOutliers += state.typeFlags().test(Acts::TrackStateFlag::OutlierFlag);
      }
      BOOST_CHECK_EQUAL(nOutliers, 1u);
      BOOST_REQUIRE(!track.hasReferenceSurface());
      BOOST_CHECK_EQUAL(track.nMeasurements(), withOutlier.size() - 1u);
      // check the output status flags
      if (doDiag) {
        BOOST_CHECK_EQUAL(smoothed(track), expected_smoothed);
        BOOST_CHECK_EQUAL(reversed(track), expected_reversed);
      }
      BOOST_CHECK_EQUAL(track.nHoles(), 0u);
    }
    BOOST_CHECK_EQUAL(tracks.size(), sourceLinks.size());
  }

  template <typename fitter_t, typename fitter_options_t, typename parameters_t>
  void test_ZeroFieldWithReverseFiltering(const fitter_t& fitter,
                                          fitter_options_t options,
                                          const parameters_t& start, Rng& rng,
                                          const bool expected_reversed,
                                          const bool expected_smoothed,
                                          const bool doDiag) const {
    auto measurements = createMeasurements(simPropagator, geoCtx, magCtx, start,
                                           resolutions, rng);

    Acts::TrackContainer tracks{Acts::VectorTrackContainer{},
                                Acts::VectorMultiTrajectory{}};
    tracks.addColumn<bool>("reversed");
    tracks.addColumn<bool>("smoothed");

    Acts::ConstProxyAccessor<bool> reversed{"reversed"};
    Acts::ConstProxyAccessor<bool> smoothed{"smoothed"};

    auto sourceLinks = prepareSourceLinks(measurements.sourceLinks);

    const auto& outlierSourceLinks = measurements.outlierSourceLinks;
    BOOST_REQUIRE_EQUAL(sourceLinks.size(), nMeasurements);
    BOOST_REQUIRE_EQUAL(outlierSourceLinks.size(), nMeasurements);

    // create a boundless target surface near the tracker entry
    Acts::Vector3 center(-3._m, 0., 0.);
    Acts::Vector3 normal(1., 0., 0.);
    auto targetSurface =
        Acts::CurvilinearSurface(center, normal).planeSurface();

    options.referenceSurface = targetSurface.get();

    auto res = fitter.fit(sourceLinks.begin(), sourceLinks.end(), start,
                          options, tracks);
    BOOST_REQUIRE(res.ok());
    const auto& track = res.value();

    // Track of 1 GeV with a threshold set at 0.1 GeV, reversed filtering should
    // not be used
    if (doDiag) {
      BOOST_CHECK_EQUAL(smoothed(track), expected_smoothed);
      BOOST_CHECK_EQUAL(reversed(track), expected_reversed);
    }
  }

  // TODO this is not really Kalman fitter specific. is probably better tested
  // with a synthetic trajectory.
  template <typename fitter_t, typename fitter_options_t, typename parameters_t>
  void test_GlobalCovariance(const fitter_t& fitter, fitter_options_t options,
                             const parameters_t& start, Rng& rng) const {
    auto measurements = createMeasurements(simPropagator, geoCtx, magCtx, start,
                                           resolutions, rng);
    auto sourceLinks = prepareSourceLinks(measurements.sourceLinks);
    BOOST_REQUIRE_EQUAL(sourceLinks.size(), nMeasurements);

    Acts::TrackContainer tracks{Acts::VectorTrackContainer{},
                                Acts::VectorMultiTrajectory{}};

    auto res = fitter.fit(sourceLinks.begin(), sourceLinks.end(), start,
                          options, tracks);
    BOOST_REQUIRE(res.ok());

    // Calculate global track parameters covariance matrix
    const auto& track = res.value();
    auto [trackParamsCov, stateRowIndices] =
        Acts::detail::globalTrackParametersCovariance(
            tracks.trackStateContainer(), track.tipIndex());
    BOOST_CHECK_EQUAL(trackParamsCov.rows(),
                      sourceLinks.size() * Acts::eBoundSize);
    BOOST_CHECK_EQUAL(stateRowIndices.size(), sourceLinks.size());
    // Each smoothed track state will have eBoundSize rows/cols in the global
    // covariance. stateRowIndices is a map of the starting row/index with the
    // state tip as the key. Thus, the last track state (i.e. the state
    // corresponding track.tipIndex()) has a starting row/index =
    // eBoundSize * (nMeasurements - 1), i.e. 6*(6-1) = 30.
    BOOST_CHECK_EQUAL(stateRowIndices.at(track.tipIndex()),
                      Acts::eBoundSize * (nMeasurements - 1));
  }
};
