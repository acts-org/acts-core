// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/FullBilloirVertexFitter.hpp"
#include "Acts/Vertexing/HelicalTrackLinearizer.hpp"
#include "Acts/Vertexing/ImpactPointEstimator.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"
#include "Acts/Vertexing/ZScanVertexFinder.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

using Covariance = BoundSquareMatrix;
using Propagator = Acts::Propagator<EigenStepper<>>;

// Create a test context
GeometryContext geoContext = GeometryContext();
MagneticFieldContext magFieldContext = MagneticFieldContext();

// Vertex x/y position distribution
std::uniform_real_distribution<double> vXYDist(-0.1_mm, 0.1_mm);
// Vertex z position distribution
std::uniform_real_distribution<double> vZDist(-20_mm, 20_mm);
// Track d0 distribution
std::uniform_real_distribution<double> d0Dist(-0.01_mm, 0.01_mm);
// Track z0 distribution
std::uniform_real_distribution<double> z0Dist(-0.2_mm, 0.2_mm);
// Track pT distribution
std::uniform_real_distribution<double> pTDist(0.4_GeV, 10_GeV);
// Track phi distribution
std::uniform_real_distribution<double> phiDist(-M_PI, M_PI);
// Track theta distribution
std::uniform_real_distribution<double> thetaDist(1.0, M_PI - 1.0);
// Track charge helper distribution
std::uniform_real_distribution<double> qDist(-1, 1);
// Track IP resolution distribution
std::uniform_real_distribution<double> resIPDist(0., 100_um);
// Track angular distribution
std::uniform_real_distribution<double> resAngDist(0., 0.1);
// Track q/p resolution distribution
std::uniform_real_distribution<double> resQoPDist(-0.01, 0.01);

///
/// @brief Unit test for ZScanVertexFinder
///
BOOST_AUTO_TEST_CASE(zscan_finder_test) {
  unsigned int nTests = 50;

  for (unsigned int iTest = 0; iTest < nTests; ++iTest) {
    // Number of tracks
    unsigned int nTracks = 30;

    // Set up RNG
    int mySeed = 31415;
    std::mt19937 gen(mySeed);

    // Set up constant B-Field
    auto bField = std::make_shared<ConstantBField>(Vector3{0.0, 0.0, 1_T});

    // Set up Eigenstepper
    EigenStepper<> stepper(bField);

    // Set up propagator with void navigator
    auto propagator = std::make_shared<Propagator>(stepper);

    // Create perigee surface
    std::shared_ptr<PerigeeSurface> perigeeSurface =
        Surface::makeShared<PerigeeSurface>(Vector3(0., 0., 0.));

    // Create position of vertex and perigee surface
    double x = vXYDist(gen);
    double y = vXYDist(gen);
    double z = vZDist(gen);

    // Calculate d0 and z0 corresponding to vertex position
    double d0_v = std::hypot(x, y);
    double z0_v = z;

    // Start constructing nTracks tracks in the following
    std::vector<BoundTrackParameters> tracks;

    // Construct random track emerging from vicinity of vertex position
    // Vector to store track objects used for vertex fit
    for (unsigned int iTrack = 0; iTrack < nTracks; iTrack++) {
      // Construct positive or negative charge randomly
      double q = qDist(gen) < 0 ? -1. : 1.;

      // Construct random track parameters
      BoundVector paramVec = BoundVector::Zero();
      paramVec[eBoundLoc0] = d0_v + d0Dist(gen);
      paramVec[eBoundLoc1] = z0_v + z0Dist(gen);
      paramVec[eBoundPhi] = phiDist(gen);
      paramVec[eBoundTheta] = thetaDist(gen);
      paramVec[eBoundQOverP] = q / pTDist(gen);

      // Resolutions
      double resD0 = resIPDist(gen);
      double resZ0 = resIPDist(gen);
      double resPh = resAngDist(gen);
      double resTh = resAngDist(gen);
      double resQp = resQoPDist(gen);

      // Fill vector of track objects with simple covariance matrix
      Covariance covMat;

      covMat << resD0 * resD0, 0., 0., 0., 0., 0., 0., resZ0 * resZ0, 0., 0.,
          0., 0., 0., 0., resPh * resPh, 0., 0., 0., 0., 0., 0., resTh * resTh,
          0., 0., 0., 0., 0., 0., resQp * resQp, 0., 0., 0., 0., 0., 0., 1.;

      tracks.emplace_back(perigeeSurface, paramVec, std::move(covMat),
                          ParticleHypothesis::pion());
    }

    std::vector<InputTrack> inputTracks;
    for (const auto& trk : tracks) {
      inputTracks.emplace_back(&trk);
    }

    using VertexFinder = ZScanVertexFinder;

    ImpactPointEstimator::Config ipEstimatorCfg(bField, propagator);
    ImpactPointEstimator ipEstimator(ipEstimatorCfg);

    VertexFinder::Config cfg(ipEstimator);
    cfg.extractParameters.connect<&InputTrack::extractParameters>();

    VertexFinder finder(cfg);

    VertexingOptions vertexingOptions(geoContext, magFieldContext);

    auto state = finder.makeState(magFieldContext);
    auto res = finder.find(inputTracks, vertexingOptions, state);

    BOOST_CHECK(res.ok());

    if (!res.ok()) {
      std::cout << res.error().message() << std::endl;
    }

    if (res.ok()) {
      BOOST_CHECK(!(*res).empty());
      Vector3 result = (*res).back().position();
      CHECK_CLOSE_ABS(result[eZ], z, 1_mm);
    }
  }
}

