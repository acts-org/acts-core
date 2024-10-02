// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/Charge.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
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
#include "Acts/Utilities/AnnealingUtility.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/AMVFInfo.hpp"
#include "Acts/Vertexing/AdaptiveMultiVertexFitter.hpp"
#include "Acts/Vertexing/HelicalTrackLinearizer.hpp"
#include "Acts/Vertexing/ImpactPointEstimator.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

using Acts::VectorHelpers::makeVector4;

// Set up logger
ACTS_LOCAL_LOGGER(Acts::getDefaultLogger("AMVFitterTests", Acts::Logging::INFO))

using Covariance = BoundSquareMatrix;
using Propagator = Acts::Propagator<EigenStepper<>>;
using Linearizer = HelicalTrackLinearizer;

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
std::uniform_real_distribution<double> pTDist(1._GeV, 30._GeV);
// Track phi distribution
std::uniform_real_distribution<double> phiDist(-M_PI, M_PI);
// Track theta distribution
std::uniform_real_distribution<double> thetaDist(1.0, M_PI - 1.0);
// Track charge helper distribution
std::uniform_real_distribution<double> qDist(-1, 1);
// Distribution of track time (relative to vertex time). Values are unrealistic
// and only used for testing purposes.
std::uniform_real_distribution<double> relTDist(-4_ps, 4_ps);
// Track IP resolution distribution
std::uniform_real_distribution<double> resIPDist(0., 100._um);
// Track angular distribution
std::uniform_real_distribution<double> resAngDist(0., 0.1);
// Track q/p resolution distribution
std::uniform_real_distribution<double> resQoPDist(-0.1, 0.1);
// Track time resolution distribution. Values are unrealistic and only used for
// testing purposes.
std::uniform_real_distribution<double> resTDist(0_ps, 8_ps);

