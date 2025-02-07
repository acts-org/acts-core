// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/AnnealingUtility.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/AdaptiveGridDensityVertexFinder.hpp"
#include "Acts/Vertexing/AdaptiveMultiVertexFinder.hpp"
#include "Acts/Vertexing/AdaptiveMultiVertexFitter.hpp"
#include "Acts/Vertexing/GaussianTrackDensity.hpp"
#include "Acts/Vertexing/GridDensityVertexFinder.hpp"
#include "Acts/Vertexing/HelicalTrackLinearizer.hpp"
#include "Acts/Vertexing/IVertexFinder.hpp"
#include "Acts/Vertexing/ImpactPointEstimator.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "Acts/Vertexing/TrackDensityVertexFinder.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <numbers>
#include <string>
#include <system_error>
#include <tuple>
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

const std::string toolString = "AMVF";

/// @brief AMVF test with Gaussian seed finder
BOOST_AUTO_TEST_CASE(adaptive_multi_vertex_finder_test) {
  // Set debug mode
  bool debugMode = false;
  // Set up constant B-Field
  auto bField = std::make_shared<ConstantBField>(Vector3(0., 0., 2_T));

  // Set up EigenStepper
  // EigenStepper<> stepper(bField);
  EigenStepper<> stepper(bField);

  // Set up propagator with void navigator
  auto propagator = std::make_shared<Propagator>(stepper);

  // IP 3D Estimator
  ImpactPointEstimator::Config ipEstimatorCfg(bField, propagator);
  ImpactPointEstimator ipEstimator(ipEstimatorCfg);

  std::vector<double> temperatures{
      8., 4., 2., std::numbers::sqrt2, std::sqrt(3. / 2.), 1.};
  AnnealingUtility::Config annealingConfig;
  annealingConfig.setOfTemperatures = temperatures;
  AnnealingUtility annealingUtility(annealingConfig);

  using Fitter = AdaptiveMultiVertexFitter;

  Fitter::Config fitterCfg(ipEstimator);

  fitterCfg.annealingTool = annealingUtility;

  // Linearizer for BoundTrackParameters type test
  Linearizer::Config ltConfig;
  ltConfig.bField = bField;
  ltConfig.propagator = propagator;
  Linearizer linearizer(ltConfig);

  // Test smoothing
  fitterCfg.doSmoothing = true;
  fitterCfg.extractParameters.connect<&InputTrack::extractParameters>();
  fitterCfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(&linearizer);

  Fitter fitter(fitterCfg);

  GaussianTrackDensity::Config densityCfg;
  densityCfg.extractParameters.connect<&InputTrack::extractParameters>();
  auto seedFinder = std::make_shared<TrackDensityVertexFinder>(
      TrackDensityVertexFinder::Config{densityCfg});

  AdaptiveMultiVertexFinder::Config finderConfig(std::move(fitter), seedFinder,
                                                 ipEstimator, bField);
  finderConfig.extractParameters.connect<&InputTrack::extractParameters>();

  AdaptiveMultiVertexFinder finder(std::move(finderConfig));
  IVertexFinder::State state = finder.makeState(magFieldContext);

  auto csvData = readTracksAndVertexCSV(toolString);
  std::vector<BoundTrackParameters> tracks = std::get<TracksData>(csvData);

  if (debugMode) {
    std::cout << "Number of tracks in event: " << tracks.size() << std::endl;
    int maxCout = 10;
    int count = 0;
    for (const auto& trk : tracks) {
      std::cout << count << ". track: " << std::endl;
      std::cout << "params: " << trk << std::endl;
      count++;
      if (count == maxCout) {
        break;
      }
    }
  }

  std::vector<InputTrack> inputTracks;
  for (const auto& trk : tracks) {
    inputTracks.emplace_back(&trk);
  }

  // TODO: test without using beam spot constraint
  Vertex bsConstr = std::get<BeamSpotData>(csvData);
  VertexingOptions vertexingOptions(geoContext, magFieldContext, bsConstr);

  auto t1 = std::chrono::system_clock::now();
  auto findResult = finder.find(inputTracks, vertexingOptions, state);
  auto t2 = std::chrono::system_clock::now();

  auto timediff =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  if (!findResult.ok()) {
    std::cout << findResult.error().message() << std::endl;
  }

  BOOST_CHECK(findResult.ok());

  std::vector<Vertex> allVertices = *findResult;

  if (debugMode) {
    std::cout << "Time needed: " << timediff << " ms." << std::endl;
    std::cout << "Number of vertices reconstructed: " << allVertices.size()
              << std::endl;

    int count = 0;
    for (const auto& vtx : allVertices) {
      count++;
      std::cout << count << ". Vertex at position: " << vtx.position()[0]
                << ", " << vtx.position()[1] << ", " << vtx.position()[2]
                << std::endl;
      std::cout << count << ". Vertex with cov: " << vtx.covariance()
                << std::endl;
      std::cout << "\t with n tracks: " << vtx.tracks().size() << std::endl;
    }
  }

  // Test expected outcomes from athena implementation
  // Number of reconstructed vertices
  auto verticesInfo = std::get<VerticesData>(csvData);
  const int expNRecoVertices = verticesInfo.size();

  BOOST_CHECK_EQUAL(allVertices.size(), expNRecoVertices);

  double relTol = 1e-2;
  double small = 1e-3;
  for (int i = 0; i < expNRecoVertices; i++) {
    auto recoVtx = allVertices[i];
    auto expVtx = verticesInfo[i];
    CHECK_CLOSE_OR_SMALL(recoVtx.position(), expVtx.position, relTol, small);
    CHECK_CLOSE_OR_SMALL(recoVtx.covariance(), expVtx.covariance, relTol,
                         small);
    BOOST_CHECK_EQUAL(recoVtx.tracks().size(), expVtx.nTracks);
    CHECK_CLOSE_OR_SMALL(recoVtx.tracks()[0].trackWeight, expVtx.trk1Weight,
                         relTol, small);
    CHECK_CLOSE_OR_SMALL(recoVtx.tracks()[0].vertexCompatibility,
                         expVtx.trk1Comp, relTol, small);
  }
}

