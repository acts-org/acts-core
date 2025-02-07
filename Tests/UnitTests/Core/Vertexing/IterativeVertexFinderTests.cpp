// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
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
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/FullBilloirVertexFitter.hpp"
#include "Acts/Vertexing/HelicalTrackLinearizer.hpp"
#include "Acts/Vertexing/IVertexFinder.hpp"
#include "Acts/Vertexing/ImpactPointEstimator.hpp"
#include "Acts/Vertexing/IterativeVertexFinder.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"
#include "Acts/Vertexing/ZScanVertexFinder.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <numbers>
#include <optional>
#include <random>
#include <string>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "VertexingDataHelper.hpp"

using namespace Acts::UnitLiterals;

namespace Acts::Test {

using Covariance = BoundSquareMatrix;
using Propagator = Acts::Propagator<EigenStepper<>>;
using Linearizer = HelicalTrackLinearizer;

// Create a test context
GeometryContext geoContext = GeometryContext();
MagneticFieldContext magFieldContext = MagneticFieldContext();

const std::string toolString = "IVF";

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
std::uniform_real_distribution<double> phiDist(-std::numbers::pi,
                                               std::numbers::pi);
// Track theta distribution
std::uniform_real_distribution<double> thetaDist(1., std::numbers::pi - 1.);
// Track charge helper distribution
std::uniform_real_distribution<double> qDist(-1, 1);
// Track IP resolution distribution
std::uniform_real_distribution<double> resIPDist(0., 100_um);
// Track angular distribution
std::uniform_real_distribution<double> resAngDist(0., 0.1);
// Track q/p resolution distribution
std::uniform_real_distribution<double> resQoPDist(-0.01, 0.01);
// Number of vertices per test event distribution
std::uniform_int_distribution<std::uint32_t> nVertexDist(1, 6);
// Number of tracks per vertex distribution
std::uniform_int_distribution<std::uint32_t> nTracksDist(5, 15);

// Dummy user-defined InputTrack type
struct InputTrackStub {
  InputTrackStub(const BoundTrackParameters& params) : m_parameters(params) {}

  const BoundTrackParameters& parameters() const { return m_parameters; }

  // store e.g. link to original objects here