/// @brief Unit test for AdaptiveMultiVertexFitter
///
BOOST_AUTO_TEST_CASE(adaptive_multi_vertex_fitter_test) {
  // Set up RNG
  int mySeed = 31415;
  std::mt19937 gen(mySeed);

  // Set up constant B-Field
  auto bField = std::make_shared<ConstantBField>(Vector3{0.0, 0.0, 1_T});

  // Set up EigenStepper
  EigenStepper<> stepper(bField);

  // Set up propagator with void navigator
  auto propagator = std::make_shared<Propagator>(stepper);

  VertexingOptions vertexingOptions(geoContext, magFieldContext);

  // IP 3D Estimator
  ImpactPointEstimator::Config ip3dEstCfg(bField, propagator);
  ImpactPointEstimator ip3dEst(ip3dEstCfg);

  // Linearizer for BoundTrackParameters type test
  Linearizer::Config ltConfig;
  ltConfig.bField = bField;
  ltConfig.propagator = propagator;
  Linearizer linearizer(ltConfig);

  AdaptiveMultiVertexFitter::Config fitterCfg(ip3dEst);
  fitterCfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(&linearizer);

  // Test smoothing
  fitterCfg.doSmoothing = true;
  fitterCfg.extractParameters.connect<&InputTrack::extractParameters>();

  AdaptiveMultiVertexFitter fitter(std::move(fitterCfg));

  // Create positions of three vertices, two of which (1 and 2) are
  // close to one another and will share a common track later
  Vector3 vtxPos1(-0.15_mm, -0.1_mm, -1.5_mm);
  Vector3 vtxPos2(-0.1_mm, -0.15_mm, -3._mm);
  Vector3 vtxPos3(0.2_mm, 0.2_mm, 10._mm);

  std::vector<Vector3> vtxPosVec{vtxPos1, vtxPos2, vtxPos3};

  // Resolutions, use the same for all tracks
  double resD0 = resIPDist(gen);
  double resZ0 = resIPDist(gen);
  double resPh = resAngDist(gen);
  double resTh = resAngDist(gen);
  double resQp = resQoPDist(gen);

  std::vector<Vertex> vtxList;
  for (auto& vtxPos : vtxPosVec) {
    Vertex vtx(vtxPos);
    // Set some vertex covariance
    SquareMatrix4 posCovariance(SquareMatrix4::Identity());
    vtx.setFullCovariance(posCovariance);
    // Add to vertex list
    vtxList.push_back(vtx);
  }

  std::vector<Vertex*> vtxPtrList;
  ACTS_DEBUG("All vertices in test case:");
  int cv = 0;
  for (auto& vtx : vtxList) {
    cv++;
    ACTS_DEBUG("\t" << cv << ". vertex ptr: " << &vtx);
    vtxPtrList.push_back(&vtx);
  }

  std::vector<BoundTrackParameters> allTracks;

  unsigned int nTracksPerVtx = 4;
  // Construct nTracksPerVtx * 3 (3 vertices) random track emerging
  // from vicinity of vertex positions
  for (unsigned int iTrack = 0; iTrack < nTracksPerVtx * vtxPosVec.size();
       iTrack++) {
    // Construct positive or negative charge randomly
    double q = qDist(gen) < 0 ? -1. : 1.;

    // Fill vector of track objects with simple covariance matrix
    Covariance covMat;

    covMat << resD0 * resD0, 0., 0., 0., 0., 0., 0., resZ0 * resZ0, 0., 0., 0.,
        0., 0., 0., resPh * resPh, 0., 0., 0., 0., 0., 0., resTh * resTh, 0.,
        0., 0., 0., 0., 0., resQp * resQp, 0., 0., 0., 0., 0., 0., 1.;

    // Index of current vertex
    int vtxIdx = static_cast<int>(iTrack / nTracksPerVtx);

    // Construct random track parameters
    BoundTrackParameters::ParametersVector paramVec;
    paramVec << d0Dist(gen), z0Dist(gen), phiDist(gen), thetaDist(gen),
        q / pTDist(gen), 0.;

    std::shared_ptr<PerigeeSurface> perigeeSurface =
        Surface::makeShared<PerigeeSurface>(vtxPosVec[vtxIdx]);

    allTracks.emplace_back(perigeeSurface, paramVec, std::move(covMat),
                           ParticleHypothesis::pion());
  }

  int ct = 0;
  ACTS_DEBUG("All tracks in test case:");
  for (auto& trk : allTracks) {
    ct++;
    ACTS_DEBUG("\t" << ct << ". track ptr: " << &trk);
  }

  AdaptiveMultiVertexFitter::State state(*bField, magFieldContext);

  for (unsigned int iTrack = 0; iTrack < nTracksPerVtx * vtxPosVec.size();
       iTrack++) {
    // Index of current vertex
    int vtxIdx = static_cast<int>(iTrack / nTracksPerVtx);

    InputTrack inputTrack{&allTracks[iTrack]};

    state.vtxInfoMap[&(vtxList[vtxIdx])].trackLinks.push_back(inputTrack);
    state.tracksAtVerticesMap.insert(
        std::make_pair(std::make_pair(inputTrack, &(vtxList[vtxIdx])),
                       TrackAtVertex(1., allTracks[iTrack], inputTrack)));

    // Use first track also for second vertex to let vtx1 and vtx2
    // share this track
    if (iTrack == 0) {
      state.vtxInfoMap[&(vtxList.at(1))].trackLinks.push_back(inputTrack);
      state.tracksAtVerticesMap.insert(
          std::make_pair(std::make_pair(inputTrack, &(vtxList.at(1))),
                         TrackAtVertex(1., allTracks[iTrack], inputTrack)));
    }
  }

  for (auto& vtx : vtxPtrList) {
    state.addVertexToMultiMap(*vtx);
    ACTS_DEBUG("Vertex, with ptr: " << vtx);
    for (auto& trk : state.vtxInfoMap[vtx].trackLinks) {
      ACTS_DEBUG("\t track ptr: " << trk);
    }
  }

  ACTS_DEBUG("Checking all vertices linked to a single track:");
  for (auto& trk : allTracks) {
    ACTS_DEBUG("Track with ptr: " << &trk);
    auto range = state.trackToVerticesMultiMap.equal_range(InputTrack{&trk});
    for (auto vtxIter = range.first; vtxIter != range.second; ++vtxIter) {
      ACTS_DEBUG("\t used by vertex: " << vtxIter->second);
    }
  }

  // Copy vertex seeds from state.vertexCollection to new
  // list in order to be able to compare later
  std::vector<Vertex> seedListCopy = vtxList;

  auto res1 = fitter.addVtxToFit(state, vtxList.at(0), vertexingOptions);
  ACTS_DEBUG("Tracks linked to each vertex AFTER fit:");
  int c = 0;
  for (auto& vtx : vtxPtrList) {
    c++;
    ACTS_DEBUG(c << ". vertex, with ptr: " << vtx);
    for (const auto& trk : state.vtxInfoMap[vtx].trackLinks) {
      ACTS_DEBUG("\t track ptr: " << trk);
    }
  }

  ACTS_DEBUG("Checking all vertices linked to a single track AFTER fit:");
  for (auto& trk : allTracks) {
    ACTS_DEBUG("Track with ptr: " << &trk);
    auto range = state.trackToVerticesMultiMap.equal_range(InputTrack{&trk});
    for (auto vtxIter = range.first; vtxIter != range.second; ++vtxIter) {
      ACTS_DEBUG("\t used by vertex: " << vtxIter->second);
    }
  }

  BOOST_CHECK(res1.ok());

  ACTS_DEBUG("Vertex positions after fit of vertex 1 and 2:");
  for (std::size_t vtxIter = 0; vtxIter < 3; vtxIter++) {
    ACTS_DEBUG("Vtx " << vtxIter + 1 << ", seed position:\n "
                      << seedListCopy.at(vtxIter).fullPosition()
                      << "\nFitted position:\n "
                      << vtxList.at(vtxIter).fullPosition());
  }

  // After fit of first vertex, only first and second vertex seed
  // should have been modified while third vertex should remain untouched
  BOOST_CHECK_NE(vtxList.at(0).fullPosition(),
                 seedListCopy.at(0).fullPosition());
  BOOST_CHECK_NE(vtxList.at(1).fullPosition(),
                 seedListCopy.at(1).fullPosition());
  BOOST_CHECK_EQUAL(vtxList.at(2).fullPosition(),
                    seedListCopy.at(2).fullPosition());

  CHECK_CLOSE_ABS(vtxList.at(0).fullPosition(),
                  seedListCopy.at(0).fullPosition(), 1_mm);
  CHECK_CLOSE_ABS(vtxList.at(1).fullPosition(),
                  seedListCopy.at(1).fullPosition(), 1_mm);

  auto res2 = fitter.addVtxToFit(state, vtxList.at(2), vertexingOptions);
  BOOST_CHECK(res2.ok());

  // Now also the third vertex should have been modified and fitted
  BOOST_CHECK_NE(vtxList.at(2).fullPosition(),
                 seedListCopy.at(2).fullPosition());
  CHECK_CLOSE_ABS(vtxList.at(2).fullPosition(),
                  seedListCopy.at(2).fullPosition(), 1_mm);

  ACTS_DEBUG("Vertex positions after fit of vertex 3:");
  ACTS_DEBUG("Vtx 1, seed position:\n " << seedListCopy.at(0).fullPosition()
                                        << "\nFitted position:\n "
                                        << vtxList.at(0).fullPosition());
  ACTS_DEBUG("Vtx 2, seed position:\n " << seedListCopy.at(1).fullPosition()
                                        << "\nFitted position:\n "
                                        << vtxList.at(1).fullPosition());
  ACTS_DEBUG("Vtx 3, seed position:\n " << seedListCopy.at(2).fullPosition()
                                        << "\nFitted position:\n "
                                        << vtxList.at(2).fullPosition());
}

