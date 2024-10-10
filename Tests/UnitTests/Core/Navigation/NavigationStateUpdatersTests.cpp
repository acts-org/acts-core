// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Navigation/NavigationStateFillers.hpp"
#include "Acts/Navigation/NavigationStateUpdaters.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/IAxis.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "../Surfaces/SurfaceStub.hpp"

// A test context
Acts::GeometryContext tContext;

namespace Acts {

namespace Experimental {

class Detector;

/// A detector volume
class DetectorVolume {
 public:
  const Detector* d = nullptr;
  std::vector<const Surface*> sfs = {};
  std::vector<const Portal*> prts = {};
  const std::vector<const Surface*> surfaces() const { return sfs; }
  const std::vector<const Portal*> portals() const { return prts; }
  const Detector* detector() const { return d; };
};

/// A detector
class Detector {
 public:
  std::vector<const DetectorVolume*> vs = {};
  const std::vector<const DetectorVolume*> volumes() const { return vs; }
};

/// Helper extractors: all surfaces
struct AllPortalsExtractor {
  /// Extract the surfaces from the volume
  ///
  /// @param gctx the geometry contextfor this extraction call
  /// @param nState is the navigation state for the extraction
  /// @param indices is an ignored index vector
  inline static const std::vector<const Portal*> extract(
      [[maybe_unused]] const GeometryContext& gctx,
      const NavigationState& nState,
      [[maybe_unused]] const std::vector<std::size_t>& indices = {}) {
    return nState.currentVolume->portals();
  }
};

/// Helper extractors: all surfaces
struct AllSurfacesExtractor {
  /// Extract the surfaces from the volume
  ///
  /// @param gctx the geometry contextfor this extraction call
  /// @param nState is the navigation state for the extraction
  /// @param indices is an ignored index vector
  inline static const std::vector<const Surface*> extract(
      [[maybe_unused]] const GeometryContext& gctx,
      const NavigationState& nState,
      [[maybe_unused]] const std::vector<std::size_t>& indices = {}) {
    return nState.currentVolume->surfaces();
  }
};

/// Helper extractors: indexed surfaces
struct IndexedSurfacesExtractor {
  /// Extract the surfaces from the volume
  ///
  /// @param gctx the geometry contextfor this extraction call
  /// @param nState is the navigation state for the extraction
  /// @param indices are access indices into the surfaces store
  inline static const std::vector<const Surface*> extract(
      [[maybe_unused]] const GeometryContext& gctx,
      const NavigationState& nState, const std::vector<std::size_t>& indices) {
    // Get the surface container
    const auto& surfaces = nState.currentVolume->surfaces();
    // The extracted surfaces
    std::vector<const Surface*> eSurfaces;
    eSurfaces.reserve(indices.size());
    std::for_each(indices.begin(), indices.end(),
                  [&](const auto& i) { eSurfaces.push_back(surfaces[i]); });
    return eSurfaces;
  }
};

}  // namespace Experimental

class TestAxis : public IAxis {
 public:
  TestAxis() = default;

  bool isEquidistant() const final { return true; }

  bool isVariable() const final { return false; }

  AxisType getType() const final { return AxisType::Equidistant; }

  AxisBoundaryType getBoundaryType() const final {
    return AxisBoundaryType::Closed;
  }

  std::vector<ActsScalar> getBinEdges() const final { return {-1, 1}; }

  ActsScalar getMin() const final { return -1.; }

  ActsScalar getMax() const final { return 1.; }

  std::size_t getNBins() const final { return 1; };

  void toStream(std::ostream& os) const final { os << "TextAxis"; }
};

class MultiGrid1D {
 public:
  static constexpr std::size_t DIM = 1u;
  using point_t = std::array<ActsScalar, DIM>;

  const std::vector<std::size_t>& atPosition(
      const std::array<ActsScalar, 1u>& /*position*/) const {
    return e;
  }

  std::array<const IAxis*, DIM> axes() const { return {&ta}; }
  TestAxis ta = TestAxis();

