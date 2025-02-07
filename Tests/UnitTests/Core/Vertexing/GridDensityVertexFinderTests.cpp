// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "Acts/Vertexing/AdaptiveGridDensityVertexFinder.hpp"
#include "Acts/Vertexing/AdaptiveGridTrackDensity.hpp"
#include "Acts/Vertexing/GaussianGridTrackDensity.hpp"
#include "Acts/Vertexing/GridDensityVertexFinder.hpp"
#include "Acts/Vertexing/IVertexFinder.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"

#include <iostream>
#include <memory>
#include <numbers>
#include <random>
#include <system_error>
#include <vector>

using namespace Acts::UnitLiterals;
using Acts::VectorHelpers::makeVector4;

namespace Acts::Test {

using Covariance = BoundSquareMatrix;

// Create a test context
GeometryContext geoContext = GeometryContext();
MagneticFieldContext magFieldContext = MagneticFieldContext();

const double zVertexPos1 = 12.;
const double zVertexPos2 = -3.;
// x position
std::normal_distribution<double> xdist(1_mm, 0.1_mm);
// y position
std::normal_distribution<double> ydist(-0.7_mm, 0.1_mm);
// z1 position
std::normal_distribution<double> z1dist(zVertexPos1 * 1_mm, 1_mm);
// z2 position
std::normal_distribution<double> z2dist(zVertexPos2 * 1_mm, 0.5_mm);
// Track pT distribution
std::uniform_real_distribution<double> pTDist(0.1_GeV, 100_GeV);
// Track phi distribution
std::uniform_real_distribution<double> phiDist(-std::numbers::pi,
                                               std::numbers::pi);
// Track eta distribution
std::uniform_real_distribution<double> etaDist(-4., 4.);

///
/// @brief Unit test for GridDensityVertexFinder without caching
/// of track density values
///
BOOST_AUTO_TEST_CASE(grid_density_vertex_finder_test) {
  bool debugMode = false;

  // Note that the AdaptiveGridTrackDensity and the GaussianGridTrackDensity
  // only furnish exactly the same results for uneven mainGridSize, where the
  // binnings of the two track densities align. For even mainGridSize the
  // binning of GaussianGridTrackDensity is:
  // ..., [-binSize, 0), [0, binSize), ...
  // and the binning of AdaptiveGridTrackDensity is:
  // ..., [-0.5*binSize, 0.5*binSize), [0.5*binSize, 1.5*binSize), ...
  // This is because the AdaptiveGridTrackDensity always has 0 as a bin center.
  // As a consequence of these different binnings, results would be shifted for
  // binSize/2 if mainGridSize is even.
  const int mainGridSize = 3001;
  const int trkGridSize = 35;

  Covariance covMat = Covariance::Identity();

  // Perigee surface for track parameters
  Vector3 pos0{0, 0, 0};
  std::shared_ptr<PerigeeSurface> perigeeSurface =
      Surface::makeShared<PerigeeSurface>(pos0);

  VertexingOptions vertexingOptions(geoContext, magFieldContext);

  using Finder1 = GridDensityVertexFinder;
  Finder1::Config cfg1{{{100, mainGridSize, trkGridSize}}};
  cfg1.cacheGridStateForTrackRemoval = false;
  cfg1.extractParameters.connect<&InputTrack::extractParameters>();
  Finder1 finder1(cfg1);
  IVertexFinder::State state1 = finder1.makeState(magFieldContext);

  // Use custom grid density here with same bin size as Finder1
  AdaptiveGridTrackDensity::Config adaptiveDensityConfig;
  adaptiveDensityConfig.spatialTrkGridSizeRange = {trkGridSize, trkGridSize};
  adaptiveDensityConfig.spatialBinExtent = 2. / 30.01 * 1_mm;
  AdaptiveGridTrackDensity adaptiveDensity(adaptiveDensityConfig);

  using Finder2 = AdaptiveGridDensityVertexFinder;
  Finder2::Config cfg2(adaptiveDensity);
  cfg2.cacheGridStateForTrackRemoval = false;
  cfg2.extractParameters.connect<&InputTrack::extractParameters>();
  Finder2 finder2(cfg2);
  IVertexFinder::State state2 = finder2.makeState(magFieldContext);

  int mySeed = 31415;
  std::mt19937 gen(mySeed);
  unsigned int nTracks = 200;

  std::vector<BoundTrackParameters> trackVec;
  trackVec.reserve(nTracks);

  // Create nTracks tracks for test case
  for (unsigned int i = 0; i < nTracks; i++) {
    // The position of the particle
    Vector3 pos(xdist(gen), ydist(gen), 0);

    // Create momentum and charge of track
    double pt = pTDist(gen);
    double phi = phiDist(gen);
    double eta = etaDist(gen);
    double charge = etaDist(gen) > 0 ? 1 : -1;

    // project the position on the surface
    Vector3 direction = makeDirectionFromPhiEta(phi, eta);
    auto intersection =
        perigeeSurface->intersect(geoContext, pos, direction).closest();
    pos = intersection.position();

    // Produce most of the tracks at near z1 position,
    // some near z2. Highest track density then expected at z1
    pos[eZ] = ((i % 4) == 0) ? z2dist(gen) : z1dist(gen);

    trackVec.push_back(BoundTrackParameters::create(
                           perigeeSurface, geoContext, makeVector4(pos, 0),
                           direction, charge / pt, covMat,
                           ParticleHypothesis::pion())
                           .value());
  }

  std::vector<InputTrack> inputTracks;
  for (const auto& trk : trackVec) {
    inputTracks.emplace_back(&trk);
  }

  auto res1 = finder1.find(inputTracks, vertexingOptions, state1);
  if (!res1.ok()) {
    std::cout << res1.error().message() << std::endl;
  }

  auto res2 = finder2.find(inputTracks, vertexingOptions, state2);
  if (!res2.ok()) {
    std::cout << res2.error().message() << std::endl;
  }

  double zResult1 = 0;
  if (res1.ok()) {
    BOOST_CHECK(!(*res1).empty());
    Vector3 result1 = (*res1).back().position();
    if (debugMode) {
      std::cout << "Vertex position result 1: " << result1.transpose()
                << std::endl;
    }
    CHECK_CLOSE_ABS(result1[eZ], zVertexPos1, 1_mm);
    zResult1 = result1[eZ];
  }

  double zResult2 = 0;
  if (res2.ok()) {
    BOOST_CHECK(!(*res2).empty());
    Vector3 result2 = (*res2).back().position();
    if (debugMode) {
      std::cout << "Vertex position result 2: " << result2.transpose()
                << std::endl;
    }
    CHECK_CLOSE_ABS(result2[eZ], zVertexPos1, 1_mm);
    zResult2 = result2[eZ];
  }

  // Both finders should give same results
  CHECK_CLOSE_REL(zResult1, zResult2, 1e-5);
}

BOOST_AUTO_TEST_CASE(grid_density_vertex_finder_track_caching_test) {
  bool debugMode = false;

  const int mainGridSize = 3001;
  const int trkGridSize = 35;

  Covariance covMat = Covariance::Identity();

  // Perigee surface for track parameters
  Vector3 pos0{0, 0, 0};
  std::shared_ptr<PerigeeSurface> perigeeSurface =
      Surface::makeShared<PerigeeSurface>(pos0);

  VertexingOptions vertexingOptions(geoContext, magFieldContext);

  using Finder1 = GridDensityVertexFinder;
  using GridDensity = GaussianGridTrackDensity;

  // Use custom grid density here
  GridDensity::Config densityConfig(100_mm, mainGridSize, trkGridSize);
  densityConfig.useHighestSumZPosition = true;
  GridDensity density(densityConfig);

  Finder1::Config cfg(density);
  cfg.cacheGridStateForTrackRemoval = true;
  cfg.extractParameters.connect<&InputTrack::extractParameters>();
  Finder1 finder1(cfg);

  // Use custom grid density here with same bin size as Finder1
  AdaptiveGridTrackDensity::Config adaptiveDensityConfig;
  adaptiveDensityConfig.spatialTrkGridSizeRange = {trkGridSize, trkGridSize};
  adaptiveDensityConfig.spatialBinExtent = 2. / 30.01 * 1_mm;
  adaptiveDensityConfig.useHighestSumZPosition = true;
  AdaptiveGridTrackDensity adaptiveDensity(adaptiveDensityConfig);

  using Finder2 = AdaptiveGridDensityVertexFinder;
  Finder2::Config cfg2(adaptiveDensity);
  cfg2.cacheGridStateForTrackRemoval = true;
  cfg2.extractParameters.connect<&InputTrack::extractParameters>();
  Finder2 finder2(cfg2);

  int mySeed = 31415;
  std::mt19937 gen(mySeed);
  unsigned int nTracks = 200;

  std::vector<BoundTrackParameters> trackVec;
  trackVec.reserve(nTracks);

  // Create nTracks tracks for test case
  for (unsigned int i = 0; i < nTracks; i++) {
    // The position of the particle
    Vector3 pos(xdist(gen), ydist(gen), 0);

    // Create momentum and charge of track
    double pt = pTDist(gen);
    double phi = phiDist(gen);
    double eta = etaDist(gen);
    double charge = etaDist(gen) > 0 ? 1 : -1;

    // project the position on the surface
    Vector3 direction = makeDirectionFromPhiEta(phi, eta);
    auto intersection =
        perigeeSurface->intersect(geoContext, pos, direction).closest();
    pos = intersection.position();

    // Produce most of the tracks at near z1 position,
    // some near z2. Highest track density then expected at z1
    pos[eZ] = ((i % 4) == 0) ? z2dist(gen) : z1dist(gen);

    trackVec.push_back(BoundTrackParameters::create(
                           perigeeSurface, geoContext, makeVector4(pos, 0),
                           direction, charge / pt, covMat,
                           ParticleHypothesis::pion())
                           .value());
  }

  std::vector<InputTrack> inputTracks;
  for (const auto& trk : trackVec) {
    inputTracks.emplace_back(&trk);
  }

  IVertexFinder::State state1 = finder1.makeState(magFieldContext);
  IVertexFinder::State state2 = finder2.makeState(magFieldContext);

  double zResult1 = 0;
  double zResult2 = 0;

  auto res1 = finder1.find(inputTracks, vertexingOptions, state1);
  if (!res1.ok()) {
    std::cout << res1.error().message() << std::endl;
  }
  if (res1.ok()) {
    BOOST_CHECK(!(*res1).empty());
    Vector3 result = (*res1).back().position();
    if (debugMode) {
      std::cout << "Vertex position after first fill 1: " << result.transpose()
                << std::endl;
    }
    CHECK_CLOSE_ABS(result[eZ], zVertexPos1, 1_mm);
    zResult1 = result[eZ];
  }

  auto res2 = finder2.find(inputTracks, vertexingOptions, state2);
  if (!res2.ok()) {
    std::cout << res2.error().message() << std::endl;
  }
  if (res2.ok()) {
    BOOST_CHECK(!(*res2).empty());
    Vector3 result = (*res2).back().position();
    if (debugMode) {
      std::cout << "Vertex position after first fill 2: " << result.transpose()
                << std::endl;
    }
    CHECK_CLOSE_ABS(result[eZ], zVertexPos1, 1_mm);
    zResult2 = result[eZ];
  }

  CHECK_CLOSE_REL(zResult1, zResult2, 1e-5);

  int trkCount = 0;
  std::vector<InputTrack> removedTracks;
  for (const auto& trk : trackVec) {
    if ((trkCount % 4) != 0) {
      removedTracks.emplace_back(&trk);
    }
    trkCount++;
  }

  state1.as<Finder1::State>().tracksToRemove = removedTracks;
  state2.as<Finder2::State>().tracksToRemove = removedTracks;

  auto res3 = finder1.find(inputTracks, vertexingOptions, state1);
  if (!res3.ok()) {
    std::cout << res3.error().message() << std::endl;
  }
  if (res3.ok()) {
    BOOST_CHECK(!(*res3).empty());
    Vector3 result = (*res3).back().position();
    if (debugMode) {
      std::cout
          << "Vertex position after removing tracks near first density peak 1: "
          << result << std::endl;
    }
    CHECK_CLOSE_ABS(result[eZ], zVertexPos2, 1_mm);
    zResult1 = result[eZ];
  }

  auto res4 = finder2.find(inputTracks, vertexingOptions, state2);
  if (!res4.ok()) {
    std::cout << res4.error().message() << std::endl;
  }
  if (res4.ok()) {
    BOOST_CHECK(!(*res4).empty());
    Vector3 result = (*res4).back().position();
    if (debugMode) {
      std::cout
          << "Vertex position after removing tracks near first density peak 2: "
          << result << std::endl;
    }
    CHECK_CLOSE_ABS(result[eZ], zVertexPos2, 1_mm);
    zResult2 = result[eZ];
  }

  CHECK_CLOSE_REL(zResult1, zResult2, 1e-5);
}

///
/// @brief Unit test for GridDensityVertexFinder with seed with estimation
///
BOOST_AUTO_TEST_CASE(grid_density_vertex_finder_seed_width_test) {
  bool debugMode = false;

  const int mainGridSize = 3001;
  const int trkGridSize = 35;

  Covariance covMat = Covariance::Identity();

  // Perigee surface for track parameters
  Vector3 pos0{0, 0, 0};
  std::shared_ptr<PerigeeSurface> perigeeSurface =
      Surface::makeShared<PerigeeSurface>(pos0);

  VertexingOptions vertexingOptions(geoContext, magFieldContext);
  Vertex constraintVtx;
  constraintVtx.setCovariance(SquareMatrix3::Identity());
  vertexingOptions.constraint = constraintVtx;

  using Finder1 = GridDensityVertexFinder;
  Finder1::Config cfg1{{{100, mainGridSize, trkGridSize}}};
  cfg1.cacheGridStateForTrackRemoval = false;
  cfg1.estimateSeedWidth = true;
  cfg1.extractParameters.connect<&InputTrack::extractParameters>();
  Finder1 finder1(cfg1);
  IVertexFinder::State state1 = finder1.makeState(magFieldContext);

  // Use custom grid density here with same bin size as Finder1
  AdaptiveGridTrackDensity::Config adaptiveDensityConfig;
  adaptiveDensityConfig.spatialTrkGridSizeRange = {trkGridSize, trkGridSize};
  adaptiveDensityConfig.spatialBinExtent = 2. / 30.01 * 1_mm;
  AdaptiveGridTrackDensity adaptiveDensity(adaptiveDensityConfig);

  using Finder2 = AdaptiveGridDensityVertexFinder;
  Finder2::Config cfg2(adaptiveDensity);
  cfg2.cacheGridStateForTrackRemoval = false;
  cfg2.estimateSeedWidth = true;
  cfg2.extractParameters.connect<&InputTrack::extractParameters>();
  Finder2 finder2(cfg2);
  IVertexFinder::State state2 = finder2.makeState(magFieldContext);

  int mySeed = 31415;
  std::mt19937 gen(mySeed);
  unsigned int nTracks = 20;

  std::vector<BoundTrackParameters> trackVec;
  trackVec.reserve(nTracks);

  // Create nTracks tracks for test case
  for (unsigned int i = 0; i < nTracks; i++) {
    // The position of the particle
    Vector3 pos(xdist(gen), ydist(gen), 0);

    // Create momentum and charge of track
    double pt = pTDist(gen);
    double phi = phiDist(gen);
    double eta = etaDist(gen);
    double charge = etaDist(gen) > 0 ? 1 : -1;

    // project the position on the surface
    Vector3 direction = makeDirectionFromPhiEta(phi, eta);
    auto intersection =
        perigeeSurface->intersect(geoContext, pos, direction).closest();
    pos = intersection.position();

    pos[eZ] = z1dist(gen);

    trackVec.push_back(BoundTrackParameters::create(
                           perigeeSurface, geoContext, makeVector4(pos, 0),
                           direction, charge / pt, covMat,
                           ParticleHypothesis::pion())
                           .value());
  }

  std::vector<InputTrack> inputTracks;
  for (const auto& trk : trackVec) {
    inputTracks.emplace_back(&trk);
  }

  // Test finder 1
  auto res1 = finder1.find(inputTracks, vertexingOptions, state1);
  if (!res1.ok()) {
    std::cout << res1.error().message() << std::endl;
  }

  double covZZ1 = 0;
  if (res1.ok()) {
    BOOST_CHECK(!(*res1).empty());
    SquareMatrix3 cov = (*res1).back().covariance();
    BOOST_CHECK_NE(constraintVtx.covariance(), cov);
    BOOST_CHECK_NE(cov(eZ, eZ), 0.);
    covZZ1 = cov(eZ, eZ);
    if (debugMode) {
      std::cout << "Estimated z-seed width 1: " << cov(eZ, eZ) << std::endl;
    }
  }

  // Test finder 2
  auto res2 = finder2.find(inputTracks, vertexingOptions, state2);
  if (!res2.ok()) {
    std::cout << res2.error().message() << std::endl;
  }

  double covZZ2 = 0;
  if (res2.ok()) {
    BOOST_CHECK(!(*res2).empty());
    SquareMatrix3 cov = (*res2).back().covariance();
    BOOST_CHECK_NE(constraintVtx.covariance(), cov);
    BOOST_CHECK_NE(cov(eZ, eZ), 0.);
    covZZ2 = cov(eZ, eZ);
    if (debugMode) {
      std::cout << "Estimated z-seed width 2: " << cov(eZ, eZ) << std::endl;
    }
  }

  // Test for same seed width
  CHECK_CLOSE_ABS(covZZ1, covZZ2, 1e-4);
}

}  // namespace Acts::Test
