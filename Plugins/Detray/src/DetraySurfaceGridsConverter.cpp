// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Detray/DetraySurfaceGridsConverter.hpp"

#include "Acts/Detector/Detector.hpp"
#include "Acts/Plugins/Detray/DetrayConversionUtils.hpp"
#include "Acts/Plugins/Json/DetrayJsonHelper.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"

#include <stdexcept>

namespace Acts {

/// DetraySurfaceGridsConverter converts surface grids from acts to detray
/// format

// convertAxis
detray::io::axis_payload Acts::DetraySurfaceGridsConverter::convertAxis(
    const Acts::IAxis& ia) {
  detray::io::axis_payload axis_pd;
  axis_pd.bounds = ia.getBoundaryType() == Acts::AxisBoundaryType::Bound
                       ? detray::axis::bounds::e_closed
                       : detray::axis::bounds::e_circular;
  axis_pd.binning = ia.isEquidistant() ? detray::axis::binning::e_regular
                                       : detray::axis::binning::e_irregular;
  axis_pd.bins = ia.getNBins();
  if (ia.isEquidistant()) {
    axis_pd.edges = {ia.getBinEdges().front(), ia.getBinEdges().back()};
  } else {
    axis_pd.edges = ia.getBinEdges();
  }

  return axis_pd;
}

// convertGrid
template <typename grid_type>
detray::io::grid_payload<std::size_t, detray::io::accel_id>
Acts::DetraySurfaceGridsConverter::convertGrid(const grid_type& grid,
                                               bool swapAxis) {
  // Get the grid axes & potentially swap them
  detray::io::grid_payload<std::size_t, detray::io::accel_id> grid_pd;

  auto axes = grid.axes();
  if (swapAxis && grid_type::DIM == 2u) {
    std::swap(axes[0u], axes[1u]);
  }

  // Fill the axes in the order they are
  for (unsigned int ia = 0u; ia < grid_type::DIM; ++ia) {
    detray::io::axis_payload axis_pd = convertAxis(*axes[ia]);
    axis_pd.label = static_cast<detray::axis::label>(ia);
    grid_pd.axes.push_back(axis_pd);  // push axis to axes
  }

  // 1D connections
  if constexpr (grid_type::DIM == 1u) {
    for (unsigned int ib0 = 1u; ib0 <= axes[0u]->getNBins(); ++ib0) {
      // Lookup bin
      typename grid_type::index_t lbin;
      detray::io::grid_bin_payload<std::size_t> grid_bin_pd;

      lbin[0u] = ib0;
      grid_bin_pd.content = grid.atLocalBins(lbin);
      // Corrected bin for detray
      lbin[0u] = ib0 - 1u;
      grid_bin_pd.loc_index =
          std::vector<unsigned int>(lbin.begin(), lbin.end());
      grid_pd.bins.push_back(grid_bin_pd);
    }
  }

  // 2D connections
  if constexpr (grid_type::DIM == 2u) {
    for (unsigned int ib0 = 1u; ib0 <= axes[0u]->getNBins(); ++ib0) {
      for (unsigned int ib1 = 1u; ib1 <= axes[1u]->getNBins(); ++ib1) {
        typename grid_type::index_t lbin;
        // Lookup bin - respect swap (if it happened) for the lookup
        lbin[0u] = swapAxis ? ib1 : ib0;
        lbin[1u] = swapAxis ? ib0 : ib1;

        detray::io::grid_bin_payload<std::size_t> grid_bin_pd;

        nlohmann::json jBin;
        grid_bin_pd.content = grid.atLocalBins(lbin);
        // Corrected bin for detray
        lbin[0u] = ib0 - 1u;
        lbin[1u] = ib1 - 1u;
        grid_bin_pd.loc_index =
            std::vector<unsigned int>(lbin.begin(), lbin.end());
        grid_pd.bins.push_back(grid_bin_pd);
      }
    }
  }

  return grid_pd;
}

template <typename index_grid>
detray::io::grid_payload<std::size_t, detray::io::accel_id>
Acts::DetraySurfaceGridsConverter::convertImpl(const index_grid& indexGrid) {
  bool swapAxes = true;

  if constexpr (index_grid::grid_type::DIM == 2u) {
    // Check for axis swap
    swapAxes = (indexGrid.casts[0u] == Acts::BinningValue::binZ &&
                indexGrid.casts[1u] == Acts::BinningValue::binPhi);
  }

  detray::io::grid_payload<std::size_t, detray::io::accel_id> grid_pd =
      convertGrid(indexGrid.grid, swapAxes);

  return grid_pd;
}

// convert
template <typename instance_type>
std::optional<detray::io::grid_payload<std::size_t, detray::io::accel_id>>
Acts::DetraySurfaceGridsConverter::convert(
    const Acts::Experimental::InternalNavigationDelegate& delegate,
    [[maybe_unused]] const instance_type& refInstance) {
  using GridType =
      typename instance_type::template grid_type<std::vector<std::size_t>>;
  // Defining a Delegate type
  using ConversionDelegateType =
      Acts::Experimental::IndexedSurfacesAllPortalsNavigation<
          GridType, Acts::Experimental::IndexedSurfacesNavigation>;
  using SubDelegateType =
      Acts::Experimental::IndexedSurfacesNavigation<GridType>;

  // Get the instance
  const auto* instance = delegate.instance();
  auto castedDelegate = dynamic_cast<const ConversionDelegateType*>(instance);

  if (castedDelegate != nullptr) {
    // Get the surface updator
    detray::io::grid_payload<std::size_t, detray::io::accel_id> grid_pd;
    auto indexedSurfaces = std::get<SubDelegateType>(castedDelegate->updators);
    grid_pd = convertImpl<SubDelegateType>(indexedSurfaces);
    grid_pd.grid_link.type = static_cast<detray::io::accel_id>(
        Acts::DetrayJsonHelper::accelerationLink(indexedSurfaces.casts));
    grid_pd.grid_link.index = std::numeric_limits<std::size_t>::max();
    return grid_pd;
  }

  return std::nullopt;
}

template <typename... Args>
std::vector<detray::io::grid_payload<std::size_t, detray::io::accel_id>>
Acts::DetraySurfaceGridsConverter::unrollConvert(
    const Acts::Experimental::InternalNavigationDelegate& delegate,
    Acts::TypeList<Args...> /*unused*/) {
  std::vector<detray::io::grid_payload<std::size_t, detray::io::accel_id>>
      grid_pds;

  const auto convertAndPush = [&grid_pds](const auto& adele,
                                          const auto& args) -> void {
    auto grid_pd = convert(adele, args);
    if (grid_pd.has_value()) {
      grid_pds.push_back(*grid_pd);
    }
  };

  (convertAndPush(delegate, Args{}), ...);

  return grid_pds;
}

detray::io::detector_grids_payload<std::size_t, detray::io::accel_id>
Acts::DetraySurfaceGridsConverter::convertSurfaceGrids(
    const Acts::Experimental::Detector& detector) {
  detray::io::detector_grids_payload<std::size_t, detray::io::accel_id>
      grids_pd = detray::io::detector_grids_payload<std::size_t,
                                                    detray::io::accel_id>();
  auto volumes = detector.volumes();

  for (const auto [iv, volume] : Acts::enumerate(volumes)) {
    std::vector<detray::io::grid_payload<std::size_t, detray::io::accel_id>>
        grid_pd = unrollConvert(volume->internalNavigation(),
                                Acts::GridAxisGenerators::PossibleAxes{});

    for (auto& grid : grid_pd) {
      detray::io::single_link_payload lnk;
      lnk.link = iv;
      grid.owner_link = lnk;
      grids_pd.grids[iv].push_back(grid);
    }
  }
  return grids_pd;
}

}  // namespace Acts
