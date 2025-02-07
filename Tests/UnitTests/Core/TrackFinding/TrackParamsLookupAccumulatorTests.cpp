// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/tools/detail/fwd.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/TrackFinding/TrackParamsLookupAccumulator.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"

#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <vector>

BOOST_AUTO_TEST_SUITE(TrackParamsLookupAccumulator)

Acts::GeometryContext gctx;

using Axis =
    Acts::Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Open>;
using AxisGen = Acts::GridAxisGenerators::EqOpenEqOpen;

using CellBound = std::pair<std::shared_ptr<Acts::BoundTrackParameters>,
                            std::shared_ptr<Acts::BoundTrackParameters>>;

using GridBound = Acts::Grid<CellBound, Axis, Axis>;
using AccBound = Acts::TrackParamsLookupAccumulator<GridBound>;

using CellCurvilinear =
    std::pair<std::shared_ptr<Acts::CurvilinearTrackParameters>,
              std::shared_ptr<Acts::CurvilinearTrackParameters>>;

using GridCurvilinear = Acts::Grid<CellCurvilinear, Axis, Axis>;
using AccCurvilinear = Acts::TrackParamsLookupAccumulator<GridCurvilinear>;

using CellFree = std::pair<std::shared_ptr<Acts::FreeTrackParameters>,
                           std::shared_ptr<Acts::FreeTrackParameters>>;

using GridFree = Acts::Grid<CellFree, Axis, Axis>;
using AccFree = Acts::TrackParamsLookupAccumulator<GridFree>;

AxisGen axisGen{{-1, 1}, 2, {-1, 1}, 2};