// Dummy user-defined InputTrackStub type
struct InputTrackStub {
  InputTrackStub(const BoundTrackParameters& params, int id)
      : m_parameters(params), m_id(id) {}

  const BoundTrackParameters& parameters() const { return m_parameters; }
  // store e.g. link to original objects here

  int id() const { return m_id; }

 private:
  BoundTrackParameters m_parameters;

  // Some test track ID
  int m_id;
};

/// @brief AMVF test with user-defined input track type
BOOST_AUTO_TEST_CASE(adaptive_multi_vertex_finder_usertype_test) {
  // Set debug mode
  bool debugMode = false;
  // Set up constant B-Field
  auto bField = std::make_shared<ConstantBField>(Vector3(0., 0., 2_T));

  // Set up EigenStepper
  // EigenStepper<> stepper(bField);
  EigenStepper<> stepper(bField);

  // Set up propagator with void navigator
  auto propagator = std::make_shared<Propagator>(stepper);

  // Create a custom std::function to extract BoundTrackParameters from
  // user-defined InputTrackStub
  auto extractParameters = [](const InputTrack& track) {
    return track.as<InputTrackStub>()->parameters();
  };

  // IP 3D Estimator
  ImpactPointEstimator::Config ipEstimatorCfg(bField, propagator);
  ImpactPointEstimator ipEstimator(ipEstimatorCfg);

  std::vector<double> temperatures{
      8., 4., 2., std::numbers::sqrt2, std::sqrt(3. / 2.), 1.};
  AnnealingUtility::Config annealingConfig;
  annealingConfig.setOfTemperatures = temperatures;
  AnnealingUtility annealingUtility(annealingConfig);

  using Fitter = AdaptiveMultiVertexFitter;

  Fitter::Config fitterCfg(ipEstimator);

  fitterCfg.annealingTool = annealingUtility;

  // Linearizer
  Linearizer::Config ltConfig;
  ltConfig.bField = bField;
  ltConfig.propagator = propagator;
  Linearizer linearizer(ltConfig);

  // Test smoothing
  fitterCfg.doSmoothing = true;
  fitterCfg.extractParameters.connect(extractParameters);
  fitterCfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(&linearizer);

  Fitter fitter(fitterCfg);

  GaussianTrackDensity::Config densityCfg;
  densityCfg.extractParameters.connect(extractParameters);
  auto seedFinder = std::make_shared<TrackDensityVertexFinder>(
      TrackDensityVertexFinder::Config{densityCfg});

  AdaptiveMultiVertexFinder::Config finderConfig(
      std::move(fitter), std::move(seedFinder), ipEstimator, bField);
  finderConfig.extractParameters.connect(extractParameters);

  AdaptiveMultiVertexFinder finder(std::move(finderConfig));
  IVertexFinder::State state = finder.makeState(magFieldContext);

  auto csvData = readTracksAndVertexCSV(toolString);
  auto tracks = std::get<TracksData>(csvData);

  std::vector<InputTrackStub> userTracks;
  int idCount = 0;
  for (const auto& trk : tracks) {
    userTracks.push_back(InputTrackStub(trk, idCount));
    idCount++;
  }

  if (debugMode) {
    std::cout << "Number of tracks in event: " << tracks.size() << std::endl;
    int maxCout = 10;
    int count = 0;
    for (const auto& trk : tracks) {
      std::cout << count << ". track: " << std::endl;
      std::cout << "params: " << trk << std::endl;
      count++;
      if (count == maxCout) {
        break;
      }
    }
  }

  std::vector<InputTrack> userInputTracks;
  for (const auto& trk : userTracks) {
    userInputTracks.emplace_back(&trk);
  }

  Vertex constraintVtx;
  constraintVtx.setPosition(std::get<BeamSpotData>(csvData).position());
  constraintVtx.setCovariance(std::get<BeamSpotData>(csvData).covariance());

  VertexingOptions vertexingOptions(geoContext, magFieldContext, constraintVtx);

  auto findResult = finder.find(userInputTracks, vertexingOptions, state);

  if (!findResult.ok()) {
    std::cout << findResult.error().message() << std::endl;
  }

  BOOST_CHECK(findResult.ok());

  std::vector<Vertex> allVertices = *findResult;

  if (debugMode) {
    std::cout << "Number of vertices reconstructed: " << allVertices.size()
              << std::endl;

    int count = 0;
    for (const auto& vtx : allVertices) {
      count++;
      std::cout << count << ". Vertex at position: " << vtx.position()[0]
                << ", " << vtx.position()[1] << ", " << vtx.position()[2]
                << std::endl;
      std::cout << count << ". Vertex with cov: " << vtx.covariance()
                << std::endl;
      std::cout << "\t with n tracks: " << vtx.tracks().size() << std::endl;
    }
    for (auto& trk : allVertices[0].tracks()) {
      std::cout << "Track ID at first vertex: "
                << trk.originalParams.as<InputTrackStub>()->id() << std::endl;
    }
  }

  auto verticesInfo = std::get<VerticesData>(csvData);
  const int expNRecoVertices = verticesInfo.size();

  BOOST_CHECK_EQUAL(allVertices.size(), expNRecoVertices);

  double relTol = 1e-2;
  double small = 1e-3;
  for (int i = 0; i < expNRecoVertices; i++) {
    auto recoVtx = allVertices[i];
    auto expVtx = verticesInfo[i];
    CHECK_CLOSE_OR_SMALL(recoVtx.position(), expVtx.position, relTol, small);
    CHECK_CLOSE_OR_SMALL(recoVtx.covariance(), expVtx.covariance, relTol,
                         small);
    BOOST_CHECK_EQUAL(recoVtx.tracks().size(), expVtx.nTracks);
    CHECK_CLOSE_OR_SMALL(recoVtx.tracks()[0].trackWeight, expVtx.trk1Weight,
                         relTol, small);
    CHECK_CLOSE_OR_SMALL(recoVtx.tracks()[0].vertexCompatibility,
                         expVtx.trk1Comp, relTol, small);
  }
}