/// @brief Unit test for fitting a 4D vertex position
///
BOOST_AUTO_TEST_CASE(time_fitting) {
  // Set up RNG
  int mySeed = 31415;
  std::mt19937 gen(mySeed);

  // Set up constant B-Field
  auto bField = std::make_shared<ConstantBField>(Vector3{0.0, 0.0, 1_T});

  // Set up EigenStepper
  EigenStepper<> stepper(bField);

  // Set up propagator with void navigator
  auto propagator = std::make_shared<Propagator>(stepper);

  VertexingOptions vertexingOptions(geoContext, magFieldContext);

  ImpactPointEstimator::Config ip3dEstCfg(bField, propagator);
  ImpactPointEstimator ip3dEst(ip3dEstCfg);

  AdaptiveMultiVertexFitter::Config fitterCfg(ip3dEst);

  // Linearizer for BoundTrackParameters type test
  Linearizer::Config ltConfig;
  ltConfig.bField = bField;
  ltConfig.propagator = propagator;
  Linearizer linearizer(ltConfig);

  // Test smoothing
  fitterCfg.doSmoothing = true;
  // Do time fit
  fitterCfg.useTime = true;
  fitterCfg.extractParameters.connect<&InputTrack::extractParameters>();
  fitterCfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(&linearizer);

  AdaptiveMultiVertexFitter fitter(std::move(fitterCfg));

  // Vertex position
  double trueVtxTime = 40.0_ps;
  Vector3 trueVtxPos(-0.15_mm, -0.1_mm, -1.5_mm);

  // Seed position of the vertex
  Vector4 vtxSeedPos(0.0_mm, 0.0_mm, -1.4_mm, 0.0_ps);

  Vertex vtx(vtxSeedPos);
  // Set initial covariance matrix to a large value
  SquareMatrix4 initialCovariance(SquareMatrix4::Identity() * 1e+8);
  vtx.setFullCovariance(initialCovariance);

  // Vector of all tracks
  std::vector<BoundTrackParameters> trks;

  unsigned int nTracks = 4;
  for (unsigned int _ = 0; _ < nTracks; _++) {
    // Construct positive or negative charge randomly
    double q = qDist(gen) < 0 ? -1. : 1.;

    // Track resolution
    double resD0 = resIPDist(gen);
    double resZ0 = resIPDist(gen);
    double resPh = resAngDist(gen);
    double resTh = resAngDist(gen);
    double resQp = resQoPDist(gen);
    double resT = resTDist(gen);

    // Random diagonal covariance matrix
    Covariance covMat;

    // clang-format off
    covMat <<
      resD0 * resD0, 0., 0., 0., 0., 0.,
      0., resZ0 * resZ0, 0., 0., 0., 0.,
      0., 0., resPh * resPh, 0., 0., 0.,
      0., 0., 0., resTh * resTh, 0., 0.,
      0., 0., 0., 0., resQp * resQp, 0.,
      0., 0., 0., 0., 0., resT * resT;
    // clang-format on

    // Random track parameters
    BoundTrackParameters::ParametersVector paramVec;
    paramVec << d0Dist(gen), z0Dist(gen), phiDist(gen), thetaDist(gen),
        q / pTDist(gen), trueVtxTime + relTDist(gen);

    std::shared_ptr<PerigeeSurface> perigeeSurface =
        Surface::makeShared<PerigeeSurface>(trueVtxPos);

    trks.emplace_back(perigeeSurface, paramVec, std::move(covMat),
                      ParticleHypothesis::pion());
  }

  std::vector<const BoundTrackParameters*> trksPtr;
  for (const auto& trk : trks) {
    trksPtr.push_back(&trk);
  }

  // Prepare fitter state
  AdaptiveMultiVertexFitter::State state(*bField, magFieldContext);

  for (const auto& trk : trks) {
    ACTS_DEBUG("Track parameters:\n" << trk);
    // Index of current vertex
    state.vtxInfoMap[&vtx].trackLinks.push_back(InputTrack{&trk});
    state.tracksAtVerticesMap.insert(
        std::make_pair(std::make_pair(InputTrack{&trk}, &vtx),
                       TrackAtVertex(1., trk, InputTrack{&trk})));
  }

  state.addVertexToMultiMap(vtx);

  auto res = fitter.addVtxToFit(state, vtx, vertexingOptions);

  BOOST_CHECK(res.ok());

  ACTS_DEBUG("Truth vertex position:  " << trueVtxPos.transpose());
  ACTS_DEBUG("Fitted vertex position: " << vtx.position().transpose());

  ACTS_DEBUG("Truth vertex time:  " << trueVtxTime);
  ACTS_DEBUG("Fitted vertex time: " << vtx.time());

  // Check that true vertex position and time approximately recovered
  CHECK_CLOSE_ABS(trueVtxPos, vtx.position(), 60_um);
  CHECK_CLOSE_ABS(trueVtxTime, vtx.time(), 2_ps);

  const SquareMatrix4& vtxCov = vtx.fullCovariance();

  ACTS_DEBUG("Vertex 4D covariance after the fit:\n" << vtxCov);

  // Check that variances of the vertex position/time are positive
  for (std::size_t i = 0; i <= 3; i++) {
    BOOST_CHECK_GT(vtxCov(i, i), 0.);
  }

  // Check that the covariance matrix is approximately symmetric
  CHECK_CLOSE_ABS(vtxCov - vtxCov.transpose(), SquareMatrix4::Zero(), 1e-5);
}