BOOST_AUTO_TEST_CASE(Exceptions) {
  // Instantiate grid
  GridBound grid(axisGen());
  AccBound acc(grid);

  // Create a reference surface for bound parameters
  auto transform = Acts::Transform3::Identity();
  auto bounds1 = std::make_shared<Acts::RectangleBounds>(1, 1);
  auto bounds2 = std::make_shared<Acts::RectangleBounds>(2, 2);

  auto surf1 =
      Acts::Surface::makeShared<Acts::PlaneSurface>(transform, bounds1);

  auto surf2 =
      Acts::Surface::makeShared<Acts::PlaneSurface>(transform, bounds2);

  // Create parameters to accumulate
  Acts::Vector4 pos{1, 2, 0, 4};
  Acts::Vector3 dir{1, 0, 0};
  double P = 1.;

  auto hypothesis1 = Acts::ParticleHypothesis::electron();
  auto hypothesis2 = Acts::ParticleHypothesis::muon();

  auto pars1 = Acts::BoundTrackParameters::create(surf1, gctx, pos, dir, 1. / P,
                                                  std::nullopt, hypothesis1)
                   .value();

  auto pars2 = Acts::BoundTrackParameters::create(surf2, gctx, pos, dir, 1. / P,
                                                  std::nullopt, hypothesis1)
                   .value();

  auto pars3 = Acts::BoundTrackParameters::create(surf1, gctx, pos, dir, 1. / P,
                                                  std::nullopt, hypothesis2)
                   .value();

  auto pars4 = Acts::BoundTrackParameters::create(
                   surf1, gctx, pos, dir, -1. / P, std::nullopt, hypothesis2)
                   .value();

  // Get the point of the grid
  auto bin = grid.localBinsFromGlobalBin(2);
  auto center = grid.binCenter(bin);
  Acts::Vector2 loc{center.at(0), center.at(1)};

  // Fill in grid
  acc.addTrack(pars1, pars1, loc);

  // Different reference surfaces
  BOOST_CHECK_THROW(acc.addTrack(pars2, pars2, loc), std::invalid_argument);

  // Different particle hypotheses
  BOOST_CHECK_THROW(acc.addTrack(pars3, pars3, loc), std::invalid_argument);

  // Different charges
  BOOST_CHECK_THROW(acc.addTrack(pars4, pars4, loc), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Accumulation) {
  // Instantiate grids
  GridBound gridBound(axisGen());
  AccBound accBound(gridBound);

  GridCurvilinear gridCurvilinear(axisGen());
  AccCurvilinear accCurvilinear(gridCurvilinear);

  GridFree gridFree(axisGen());
  AccFree accFree(gridFree);

  // Create a reference surface for bound parameters
  auto transform = Acts::Transform3::Identity();
  auto bounds = std::make_shared<Acts::RectangleBounds>(1, 1);
  auto surf = Acts::Surface::makeShared<Acts::PlaneSurface>(transform, bounds);

  auto hypothesis = Acts::ParticleHypothesis::electron();

  std::vector<Acts::Vector4> avgPoss;
  std::vector<Acts::Vector3> avgMoms;
  Acts::Vector4 pos{1, 2, 0, 4};
  for (std::size_t i = 0; i < gridBound.size(); i++) {
    // Create parameters to accumulate
    std::array<Acts::Vector4, 4> fourPositions = {pos * (i + 1), pos * (i + 2),
                                                  pos * (i + 3), pos * (i + 4)};

    std::array<double, 4> thetas = {
        std::numbers::pi / (i + 1), std::numbers::pi / (i + 2),
        std::numbers::pi / (i + 3), std::numbers::pi / (i + 4)};

    std::array<double, 4> phis = {
        2 * std::numbers::pi / (i + 1), 2 * std::numbers::pi / (i + 2),
        2 * std::numbers::pi / (i + 3), 2 * std::numbers::pi / (i + 4)};

    double P = 1.5 * (i + 1);

    // Get the point of the grid
    auto bin = gridBound.localBinsFromGlobalBin(i);
    auto center = gridBound.binCenter(bin);
    Acts::Vector2 loc{center.at(0), center.at(1)};

    // Accumulate
    Acts::Vector4 avgPos{0, 0, 0, 0};
    Acts::Vector3 avgMom{0, 0, 0};
    for (std::size_t j = 0; j < 4; j++) {
      Acts::Vector3 direction{std::sin(thetas.at(j)) * std::cos(phis.at(j)),
                              std::sin(thetas.at(j)) * std::sin(phis.at(j)),
                              std::cos(thetas.at(j))};

      avgPos += fourPositions.at(j);
      avgMom += P * direction;

      // Fill in each grid
      auto parsBound = Acts::BoundTrackParameters::create(
                           surf, gctx, fourPositions.at(j), direction, 1. / P,
                           std::nullopt, hypothesis)
                           .value();

      auto parsCurvilinear = Acts::CurvilinearTrackParameters(
          fourPositions.at(j), direction, 1. / P, std::nullopt, hypothesis);

      auto parsFree = Acts::FreeTrackParameters(
          fourPositions.at(j), direction, 1. / P, std::nullopt, hypothesis);

      accBound.addTrack(parsBound, parsBound, loc);
      accCurvilinear.addTrack(parsCurvilinear, parsCurvilinear, loc);
      accFree.addTrack(parsFree, parsFree, loc);
    }
    avgPoss.push_back(avgPos / fourPositions.size());
    avgMoms.push_back(avgMom / fourPositions.size());
  }

  // Finalize and compare
  GridBound avgGridBound = accBound.finalizeLookup();
  GridCurvilinear avgGridCurvilinear = accCurvilinear.finalizeLookup();
  GridFree avgGridFree = accFree.finalizeLookup();
  for (std::size_t i = 0; i < avgGridBound.size(); i++) {
    auto [ipBound, refBound] = avgGridBound.at(i);
    auto [ipCurvilinear, refCurvilinear] = avgGridCurvilinear.at(i);
    auto [ipFree, refFree] = avgGridFree.at(i);

    Acts::Vector4 avgPos = avgPoss.at(i);

    Acts::Vector3 avgMom = avgMoms.at(i);
    Acts::Vector3 avgDir = avgMom.normalized();
    double avgP = avgMom.norm();

    CHECK_CLOSE_ABS(ipBound->fourPosition(gctx), avgPos, 1e-3);
    CHECK_CLOSE_ABS(ipBound->direction(), avgDir, 1e-3);
    CHECK_CLOSE_ABS(ipBound->absoluteMomentum(), avgP, 1e-3);

    CHECK_CLOSE_ABS(ipCurvilinear->fourPosition(), avgPos, 1e-3);
    CHECK_CLOSE_ABS(ipCurvilinear->direction(), avgDir, 1e-3);
    CHECK_CLOSE_ABS(ipCurvilinear->absoluteMomentum(), avgP, 1e-3);

    CHECK_CLOSE_ABS(ipFree->fourPosition(), avgPos, 1e-3);
    CHECK_CLOSE_ABS(ipFree->direction(), avgDir, 1e-3);
    CHECK_CLOSE_ABS(ipFree->absoluteMomentum(), avgP, 1e-3);
  }
}

BOOST_AUTO_TEST_SUITE_END()