/// @brief AMVF test with grid seed finder
BOOST_AUTO_TEST_CASE(adaptive_multi_vertex_finder_grid_seed_finder_test) {
  // Set debug mode
  bool debugMode = false;
  if (debugMode) {
    std::cout << "Starting AMVF test with grid seed finder..." << std::endl;
  }
  // Set up constant B-Field
  auto bField = std::make_shared<ConstantBField>(Vector3(0., 0., 2_T));

  // Set up EigenStepper
  // EigenStepper<> stepper(bField);
  EigenStepper<> stepper(bField);

  // Set up propagator with void navigator
  auto propagator = std::make_shared<Propagator>(stepper);

  // IP Estimator
  ImpactPointEstimator::Config ipEstCfg(bField, propagator);
  ImpactPointEstimator ipEst(ipEstCfg);

  std::vector<double> temperatures{
      8., 4., 2., std::numbers::sqrt2, std::sqrt(3. / 2.), 1.};
  AnnealingUtility::Config annealingConfig;
  annealingConfig.setOfTemperatures = temperatures;
  AnnealingUtility annealingUtility(annealingConfig);

  using Fitter = AdaptiveMultiVertexFitter;

  Fitter::Config fitterCfg(ipEst);

  fitterCfg.annealingTool = annealingUtility;

  // Linearizer for BoundTrackParameters type test
  Linearizer::Config ltConfig;
  ltConfig.bField = bField;
  ltConfig.propagator = propagator;
  Linearizer linearizer(ltConfig);

  // Test smoothing
  fitterCfg.doSmoothing = true;
  fitterCfg.extractParameters.connect<&InputTrack::extractParameters>();
  fitterCfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(&linearizer);

  Fitter fitter(fitterCfg);

  using SeedFinder = GridDensityVertexFinder;
  SeedFinder::Config seedFinderCfg{{{250, 4000, 55}}};
  seedFinderCfg.cacheGridStateForTrackRemoval = true;
  seedFinderCfg.extractParameters.connect<&InputTrack::extractParameters>();

  auto seedFinder = std::make_shared<SeedFinder>(seedFinderCfg);

  AdaptiveMultiVertexFinder::Config finderConfig(
      std::move(fitter), std::move(seedFinder), ipEst, bField);
  finderConfig.extractParameters.connect<&InputTrack::extractParameters>();

  AdaptiveMultiVertexFinder finder(std::move(finderConfig));
  IVertexFinder::State state = finder.makeState(magFieldContext);

  auto csvData = readTracksAndVertexCSV(toolString);
  auto tracks = std::get<TracksData>(csvData);

  if (debugMode) {
    std::cout << "Number of tracks in event: " << tracks.size() << std::endl;
    int maxCout = 10;
    int count = 0;
    for (const auto& trk : tracks) {
      std::cout << count << ". track: " << std::endl;
      std::cout << "params: " << trk << std::endl;
      count++;
      if (count == maxCout) {
        break;
      }
    }
  }

  std::vector<InputTrack> inputTracks;
  for (const auto& trk : tracks) {
    inputTracks.emplace_back(&trk);
  }

  // TODO: test using beam spot constraint
  Vertex bsConstr = std::get<BeamSpotData>(csvData);
  VertexingOptions vertexingOptions(geoContext, magFieldContext, bsConstr);

  auto t1 = std::chrono::system_clock::now();
  auto findResult = finder.find(inputTracks, vertexingOptions, state);
  auto t2 = std::chrono::system_clock::now();

  auto timediff =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  if (!findResult.ok()) {
    std::cout << findResult.error().message() << std::endl;
  }

  BOOST_CHECK(findResult.ok());

  std::vector<Vertex> allVertices = *findResult;

  if (debugMode) {
    std::cout << "Time needed: " << timediff << " ms." << std::endl;
    std::cout << "Number of vertices reconstructed: " << allVertices.size()
              << std::endl;

    int count = 0;
    for (const auto& vtx : allVertices) {
      count++;
      std::cout << count << ". Vertex at position: " << vtx.position()[0]
                << ", " << vtx.position()[1] << ", " << vtx.position()[2]
                << std::endl;
      std::cout << count << ". Vertex with cov: " << vtx.covariance()
                << std::endl;
      std::cout << "\t with n tracks: " << vtx.tracks().size() << std::endl;
    }
  }
  // Test expected outcomes from athena implementation
  // Number of reconstructed vertices
  auto verticesInfo = std::get<VerticesData>(csvData);
  const int expNRecoVertices = verticesInfo.size();

  BOOST_CHECK_EQUAL(allVertices.size(), expNRecoVertices);
  std::vector<bool> vtxFound(expNRecoVertices, false);

  for (const auto& vtx : allVertices) {
    double vtxZ = vtx.position()[2];
    double diffZ = 1e5;
    int foundVtxIdx = -1;
    for (int i = 0; i < expNRecoVertices; i++) {
      if (!vtxFound[i]) {
        if (std::abs(vtxZ - verticesInfo[i].position[2]) < diffZ) {
          diffZ = std::abs(vtxZ - verticesInfo[i].position[2]);
          foundVtxIdx = i;
        }
      }
    }
    if (diffZ < 0.5_mm) {
      vtxFound[foundVtxIdx] = true;
      CHECK_CLOSE_ABS(vtx.tracks().size(), verticesInfo[foundVtxIdx].nTracks,
                      1);
    }
  }
  for (bool found : vtxFound) {
    BOOST_CHECK_EQUAL(found, true);
  }
}