/// @brief Unit test for AdaptiveMultiVertexFitter
/// based on Athena unit test, i.e. same setting and
/// test values are used here
BOOST_AUTO_TEST_CASE(adaptive_multi_vertex_fitter_test_athena) {
  // Set up constant B-Field
  auto bField = std::make_shared<ConstantBField>(Vector3{0.0, 0.0, 2_T});

  // Set up EigenStepper
  // EigenStepper<> stepper(bField);
  EigenStepper<> stepper(bField);

  // Set up propagator with void navigator
  auto propagator = std::make_shared<Propagator>(stepper);

  VertexingOptions vertexingOptions(geoContext, magFieldContext);

  ImpactPointEstimator::Config ip3dEstCfg(bField, propagator);
  ImpactPointEstimator ip3dEst(ip3dEstCfg);

  std::vector<double> temperatures(1, 3.);
  AnnealingUtility::Config annealingConfig;
  annealingConfig.setOfTemperatures = temperatures;
  AnnealingUtility annealingUtility(annealingConfig);

  AdaptiveMultiVertexFitter::Config fitterCfg(ip3dEst);

  fitterCfg.annealingTool = annealingUtility;
  fitterCfg.extractParameters.connect<&InputTrack::extractParameters>();

  // Linearizer for BoundTrackParameters type test
  Linearizer::Config ltConfig;
  ltConfig.bField = bField;
  ltConfig.propagator = propagator;
  Linearizer linearizer(ltConfig);

  fitterCfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(&linearizer);

  // Test smoothing
  // fitterCfg.doSmoothing = true;

  AdaptiveMultiVertexFitter fitter(std::move(fitterCfg));

  // Create first vector of tracks
  Vector3 pos1a(0.5_mm, -0.5_mm, 2.4_mm);
  Vector3 mom1a(1000_MeV, 0_MeV, -500_MeV);
  Vector3 pos1b(0.5_mm, -0.5_mm, 3.5_mm);
  Vector3 mom1b(0_MeV, 1000_MeV, 500_MeV);
  Vector3 pos1c(-0.2_mm, 0.1_mm, 3.4_mm);
  Vector3 mom1c(-50_MeV, 180_MeV, 300_MeV);

  Vector3 pos1d(-0.1_mm, 0.3_mm, 3.0_mm);
  Vector3 mom1d(-80_MeV, 480_MeV, -100_MeV);
  Vector3 pos1e(-0.01_mm, 0.01_mm, 2.9_mm);
  Vector3 mom1e(-600_MeV, 10_MeV, 210_MeV);

  Vector3 pos1f(-0.07_mm, 0.03_mm, 2.5_mm);
  Vector3 mom1f(240_MeV, 110_MeV, 150_MeV);

  // Start creating some track parameters
  Covariance covMat1;
  covMat1 << 1_mm * 1_mm, 0, 0., 0, 0., 0, 0, 1_mm * 1_mm, 0, 0., 0, 0, 0., 0,
      0.1, 0, 0, 0, 0, 0., 0, 0.1, 0, 0, 0., 0, 0, 0, 1. / (10_GeV * 10_GeV), 0,
      0, 0, 0, 0, 0, 1_ns;

  std::vector<BoundTrackParameters> params1 = {
      BoundTrackParameters::create(Surface::makeShared<PerigeeSurface>(pos1a),
                                   geoContext, makeVector4(pos1a, 0),
                                   mom1a.normalized(), 1_e / mom1a.norm(),
                                   covMat1, ParticleHypothesis::pion())
          .value(),
      BoundTrackParameters::create(Surface::makeShared<PerigeeSurface>(pos1b),
                                   geoContext, makeVector4(pos1b, 0),
                                   mom1b.normalized(), -1_e / mom1b.norm(),
                                   covMat1, ParticleHypothesis::pion())
          .value(),
      BoundTrackParameters::create(Surface::makeShared<PerigeeSurface>(pos1c),
                                   geoContext, makeVector4(pos1c, 0),
                                   mom1c.normalized(), 1_e / mom1c.norm(),
                                   covMat1, ParticleHypothesis::pion())
          .value(),
      BoundTrackParameters::create(Surface::makeShared<PerigeeSurface>(pos1d),
                                   geoContext, makeVector4(pos1d, 0),
                                   mom1d.normalized(), -1_e / mom1d.norm(),
                                   covMat1, ParticleHypothesis::pion())
          .value(),
      BoundTrackParameters::create(Surface::makeShared<PerigeeSurface>(pos1e),
                                   geoContext, makeVector4(pos1e, 0),
                                   mom1e.normalized(), 1_e / mom1e.norm(),
                                   covMat1, ParticleHypothesis::pion())
          .value(),
      BoundTrackParameters::create(Surface::makeShared<PerigeeSurface>(pos1f),
                                   geoContext, makeVector4(pos1f, 0),
                                   mom1f.normalized(), -1_e / mom1f.norm(),
                                   covMat1, ParticleHypothesis::pion())
          .value(),
  };

  // Create second vector of tracks
  Vector3 pos2a(0.2_mm, 0_mm, -4.9_mm);
  Vector3 mom2a(5000_MeV, 30_MeV, 200_MeV);
  Vector3 pos2b(-0.5_mm, 0.1_mm, -5.1_mm);
  Vector3 mom2b(800_MeV, 1200_MeV, 200_MeV);
  Vector3 pos2c(0.05_mm, -0.5_mm, -4.7_mm);
  Vector3 mom2c(400_MeV, -300_MeV, -200_MeV);

  // Define covariance as used in athena unit test
  Covariance covMat2 = covMat1;

  std::vector<BoundTrackParameters> params2 = {
      BoundTrackParameters::create(Surface::makeShared<PerigeeSurface>(pos2a),
                                   geoContext, makeVector4(pos2a, 0),
                                   mom2a.normalized(), 1_e / mom2a.norm(),
                                   covMat2, ParticleHypothesis::pion())
          .value(),
      BoundTrackParameters::create(Surface::makeShared<PerigeeSurface>(pos2b),
                                   geoContext, makeVector4(pos2b, 0),
                                   mom2b.normalized(), -1_e / mom2b.norm(),
                                   covMat2, ParticleHypothesis::pion())
          .value(),
      BoundTrackParameters::create(Surface::makeShared<PerigeeSurface>(pos2c),
                                   geoContext, makeVector4(pos2c, 0),
                                   mom2c.normalized(), -1_e / mom2c.norm(),
                                   covMat2, ParticleHypothesis::pion())
          .value(),
  };

  AdaptiveMultiVertexFitter::State state(*bField, magFieldContext);

  // The constraint vertex position covariance
  SquareMatrix4 covConstr(SquareMatrix4::Identity());
  covConstr = covConstr * 1e+8;
  covConstr(3, 3) = 0.;

  // Prepare first vertex
  Vector3 vtxPos1(0.15_mm, 0.15_mm, 2.9_mm);
  Vertex vtx1(vtxPos1);

  // Add to vertex list
  state.vertexCollection.push_back(&vtx1);

  // The constraint vtx for vtx1
  Vertex vtx1Constr(vtxPos1);
  vtx1Constr.setFullCovariance(covConstr);
  vtx1Constr.setFitQuality(0, -3);

  // Prepare vtx info for fitter
  VertexInfo vtxInfo1;
  vtxInfo1.seedPosition = vtxInfo1.linPoint;
  vtxInfo1.linPoint.setZero();
  vtxInfo1.linPoint.head<3>() = vtxPos1;
  vtxInfo1.constraint = std::move(vtx1Constr);
  vtxInfo1.oldPosition = vtxInfo1.linPoint;

  for (const auto& trk : params1) {
    vtxInfo1.trackLinks.push_back(InputTrack{&trk});
    state.tracksAtVerticesMap.insert(
        std::make_pair(std::make_pair(InputTrack{&trk}, &vtx1),
                       TrackAtVertex(1.5, trk, InputTrack{&trk})));
  }

  // Prepare second vertex
  Vector3 vtxPos2(0.3_mm, -0.2_mm, -4.8_mm);
  Vertex vtx2(vtxPos2);

  // Add to vertex list
  state.vertexCollection.push_back(&vtx2);

  // The constraint vtx for vtx2
  Vertex vtx2Constr(vtxPos2);
  vtx2Constr.setFullCovariance(covConstr);
  vtx2Constr.setFitQuality(0, -3);

  // Prepare vtx info for fitter
  VertexInfo vtxInfo2;
  vtxInfo2.linPoint.setZero();
  vtxInfo2.linPoint.head<3>() = vtxPos2;
  vtxInfo2.constraint = std::move(vtx2Constr);
  vtxInfo2.oldPosition = vtxInfo2.linPoint;
  vtxInfo2.seedPosition = vtxInfo2.linPoint;

  for (const auto& trk : params2) {
    vtxInfo2.trackLinks.push_back(InputTrack{&trk});
    state.tracksAtVerticesMap.insert(
        std::make_pair(std::make_pair(InputTrack{&trk}, &vtx2),
                       TrackAtVertex(1.5, trk, InputTrack{&trk})));
  }

  state.vtxInfoMap[&vtx1] = std::move(vtxInfo1);
  state.vtxInfoMap[&vtx2] = std::move(vtxInfo2);

  state.addVertexToMultiMap(vtx1);
  state.addVertexToMultiMap(vtx2);

  // Fit vertices
  fitter.fit(state, vertexingOptions);

  auto vtx1Fitted = state.vertexCollection.at(0);
  auto vtx1PosFitted = vtx1Fitted->position();
  auto vtx1CovFitted = vtx1Fitted->covariance();
  auto trks1 = state.vtxInfoMap.at(vtx1Fitted).trackLinks;
  auto vtx1FQ = vtx1Fitted->fitQuality();

  auto vtx2Fitted = state.vertexCollection.at(1);
  auto vtx2PosFitted = vtx2Fitted->position();
  auto vtx2CovFitted = vtx2Fitted->covariance();
  auto trks2 = state.vtxInfoMap.at(vtx2Fitted).trackLinks;
  auto vtx2FQ = vtx2Fitted->fitQuality();

  // Vertex 1
  ACTS_DEBUG("Vertex 1, position: " << vtx1PosFitted);
  ACTS_DEBUG("Vertex 1, covariance: " << vtx1CovFitted);
  for (const auto& trk : trks1) {
    auto& trkAtVtx =
        state.tracksAtVerticesMap.at(std::make_pair(trk, vtx1Fitted));
    ACTS_DEBUG("\tTrack weight:" << trkAtVtx.trackWeight);
  }
  ACTS_DEBUG("Vertex 1, chi2: " << vtx1FQ.first);
  ACTS_DEBUG("Vertex 1, ndf: " << vtx1FQ.second);

  // Vertex 2
  ACTS_DEBUG("Vertex 2, position: " << vtx2PosFitted);
  ACTS_DEBUG("Vertex 2, covariance: " << vtx2CovFitted);
  for (const auto& trk : trks2) {
    auto& trkAtVtx =
        state.tracksAtVerticesMap.at(std::make_pair(trk, vtx2Fitted));
    ACTS_DEBUG("\tTrack weight:" << trkAtVtx.trackWeight);
  }
  ACTS_DEBUG("Vertex 2, chi2: " << vtx2FQ.first);
  ACTS_DEBUG("Vertex 2, ndf: " << vtx2FQ.second);

  // Expected values from Athena implementation
  // Vertex 1
  const Vector3 expVtx1Pos(0.077_mm, -0.189_mm, 2.924_mm);

  // Helper matrix to create const expVtx1Cov below
  SquareMatrix3 expVtx1Cov;
  expVtx1Cov << 0.329, 0.016, -0.035, 0.016, 0.250, 0.085, -0.035, 0.085, 0.242;

  ActsVector<6> expVtx1TrkWeights;
  expVtx1TrkWeights << 0.8128, 0.7994, 0.8164, 0.8165, 0.8165, 0.8119;
  const double expVtx1chi2 = 0.9812;
  const double expVtx1ndf = 6.7474;

  // Vertex 2
  const Vector3 expVtx2Pos(-0.443_mm, -0.044_mm, -4.829_mm);
  // Helper matrix to create const expVtx2Cov below
  SquareMatrix3 expVtx2Cov;
  expVtx2Cov << 1.088, 0.028, -0.066, 0.028, 0.643, 0.073, -0.066, 0.073, 0.435;

  const Vector3 expVtx2TrkWeights(0.8172, 0.8150, 0.8137);
  const double expVtx2chi2 = 0.2114;
  const double expVtx2ndf = 1.8920;

  // Compare the results
  // Vertex 1
  CHECK_CLOSE_ABS(vtx1PosFitted, expVtx1Pos, 0.001_mm);
  CHECK_CLOSE_ABS(vtx1CovFitted, expVtx1Cov, 0.001_mm);
  int trkCount = 0;
  for (const auto& trk : trks1) {
    auto& trkAtVtx =
        state.tracksAtVerticesMap.at(std::make_pair(trk, vtx1Fitted));
    CHECK_CLOSE_ABS(trkAtVtx.trackWeight, expVtx1TrkWeights[trkCount], 0.001);
    trkCount++;
  }
  CHECK_CLOSE_ABS(vtx1FQ.first, expVtx1chi2, 0.001);
  CHECK_CLOSE_ABS(vtx1FQ.second, expVtx1ndf, 0.001);

  // Vertex 2
  CHECK_CLOSE_ABS(vtx2PosFitted, expVtx2Pos, 0.001_mm);
  CHECK_CLOSE_ABS(vtx2CovFitted, expVtx2Cov, 0.001_mm);
  trkCount = 0;
  for (const auto& trk : trks2) {
    auto& trkAtVtx =
        state.tracksAtVerticesMap.at(std::make_pair(trk, vtx2Fitted));
    CHECK_CLOSE_ABS(trkAtVtx.trackWeight, expVtx2TrkWeights[trkCount], 0.001);
    trkCount++;
  }
  CHECK_CLOSE_ABS(vtx2FQ.first, expVtx2chi2, 0.001);
  CHECK_CLOSE_ABS(vtx2FQ.second, expVtx2ndf, 0.001);
}

}  // namespace Acts::Test