 private:
  BoundTrackParameters m_parameters;
};

///
/// @brief Unit test for IterativeVertexFinder for BoundTrackParameters
///
BOOST_AUTO_TEST_CASE(iterative_finder_test) {
  bool debug = false;

  // Set up RNG
  int mySeed = 31415;
  std::mt19937 gen(mySeed);

  // Number of test events
  unsigned int nEvents = 5;  // = nTest

  // Set up constant B-Field
  auto bField = std::make_shared<ConstantBField>(Vector3{0.0, 0.0, 1_T});

  for (unsigned int iEvent = 0; iEvent < nEvents; ++iEvent) {
    // Set up Eigenstepper
    EigenStepper<> stepper(bField);

    // Set up propagator with void navigator
    auto propagator = std::make_shared<Propagator>(stepper);

    // Linearizer for BoundTrackParameters type test
    Linearizer::Config ltConfig;
    ltConfig.bField = bField;
    ltConfig.propagator = propagator;
    Linearizer linearizer(ltConfig);

    using BilloirFitter = FullBilloirVertexFitter;

    // Set up Billoir Vertex Fitter
    BilloirFitter::Config vertexFitterCfg;
    vertexFitterCfg.extractParameters.connect<&InputTrack::extractParameters>();
    vertexFitterCfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(
        &linearizer);

    BilloirFitter bFitter(vertexFitterCfg);

    ImpactPointEstimator::Config ipEstimatorCfg(bField, propagator);
    ImpactPointEstimator ipEstimator(ipEstimatorCfg);

    using ZScanSeedFinder = ZScanVertexFinder;

    ZScanSeedFinder::Config seedFinderCfg(ipEstimator);
    seedFinderCfg.extractParameters.connect<&InputTrack::extractParameters>();

    auto sFinder = std::make_shared<ZScanSeedFinder>(seedFinderCfg);

    // Vertex Finder

    IterativeVertexFinder::Config cfg(std::move(bFitter), std::move(sFinder),
                                      ipEstimator);
    cfg.field = bField;
    cfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(&linearizer);

    cfg.reassignTracksAfterFirstFit = true;
    cfg.extractParameters.connect<&InputTrack::extractParameters>();

    IterativeVertexFinder finder(std::move(cfg));
    IVertexFinder::State state{
        IterativeVertexFinder::State(*bField, magFieldContext)};

    // Vector to be filled with all tracks in current event
    std::vector<std::unique_ptr<const BoundTrackParameters>> tracks;

    std::vector<InputTrack> inputTracks;

    // Vector to be filled with truth vertices for later comparison
    std::vector<Vertex> trueVertices;

    // start creating event with nVertices vertices
    std::uint32_t nVertices = nVertexDist(gen);
    for (std::uint32_t iVertex = 0; iVertex < nVertices; ++iVertex) {
      // Number of tracks
      std::uint32_t nTracks = nTracksDist(gen);

      if (debug) {
        std::cout << "Event " << iEvent << ", Vertex " << iVertex << "/"
                  << nVertices << " with " << nTracks << " tracks."
                  << std::endl;
      }
      // Create perigee surface
      std::shared_ptr<PerigeeSurface> perigeeSurface =
          Surface::makeShared<PerigeeSurface>(Vector3(0., 0., 0.));

      // Create position of vertex and perigee surface
      double x = vXYDist(gen);
      double y = vXYDist(gen);
      double z = vZDist(gen);

      // True vertex
      Vertex trueV(Vector3(x, y, z));
      std::vector<TrackAtVertex> tracksAtTrueVtx;

      // Calculate d0 and z0 corresponding to vertex position
      double d0_v = std::hypot(x, y);
      double z0_v = z;

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
        double res_d0 = resIPDist(gen);
        double res_z0 = resIPDist(gen);
        double res_ph = resAngDist(gen);
        double res_th = resAngDist(gen);
        double res_qp = resQoPDist(gen);

        // Fill vector of track objects with simple covariance matrix
        Covariance covMat;
        covMat << res_d0 * res_d0, 0., 0., 0., 0., 0., 0., res_z0 * res_z0, 0.,
            0., 0., 0., 0., 0., res_ph * res_ph, 0., 0., 0., 0., 0., 0.,
            res_th * res_th, 0., 0., 0., 0., 0., 0., res_qp * res_qp, 0., 0.,
            0., 0., 0., 0., 1.;
        auto params =
            BoundTrackParameters(perigeeSurface, paramVec, std::move(covMat),
                                 ParticleHypothesis::pion());

        tracks.push_back(std::make_unique<BoundTrackParameters>(params));

        TrackAtVertex trAtVt(0., params, InputTrack{tracks.back().get()});
        tracksAtTrueVtx.push_back(trAtVt);
      }

      trueV.setTracksAtVertex(tracksAtTrueVtx);
      trueVertices.push_back(trueV);

    }  // end loop over vertices

    // shuffle list of tracks
    std::shuffle(std::begin(tracks), std::end(tracks), gen);

    for (const auto& trk : tracks) {
      inputTracks.emplace_back(trk.get());
    }

    VertexingOptions vertexingOptions(geoContext, magFieldContext);

    // find vertices
    auto res = finder.find(inputTracks, vertexingOptions, state);

    if (!res.ok()) {
      BOOST_FAIL(res.error().message());
    }

    // Retrieve vertices found by vertex finder
    auto vertexCollection = *res;

    // check if same amount of vertices has been found with tolerance of 2
    CHECK_CLOSE_ABS(vertexCollection.size(), nVertices, 2);

    if (debug) {
      std::cout << "########## RESULT: ########## Event " << iEvent
                << std::endl;
      std::cout << "Number of true vertices: " << nVertices << std::endl;
      std::cout << "Number of reco vertices: " << vertexCollection.size()
                << std::endl;

      int count = 1;
      std::cout << "----- True vertices -----" << std::endl;
      for (const auto& vertex : trueVertices) {
        Vector3 pos = vertex.position();
        std::cout << count << ". True Vertex:\t Position:"
                  << "(" << pos[eX] << "," << pos[eY] << "," << pos[eZ] << ")"
                  << std::endl;
        std::cout << "Number of tracks: " << vertex.tracks().size() << std::endl
                  << std::endl;
        count++;
      }
      std::cout << "----- Reco vertices -----" << std::endl;
      count = 1;
      for (const auto& vertex : vertexCollection) {
        Vector3 pos = vertex.position();
        std::cout << count << ". Reco Vertex:\t Position:"
                  << "(" << pos[eX] << "," << pos[eY] << "," << pos[eZ] << ")"
                  << std::endl;
        std::cout << "Number of tracks: " << vertex.tracks().size() << std::endl
                  << std::endl;
        count++;
      }
    }

    // Check if all vertices have been found with close z-values
    bool allVerticesFound = true;
    for (const auto& trueVertex : trueVertices) {
      Vector4 truePos = trueVertex.fullPosition();
      bool currentVertexFound = false;
      for (const auto& recoVertex : vertexCollection) {
        Vector4 recoPos = recoVertex.fullPosition();
        // check only for close z distance
        double zDistance = std::abs(truePos[eZ] - recoPos[eZ]);
        if (zDistance < 2_mm) {
          currentVertexFound = true;
        }
      }
      if (!currentVertexFound) {
        allVerticesFound = false;
      }
    }

    // check if found vertices have compatible z values
    BOOST_CHECK(allVerticesFound);
  }
}