// Dummy user-defined InputTrackStub type
struct InputTrackStub {
  InputTrackStub(const BoundTrackParameters& params) : m_parameters(params) {}

  const BoundTrackParameters& parameters() const { return m_parameters; }

  // store e.g. link to original objects here

 private:
  BoundTrackParameters m_parameters;
};

///
/// @brief Unit test for ZScanVertexFinder with user-defined input track type
///
BOOST_AUTO_TEST_CASE(zscan_finder_usertrack_test) {
  unsigned int nTests = 50;

  for (unsigned int iTest = 0; iTest < nTests; ++iTest) {
    // Number of tracks
    unsigned int nTracks = 30;

    // Set up RNG
    int mySeed = 31415;
    std::mt19937 gen(mySeed);

    // Set up constant B-Field
    auto bField = std::make_shared<ConstantBField>(Vector3{0.0, 0.0, 1_T});

    // Set up Eigenstepper
    EigenStepper<> stepper(bField);

    // Set up propagator with void navigator
    auto propagator = std::make_shared<Propagator>(stepper);

    // Create perigee surface
    std::shared_ptr<PerigeeSurface> perigeeSurface =
        Surface::makeShared<PerigeeSurface>(Vector3(0., 0., 0.));

    // Create position of vertex and perigee surface
    double x = vXYDist(gen);
    double y = vXYDist(gen);
    double z = vZDist(gen);

    // Calculate d0 and z0 corresponding to vertex position
    double d0_v = std::hypot(x, y);
    double z0_v = z;

    // Start constructing nTracks tracks in the following
    std::vector<InputTrackStub> tracks;

    // Construct random track emerging from vicinity of vertex position
    // Vector to store track objects used for vertex fit
    for (unsigned int iTrack = 0; iTrack < nTracks; iTrack++) {
      // Construct positive or negative charge randomly
      double q = qDist(gen) < 0 ? -1. : 1.;

      // Construct random track parameters
      BoundVector paramVec;
      double z0track = z0_v + z0Dist(gen);
      paramVec << d0_v + d0Dist(gen), z0track, phiDist(gen), thetaDist(gen),
          q / pTDist(gen), 0.;

      // Resolutions
      double resD0 = resIPDist(gen);
      double resZ0 = resIPDist(gen);
      double resPh = resAngDist(gen);
      double resTh = resAngDist(gen);
      double resQp = resQoPDist(gen);

      // Fill vector of track objects with simple covariance matrix
      Covariance covMat;

      covMat << resD0 * resD0, 0., 0., 0., 0., 0., 0., resZ0 * resZ0, 0., 0.,
          0., 0., 0., 0., resPh * resPh, 0., 0., 0., 0., 0., 0., resTh * resTh,
          0., 0., 0., 0., 0., 0., resQp * resQp, 0., 0., 0., 0., 0., 0., 1.;

      tracks.emplace_back(BoundTrackParameters(perigeeSurface, paramVec,
                                               std::move(covMat),
                                               ParticleHypothesis::pion()));
    }

    std::vector<InputTrack> inputTracks;
    for (const auto& trk : tracks) {
      inputTracks.emplace_back(&trk);
    }

    using VertexFinder = ZScanVertexFinder;

    ImpactPointEstimator::Config ipEstimatorCfg(bField, propagator);
    ImpactPointEstimator ipEstimator(ipEstimatorCfg);

    VertexFinder::Config cfg(ipEstimator);

    // Create a custom std::function to extract BoundTrackParameters from
    // user-defined InputTrackStub
    auto extractParameters = [](const InputTrack& params) {
      return params.as<InputTrackStub>()->parameters();
    };

    cfg.extractParameters.connect(extractParameters);
    VertexFinder finder(cfg);
    auto state = finder.makeState(magFieldContext);

    VertexingOptions vertexingOptions(geoContext, magFieldContext);

    auto res = finder.find(inputTracks, vertexingOptions, state);

    BOOST_CHECK(res.ok());

    if (!res.ok()) {
      std::cout << res.error().message() << std::endl;
    }

    if (res.ok()) {
      BOOST_CHECK(!(*res).empty());
      Vector3 result = (*res).back().position();
      CHECK_CLOSE_ABS(result[eZ], z, 1_mm);
    }
  }
}

}  // namespace Acts::Test
