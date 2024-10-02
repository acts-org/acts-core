// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/IndexedRootVolumeFinderBuilder.hpp"

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/detail/CylindricalDetectorHelper.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"

namespace {

template <typename Grid2D>
void fillGridIndices2D(
    const Acts::GeometryContext& gctx, Grid2D& grid,
    const std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>&
        rootVolumes,
    const std::array<std::vector<Acts::ActsScalar>, 2u>& boundaries,
    const std::array<Acts::BinningValue, 2u>& casts) {
  // Brute force loop over all bins & all volumes
  for (const auto [ic0, c0] : Acts::enumerate(boundaries[0u])) {
    if (ic0 > 0) {
      Acts::ActsScalar v0 = 0.5 * (c0 + boundaries[0u][ic0 - 1]);
      for (const auto [ic1, c1] : Acts::enumerate(boundaries[1u])) {
        if (ic1 > 0) {
          Acts::ActsScalar v1 = 0.5 * (c1 + boundaries[1u][ic1 - 1]);
          if (casts ==
              std::array<Acts::BinningValue, 2u>{Acts::BinningValue::binZ,
                                                 Acts::BinningValue::binR}) {
            Acts::Vector3 zrPosition{v1, 0., v0};
            for (const auto [iv, v] : Acts::enumerate(rootVolumes)) {
              if (v->inside(gctx, zrPosition)) {
                typename Grid2D::point_t p{v0, v1};
                grid.atPosition(p) = iv;
              }
            }
          }
        }
      }
    }
  }
}
}  // namespace

Acts::Experimental::IndexedRootVolumeFinderBuilder::
    IndexedRootVolumeFinderBuilder(std::vector<Acts::BinningValue> binning)
    : m_casts(std::move(binning)) {
  if (m_casts != std::vector<Acts::BinningValue>{Acts::BinningValue::binZ,
                                                 Acts::BinningValue::binR}) {
    throw std::invalid_argument("Online (z,r) binning is currently supported.");
  }
}

Acts::Experimental::ExternalNavigationDelegate
Acts::Experimental::IndexedRootVolumeFinderBuilder::construct(
    const GeometryContext& gctx,
    const std::vector<std::shared_ptr<DetectorVolume>>& rootVolumes) const {
  auto rzphis =
      detail::CylindricalDetectorHelper::rzphiBoundaries(gctx, rootVolumes);

  using AxesGeneratorType = Acts::GridAxisGenerators::VarBoundVarBound;

  AxesGeneratorType zrAxes{rzphis[1], rzphis[0]};

  // Create the grid with the provided axis generator
  using GridType = typename AxesGeneratorType::template grid_type<std::size_t>;
  GridType grid(zrAxes());

  auto casts = std::array<BinningValue, 2u>{m_casts[0u], m_casts[1u]};

  auto boundaries =
      std::array<std::vector<ActsScalar>, 2u>{rzphis[1], rzphis[0]};
  fillGridIndices2D(gctx, grid, rootVolumes, boundaries, casts);

  using IndexedDetectorVolumesImpl =
      IndexedGridNavigation<IExternalNavigation, GridType,
                            IndexedDetectorVolumeExtractor,
                            DetectorVolumeFiller>;

  auto indexedDetectorVolumeImpl =
      std::make_unique<const IndexedDetectorVolumesImpl>(std::move(grid),
                                                         casts);

  // Return the root volume finder
  ExternalNavigationDelegate rootVolumeFinder;
  rootVolumeFinder.connect<&IndexedDetectorVolumesImpl::update>(
      std::move(indexedDetectorVolumeImpl));
  return rootVolumeFinder;
}