///
/// @brief Unit test for IterativeVertexFinder
///        for user defined InputTrack track type
///
BOOST_AUTO_TEST_CASE(iterative_finder_test_user_track_type) {
  bool debug = false;

  // Set up RNG
  int mySeed = 31415;
  std::mt19937 gen(mySeed);

  // Number of test events
  unsigned int nEvents = 5;  // = nTest

  // Set up constant B-Field
  auto bField = std::make_shared<ConstantBField>(Vector3{0.0, 0.0, 1_T});

  for (unsigned int iEvent = 0; iEvent < nEvents; ++iEvent) {
    // Set up Eigenstepper
    EigenStepper<> stepper(bField);

    // Set up propagator with void navigator
    auto propagator = std::make_shared<Propagator>(stepper);

    // Linearizer for user defined InputTrackStub type test
    Linearizer::Config ltConfigUT;
    ltConfigUT.bField = bField;
    ltConfigUT.propagator = propagator;
    Linearizer linearizer(ltConfigUT);

    // Set up vertex fitter for user track type
    using BilloirFitter = FullBilloirVertexFitter;

    // Create a custom std::function to extract BoundTrackParameters from
    // user-defined InputTrack
    auto extractParameters = [](const InputTrack& params) {
      return params.as<InputTrackStub>()->parameters();
    };

    // Set up Billoir Vertex Fitter
    BilloirFitter::Config vertexFitterCfg;
    vertexFitterCfg.extractParameters.connect(extractParameters);
    vertexFitterCfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(
        &linearizer);

    BilloirFitter bFitter(vertexFitterCfg);

    ImpactPointEstimator::Config ipEstimatorCfg(bField, propagator);
    ImpactPointEstimator ipEstimator(ipEstimatorCfg);

    using ZScanSeedFinder = ZScanVertexFinder;
    ZScanSeedFinder::Config seedFinderCfg(ipEstimator);
    seedFinderCfg.extractParameters.connect(extractParameters);

    auto sFinder = std::make_shared<ZScanSeedFinder>(seedFinderCfg);

    // Vertex Finder
    IterativeVertexFinder::Config cfg(std::move(bFitter), std::move(sFinder),
                                      ipEstimator);
    cfg.reassignTracksAfterFirstFit = true;
    cfg.extractParameters.connect(extractParameters);
    cfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(&linearizer);
    cfg.field = bField;

    IterativeVertexFinder finder(std::move(cfg));
    IVertexFinder::State state{
        IterativeVertexFinder::State(*bField, magFieldContext)};

    // Same for user track type tracks
    std::vector<std::unique_ptr<const InputTrackStub>> tracks;

    std::vector<InputTrack> inputTracks;

    // Vector to be filled with truth vertices for later comparison
    std::vector<Vertex> trueVertices;

    // start creating event with nVertices vertices
    std::uint32_t nVertices = nVertexDist(gen);
    for (std::uint32_t iVertex = 0; iVertex < nVertices; ++iVertex) {
      // Number of tracks
      std::uint32_t nTracks = nTracksDist(gen);

      if (debug) {
        std::cout << "Event " << iEvent << ", Vertex " << iVertex << "/"
                  << nVertices << " with " << nTracks << " tracks."
                  << std::endl;
      }
      // Create perigee surface
      std::shared_ptr<PerigeeSurface> perigeeSurface =
          Surface::makeShared<PerigeeSurface>(Vector3(0., 0., 0.));

      // Create position of vertex and perigee surface
      double x = vXYDist(gen);
      double y = vXYDist(gen);
      double z = vZDist(gen);

      // True vertex
      Vertex trueV(Vector3(x, y, z));
      std::vector<TrackAtVertex> tracksAtTrueVtx;

      // Calculate d0 and z0 corresponding to vertex position
      double d0_v = std::hypot(x, y);
      double z0_v = z;

      // Construct random track emerging from vicinity of vertex position
      // Vector to store track objects used for vertex fit
      for (std::uint32_t iTrack = 0; iTrack < nTracks; iTrack++) {
        // Construct positive or negative charge randomly
        double q = qDist(gen) < 0 ? -1. : 1.;

        // Construct random track parameters
        BoundVector paramVec;
        double z0track = z0_v + z0Dist(gen);
        paramVec << d0_v + d0Dist(gen), z0track, phiDist(gen), thetaDist(gen),
            q / pTDist(gen), 0.;

        // Resolutions
        double res_d0 = resIPDist(gen);
        double res_z0 = resIPDist(gen);
        double res_ph = resAngDist(gen);
        double res_th = resAngDist(gen);
        double res_qp = resQoPDist(gen);

        // Fill vector of track objects now for user track type
        Covariance covMat;

        covMat << res_d0 * res_d0, 0., 0., 0., 0., 0., 0., res_z0 * res_z0, 0.,
            0., 0., 0., 0., 0., res_ph * res_ph, 0., 0., 0., 0., 0., 0.,
            res_th * res_th, 0., 0., 0., 0., 0., 0., res_qp * res_qp, 0., 0.,
            0., 0., 0., 0., 1.;
        auto params =
            BoundTrackParameters(perigeeSurface, paramVec, std::move(covMat),
                                 ParticleHypothesis::pion());

        tracks.push_back(std::make_unique<InputTrackStub>(params));

        TrackAtVertex trAtVt(0., params, InputTrack{tracks.back().get()});
        tracksAtTrueVtx.push_back(trAtVt);
      }

      trueV.setTracksAtVertex(tracksAtTrueVtx);
      trueVertices.push_back(trueV);

    }  // end loop over vertices

    // shuffle list of tracks
    std::shuffle(std::begin(tracks), std::end(tracks), gen);

    for (const auto& trk : tracks) {
      inputTracks.emplace_back(trk.get());
    }

    VertexingOptions vertexingOptionsUT(geoContext, magFieldContext);

    // find vertices
    auto res = finder.find(inputTracks, vertexingOptionsUT, state);

    if (!res.ok()) {
      BOOST_FAIL(res.error().message());
    }

    // Retrieve vertices found by vertex finder
    auto vertexCollectionUT = *res;

    // check if same amount of vertices has been found with tolerance of 2
    CHECK_CLOSE_ABS(vertexCollectionUT.size(), nVertices, 2);

    if (debug) {
      std::cout << "########## RESULT: ########## Event " << iEvent
                << std::endl;
      std::cout << "Number of true vertices: " << nVertices << std::endl;
      std::cout << "Number of reco vertices: " << vertexCollectionUT.size()
                << std::endl;

      int count = 1;
      std::cout << "----- True vertices -----" << std::endl;
      for (const auto& vertex : trueVertices) {
        Vector3 pos = vertex.position();
        std::cout << count << ". True Vertex:\t Position:"
                  << "(" << pos[eX] << "," << pos[eY] << "," << pos[eZ] << ")"
                  << std::endl;
        std::cout << "Number of tracks: " << vertex.tracks().size() << std::endl
                  << std::endl;
        count++;
      }
      std::cout << "----- Reco vertices -----" << std::endl;
      count = 1;
      for (const auto& vertex : vertexCollectionUT) {
        Vector3 pos = vertex.position();
        std::cout << count << ". Reco Vertex:\t Position:"
                  << "(" << pos[eX] << "," << pos[eY] << "," << pos[eZ] << ")"
                  << std::endl;
        std::cout << "Number of tracks: " << vertex.tracks().size() << std::endl
                  << std::endl;
        count++;
      }
    }

    // Check if all vertices have been found with close z-values
    bool allVerticesFound = true;
    for (const auto& trueVertex : trueVertices) {
      Vector4 truePos = trueVertex.fullPosition();
      bool currentVertexFound = false;
      for (const auto& recoVertex : vertexCollectionUT) {
        Vector4 recoPos = recoVertex.fullPosition();
        // check only for close z distance
        double zDistance = std::abs(truePos[eZ] - recoPos[eZ]);
        if (zDistance < 2_mm) {
          currentVertexFound = true;
        }
      }
      if (!currentVertexFound) {
        allVerticesFound = false;
      }
    }

    // check if found vertices have compatible z values
    BOOST_CHECK(allVerticesFound);
  }
}