 private:
  std::vector<std::size_t> e = {0u, 1u};
};

class MultiGrid2D {
 public:
  static constexpr std::size_t DIM = 2u;
  using point_t = std::array<ActsScalar, DIM>;

  const std::vector<std::size_t>& atPosition(
      const std::array<ActsScalar, 2u>& /*position*/) const {
    return e;
  }

  std::array<const IAxis*, DIM> axes() const { return {&ta, &ta}; };
  TestAxis ta = TestAxis();

 private:
  std::vector<std::size_t> e = {1u};
};
}  // namespace Acts

using SingleVolumeUpdater = Acts::Experimental::SingleObjectNavigation<
    Acts::Experimental::IExternalNavigation, Acts::Experimental::DetectorVolume,
    Acts::Experimental::DetectorVolumeFiller>;

using AllSurfacesProvider = Acts::Experimental::StaticAccessNavigation<
    Acts::Experimental::IInternalNavigation,
    Acts::Experimental::AllSurfacesExtractor,
    Acts::Experimental::SurfacesFiller>;

using AllPortalsProvider = Acts::Experimental::StaticAccessNavigation<
    Acts::Experimental::IInternalNavigation,
    Acts::Experimental::AllPortalsExtractor, Acts::Experimental::PortalsFiller>;

auto surfaceA = Acts::Surface::makeShared<Acts::SurfaceStub>();
auto surfaceB = Acts::Surface::makeShared<Acts::SurfaceStub>();
auto surfaceC = Acts::Surface::makeShared<Acts::SurfaceStub>();

auto pSurfaceA = Acts::Surface::makeShared<Acts::SurfaceStub>();
auto pSurfaceB = Acts::Surface::makeShared<Acts::SurfaceStub>();
auto portalA = std::make_shared<Acts::Experimental::Portal>(pSurfaceA);
auto portalB = std::make_shared<Acts::Experimental::Portal>(pSurfaceB);

BOOST_AUTO_TEST_SUITE(Experimental)

BOOST_AUTO_TEST_CASE(SingleExternalNavigationDelegate) {
  Acts::Experimental::NavigationState nState;

  // Create a single object and a single object updator
  auto sVolume = std::make_shared<Acts::Experimental::DetectorVolume>();
  SingleVolumeUpdater sVolumeUpdater(sVolume.get());

  // Update the volume and check that it is indeed updated
  sVolumeUpdater.update(tContext, nState);
  BOOST_CHECK_EQUAL(nState.currentVolume, sVolume.get());
}

BOOST_AUTO_TEST_CASE(AllSurfaces) {
  // Create a single object and a single object updator
  auto dVolume = std::make_shared<Acts::Experimental::DetectorVolume>();
  (*dVolume).sfs = {surfaceA.get(), surfaceB.get(), surfaceC.get()};
  (*dVolume).prts = {portalA.get(), portalB.get()};

  Acts::Experimental::NavigationState nState;
  nState.currentVolume = dVolume.get();
  BOOST_CHECK(nState.surfaceCandidates.empty());
  AllSurfacesProvider allSurfaces;
  allSurfaces.update(tContext, nState);
  BOOST_CHECK_EQUAL(nState.surfaceCandidates.size(), 3u);
}

BOOST_AUTO_TEST_CASE(AllPortals) {
  // Create a single object and a single object updator
  auto dVolume = std::make_shared<Acts::Experimental::DetectorVolume>();
  (*dVolume).sfs = {surfaceA.get(), surfaceB.get(), surfaceC.get()};
  (*dVolume).prts = {portalA.get(), portalB.get()};

  Acts::Experimental::NavigationState nState;
  nState.currentVolume = dVolume.get();
  BOOST_CHECK(nState.surfaceCandidates.empty());
  AllPortalsProvider allPortals;
  allPortals.update(tContext, nState);
  BOOST_CHECK_EQUAL(nState.surfaceCandidates.size(), 2u);
}