/// @brief AMVF test with adaptive grid seed finder
BOOST_AUTO_TEST_CASE(
    adaptive_multi_vertex_finder_adaptive_grid_seed_finder_test) {
  // Set debug mode
  bool debugMode = false;
  if (debugMode) {
    std::cout << "Starting AMVF test with adaptive grid seed finder..."
              << std::endl;
  }
  // Set up constant B-Field
  auto bField = std::make_shared<ConstantBField>(Vector3(0., 0., 2_T));

  // Set up EigenStepper
  // EigenStepper<> stepper(bField);
  EigenStepper<> stepper(bField);

  // Set up propagator with void navigator
  auto propagator = std::make_shared<Propagator>(stepper);

  // IP Estimator
  ImpactPointEstimator::Config ipEstCfg(bField, propagator);
  ImpactPointEstimator ipEst(ipEstCfg);

  std::vector<double> temperatures{
      8., 4., 2., std::numbers::sqrt2, std::sqrt(3. / 2.), 1.};
  AnnealingUtility::Config annealingConfig;
  annealingConfig.setOfTemperatures = temperatures;
  AnnealingUtility annealingUtility(annealingConfig);

  using Fitter = AdaptiveMultiVertexFitter;

  Fitter::Config fitterCfg(ipEst);

  fitterCfg.annealingTool = annealingUtility;

  // Linearizer for BoundTrackParameters type test
  Linearizer::Config ltConfig;
  ltConfig.bField = bField;
  ltConfig.propagator = propagator;
  Linearizer linearizer(ltConfig);

  // Test smoothing
  fitterCfg.doSmoothing = true;
  fitterCfg.extractParameters.connect<&InputTrack::extractParameters>();
  fitterCfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(&linearizer);

  Fitter fitter(fitterCfg);

  // Grid density used during vertex seed finding
  AdaptiveGridTrackDensity::Config gridDensityCfg;
  // force track to have exactly spatialTrkGridSize spatial bins for testing
  // purposes
  gridDensityCfg.spatialTrkGridSizeRange = {55, 55};
  gridDensityCfg.spatialBinExtent = 0.05;
  AdaptiveGridTrackDensity gridDensity(gridDensityCfg);

  using SeedFinder = AdaptiveGridDensityVertexFinder;
  SeedFinder::Config seedFinderCfg(gridDensity);
  seedFinderCfg.cacheGridStateForTrackRemoval = true;
  seedFinderCfg.extractParameters.connect<&InputTrack::extractParameters>();

  auto seedFinder = std::make_shared<SeedFinder>(seedFinderCfg);

  AdaptiveMultiVertexFinder::Config finderConfig(
      std::move(fitter), std::move(seedFinder), ipEst, bField);
  finderConfig.extractParameters.connect<&InputTrack::extractParameters>();

  AdaptiveMultiVertexFinder finder(std::move(finderConfig));
  IVertexFinder::State state = finder.makeState(magFieldContext);

  auto csvData = readTracksAndVertexCSV(toolString);
  auto tracks = std::get<TracksData>(csvData);

  if (debugMode) {
    std::cout << "Number of tracks in event: " << tracks.size() << std::endl;
    int maxCout = 10;
    int count = 0;
    for (const auto& trk : tracks) {
      std::cout << count << ". track: " << std::endl;
      std::cout << "params: " << trk << std::endl;
      count++;
      if (count == maxCout) {
        break;
      }
    }
  }

  std::vector<InputTrack> inputTracks;
  for (const auto& trk : tracks) {
    inputTracks.emplace_back(&trk);
  }

  Vertex bsConstr = std::get<BeamSpotData>(csvData);
  VertexingOptions vertexingOptions(geoContext, magFieldContext, bsConstr);

  auto t1 = std::chrono::system_clock::now();
  auto findResult = finder.find(inputTracks, vertexingOptions, state);
  auto t2 = std::chrono::system_clock::now();

  auto timediff =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  if (!findResult.ok()) {
    std::cout << findResult.error().message() << std::endl;
  }

  BOOST_CHECK(findResult.ok());

  std::vector<Vertex> allVertices = *findResult;

  if (debugMode) {
    std::cout << "Time needed: " << timediff << " ms." << std::endl;
    std::cout << "Number of vertices reconstructed: " << allVertices.size()
              << std::endl;

    int count = 0;
    for (const auto& vtx : allVertices) {
      count++;
      std::cout << count << ". Vertex at position: " << vtx.position()[0]
                << ", " << vtx.position()[1] << ", " << vtx.position()[2]
                << std::endl;
      std::cout << count << ". Vertex with cov: " << vtx.covariance()
                << std::endl;
      std::cout << "\t with n tracks: " << vtx.tracks().size() << std::endl;
    }
  }
  // Test expected outcomes from athena implementation
  // Number of reconstructed vertices
  auto verticesInfo = std::get<VerticesData>(csvData);
  const int expNRecoVertices = verticesInfo.size();

  BOOST_CHECK_EQUAL(allVertices.size(), expNRecoVertices);
  std::vector<bool> vtxFound(expNRecoVertices, false);

  for (const auto& vtx : allVertices) {
    double vtxZ = vtx.position()[2];
    double diffZ = 1e5;
    int foundVtxIdx = -1;
    for (int i = 0; i < expNRecoVertices; i++) {
      if (!vtxFound[i]) {
        if (std::abs(vtxZ - verticesInfo[i].position[2]) < diffZ) {
          diffZ = std::abs(vtxZ - verticesInfo[i].position[2]);
          foundVtxIdx = i;
        }
      }
    }
    if (diffZ < 0.5_mm) {
      vtxFound[foundVtxIdx] = true;
      CHECK_CLOSE_ABS(vtx.tracks().size(), verticesInfo[foundVtxIdx].nTracks,
                      2);
    }
  }
  for (bool found : vtxFound) {
    BOOST_CHECK_EQUAL(found, true);
  }
}

}  // namespace Acts::Test