///
/// @brief Unit test for IterativeVertexFinder with Athena reference data
///
BOOST_AUTO_TEST_CASE(iterative_finder_test_athena_reference) {
  // Set up constant B-Field
  auto bField = std::make_shared<ConstantBField>(Vector3{0.0, 0.0, 2_T});

  // Set up Eigenstepper
  EigenStepper<> stepper(bField);

  // Set up propagator with void navigator
  auto propagator = std::make_shared<Propagator>(stepper);

  // Linearizer for BoundTrackParameters type test
  Linearizer::Config ltConfig;
  ltConfig.bField = bField;
  ltConfig.propagator = propagator;
  Linearizer linearizer(ltConfig);

  using BilloirFitter = FullBilloirVertexFitter;

  // Set up Billoir Vertex Fitter
  BilloirFitter::Config vertexFitterCfg;
  vertexFitterCfg.extractParameters.connect<&InputTrack::extractParameters>();
  vertexFitterCfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(
      &linearizer);

  BilloirFitter bFitter(vertexFitterCfg);

  ImpactPointEstimator::Config ipEstimatorCfg(bField, propagator);
  ImpactPointEstimator ipEstimator(ipEstimatorCfg);

  using ZScanSeedFinder = ZScanVertexFinder;

  ZScanSeedFinder::Config seedFinderCfg(ipEstimator);
  seedFinderCfg.extractParameters.connect<&InputTrack::extractParameters>();

  auto sFinder = std::make_shared<ZScanSeedFinder>(seedFinderCfg);

  IterativeVertexFinder::Config cfg(std::move(bFitter), std::move(sFinder),
                                    ipEstimator);
  cfg.maxVertices = 200;
  cfg.maximumChi2cutForSeeding = 49;
  cfg.significanceCutSeeding = 12;
  cfg.extractParameters.connect<&InputTrack::extractParameters>();
  cfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(&linearizer);
  cfg.field = bField;

  IterativeVertexFinder finder(std::move(cfg));
  IVertexFinder::State state{
      IterativeVertexFinder::State(*bField, magFieldContext)};

  auto csvData = readTracksAndVertexCSV(toolString);
  auto tracks = std::get<TracksData>(csvData);

  std::vector<InputTrack> inputTracks;
  for (const auto& trk : tracks) {
    inputTracks.emplace_back(&trk);
  }

  Vertex beamSpot = std::get<BeamSpotData>(csvData);
  // Set time covariance
  SquareMatrix4 fullCovariance = SquareMatrix4::Zero();
  fullCovariance.topLeftCorner<3, 3>() = beamSpot.covariance();
  fullCovariance(eTime, eTime) = 100_ns;
  beamSpot.setFullCovariance(fullCovariance);
  VertexingOptions vertexingOptions(geoContext, magFieldContext, beamSpot);

  // find vertices
  auto findResult = finder.find(inputTracks, vertexingOptions, state);

  BOOST_CHECK(findResult.ok());

  if (!findResult.ok()) {
    std::cout << findResult.error().message() << std::endl;
  }

  // Retrieve vertices found by vertex finder
  // std::vector<Vertex> allVertices = *findResult;

  // Test expected outcomes from athena implementation
  // Number of reconstructed vertices
  // auto verticesInfo = std::get<VerticesData>(csvData);
  // const int expNRecoVertices = verticesInfo.size();

  // BOOST_CHECK_EQUAL(allVertices.size(), expNRecoVertices);
  // for (int i = 0; i < expNRecoVertices; i++) {
  //   auto recoVtx = allVertices[i];
  //   auto expVtx = verticesInfo[i];
  //   CHECK_CLOSE_ABS(recoVtx.position(), expVtx.position, 0.001_mm);
  //   CHECK_CLOSE_ABS(recoVtx.covariance(), expVtx.covariance, 0.001_mm);
  //   BOOST_CHECK_EQUAL(recoVtx.tracks().size(), expVtx.nTracks);
  //   CHECK_CLOSE_ABS(recoVtx.tracks()[0].trackWeight, expVtx.trk1Weight,
  //   0.003); CHECK_CLOSE_ABS(recoVtx.tracks()[0].vertexCompatibility,
  //   expVtx.trk1Comp,
  //                   0.003);
  // }
}

}  // namespace Acts::Test