BOOST_AUTO_TEST_CASE(AllPortalsAllSurfaces) {
  // Create a single object and a single object updator
  auto dVolume = std::make_shared<Acts::Experimental::DetectorVolume>();
  (*dVolume).sfs = {surfaceA.get(), surfaceB.get(), surfaceC.get()};
  (*dVolume).prts = {portalA.get(), portalB.get()};

  Acts::Experimental::NavigationState nState;
  nState.currentVolume = dVolume.get();
  BOOST_CHECK(nState.surfaceCandidates.empty());

  AllPortalsProvider allPortals;
  AllSurfacesProvider allSurfaces;
  auto allPortalsAllSurfaces = Acts::Experimental::ChainedNavigation<
      Acts::Experimental::IInternalNavigation, AllPortalsProvider,
      AllSurfacesProvider>(std::tie(allPortals, allSurfaces));

  allPortalsAllSurfaces.update(tContext, nState);
  BOOST_CHECK_EQUAL(nState.surfaceCandidates.size(), 5u);
}

BOOST_AUTO_TEST_CASE(AllPortalsGrid1DSurfaces) {
  // Create a single object and a single object updator
  auto dVolume = std::make_shared<Acts::Experimental::DetectorVolume>();
  (*dVolume).sfs = {surfaceA.get(), surfaceB.get(), surfaceC.get()};
  (*dVolume).prts = {portalA.get(), portalB.get()};

  Acts::Experimental::NavigationState nState;
  nState.currentVolume = dVolume.get();
  BOOST_CHECK(nState.surfaceCandidates.empty());

  AllPortalsProvider allPortals;
  Acts::MultiGrid1D grid;
  using Grid1DSurfacesProvider = Acts::Experimental::IndexedGridNavigation<
      Acts::Experimental::IInternalNavigation, decltype(grid),
      Acts::Experimental::IndexedSurfacesExtractor,
      Acts::Experimental::SurfacesFiller>;
  auto grid1DSurfaces =
      Grid1DSurfacesProvider(std::move(grid), {Acts::BinningValue::binR});

  auto allPortalsGrid1DSurfaces = Acts::Experimental::ChainedNavigation<
      Acts::Experimental::IInternalNavigation, AllPortalsProvider,
      Grid1DSurfacesProvider>(std::tie(allPortals, grid1DSurfaces));

  allPortalsGrid1DSurfaces.update(tContext, nState);
  BOOST_CHECK_EQUAL(nState.surfaceCandidates.size(), 4u);
}

BOOST_AUTO_TEST_CASE(AllPortalsGrid2DSurfaces) {
  // Create a single object and a single object updator
  auto dVolume = std::make_shared<Acts::Experimental::DetectorVolume>();
  (*dVolume).sfs = {surfaceA.get(), surfaceB.get(), surfaceC.get()};
  (*dVolume).prts = {portalA.get(), portalB.get()};

  Acts::Experimental::NavigationState nState;
  nState.currentVolume = dVolume.get();
  BOOST_CHECK(nState.surfaceCandidates.empty());

  AllPortalsProvider allPortals;
  Acts::MultiGrid2D grid;
  using Grid2DSurfacesProvider = Acts::Experimental::IndexedGridNavigation<
      Acts::Experimental::IInternalNavigation, decltype(grid),
      Acts::Experimental::IndexedSurfacesExtractor,
      Acts::Experimental::SurfacesFiller>;
  auto grid2DSurfaces = Grid2DSurfacesProvider(
      std::move(grid), {Acts::BinningValue::binR, Acts::BinningValue::binZ});

  auto allPortalsGrid2DSurfaces = Acts::Experimental::ChainedNavigation<
      Acts::Experimental::IInternalNavigation, AllPortalsProvider,
      Grid2DSurfacesProvider>(std::tie(allPortals, grid2DSurfaces));

  allPortalsGrid2DSurfaces.update(tContext, nState);
  BOOST_CHECK_EQUAL(nState.surfaceCandidates.size(), 3u);
}

BOOST_AUTO_TEST_SUITE_END()
