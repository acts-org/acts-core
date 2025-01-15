// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/detail/IndexedSurfacesGenerator.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Plugins/ActSVG/GridSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SurfaceSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"
#include "Acts/Utilities/TypeList.hpp"
#include <actsvg/core.hpp>
#include <actsvg/meta.hpp>

#include <tuple>
#include <vector>

namespace Acts::Svg {

using ProtoSurface = actsvg::proto::surface<std::vector<Vector3>>;
using ProtoGrid = actsvg::proto::grid;
using ProtoIndexedSurfaceGrid =
    std::tuple<std::vector<ProtoSurface>, ProtoGrid,
               std::vector<std::vector<std::size_t>>>;

namespace IndexedSurfacesConverter {
/// Nested options struct
struct Options {
  /// Hierarchy map of styles
  GeometryHierarchyMap<Style> surfaceStyles;
  /// The Grid converter options
  GridConverter::Options gridOptions;
};

/// Convert a surface array into needed constituents
///
/// @note actual conversion implementation, bottom of unrolling loop
///
/// @param gtcx is the geometry context of the conversion call
/// @param surfaces the container of surfaces
/// @param indexGrid the indexGrid delegate
/// @param cOptions the conversion options
///
/// @return a collection of proto surface object and a grid, and associations
template <typename surface_container, typename index_grid>
ProtoIndexedSurfaceGrid convertImpl(const GeometryContext& gctx,
                                    const surface_container& surfaces,
                                    const index_grid& indexGrid,
                                    const Options& cOptions) {
  // The return surfaces
  std::vector<ProtoSurface> pSurfaces;
  pSurfaces.reserve(surfaces.size());

  // Make a local copy of the grid Options, in case we have to estimate
  // an additional bound
  GridConverter::Options gridOptions = cOptions.gridOptions;
  Extent constrain;

  // Estimate the radial extension
  // - for 1D phi
  // - for 2D z-phi or phi-z
  bool estimateR = (index_grid::grid_type::DIM == 1 &&
                    indexGrid.casts[0u] == AxisDirection::AxisPhi) ||
                   (index_grid::grid_type::DIM == 2 &&
                    (indexGrid.casts[0u] == AxisDirection::AxisPhi ||
                     indexGrid.casts[1u] == AxisDirection::AxisPhi));

  for (auto [is, s] : enumerate(surfaces)) {
    // Create the surface converter options
    SurfaceConverter::Options sOptions;
    // Try to see if you have predefined surface styles
    auto sfIter = cOptions.surfaceStyles.find(s->geometryId());
    if (sfIter != cOptions.surfaceStyles.end()) {
      sOptions.style = *sfIter;
    }
    auto pSurface = SurfaceConverter::convert(gctx, *s, sOptions);
    // Run the r estimation
    if (estimateR) {
      auto sExtent = s->polyhedronRepresentation(gctx, 4u).extent();
      if constexpr (index_grid::grid_type::DIM == 2u) {
        pSurface._radii[0u] =
            static_cast<float>(sExtent.medium(AxisDirection::AxisR));
      }
      constrain.extend(sExtent, {AxisDirection::AxisR});
    }
    // Add center info
    std::string centerInfo = " - center = (";
    const Vector3& center = s->center(gctx);
    centerInfo +=
        std::to_string(VectorHelpers::cast(center, indexGrid.casts[0u]));
    if (indexGrid.casts.size() > 1u) {
      centerInfo += ", ";
      centerInfo +=
          std::to_string(VectorHelpers::cast(center, indexGrid.casts[1u]));
      centerInfo += ")";
    }
    pSurface._aux_info["center"] = {centerInfo};
    // Add the center info
    pSurfaces.push_back(pSurface);
  }

  // Adjust the grid options
  if constexpr (index_grid::grid_type::DIM == 1u) {
    if (indexGrid.casts[0u] == AxisDirection::AxisPhi) {
      auto estRangeR = constrain.range(AxisDirection::AxisR);
      std::array<double, 2u> rRange = {estRangeR.min(), estRangeR.max()};
      gridOptions.optionalBound = {rRange, AxisDirection::AxisR};
    }
  }

  // Create the grid
  ProtoGrid pGrid =
      GridConverter::convert(indexGrid.grid, indexGrid.casts, gridOptions);

  auto axes = indexGrid.grid.axes();

  // Specify the highlight indices
  std::vector<std::vector<std::size_t>> highlightIndices;

  // 1D connections
  if constexpr (index_grid::grid_type::DIM == 1u) {
    const auto& binEdges = axes[0u]->getBinEdges();
    for (unsigned int ib0 = 1u; ib0 <= axes[0u]->getNBins(); ++ib0) {
      typename index_grid::grid_type::index_t lbin;
      lbin[0u] = ib0;
      highlightIndices.push_back(indexGrid.grid.atLocalBins(lbin));
      // Register the bin naming
      std::string binInfo =
          std::string("- bin : [") + std::to_string(ib0) + std::string("]");
      double binCenter = 0.5 * (binEdges[ib0] + binEdges[ib0 - 1u]);
      binInfo += "\n - center : (" + std::to_string(binCenter) + ")";
      pGrid._bin_ids.push_back(binInfo);
    }
  }
  // 2D connections
  if constexpr (index_grid::grid_type::DIM == 2u) {
    const auto& binEdges0 = axes[0u]->getBinEdges();
    const auto& binEdges1 = axes[1u]->getBinEdges();
    for (unsigned int ib0 = 1u; ib0 <= axes[0u]->getNBins(); ++ib0) {
      for (unsigned int ib1 = 1u; ib1 <= axes[1u]->getNBins(); ++ib1) {
        typename index_grid::grid_type::index_t lbin;
        lbin[0u] = ib0;
        lbin[1u] = ib1;
        highlightIndices.push_back(indexGrid.grid.atLocalBins(lbin));
        // Register the bin naming
        std::string binInfo = std::string("- bin : [") + std::to_string(ib0) +
                              std::string(", ") + std::to_string(ib1) +
                              std::string("]");
        double binCenter0 = 0.5 * (binEdges0[ib0] + binEdges0[ib0 - 1u]);
        double binCenter1 = 0.5 * (binEdges1[ib1] + binEdges1[ib1 - 1u]);
        binInfo += "\n - center : (" + std::to_string(binCenter0) + ", " +
                   std::to_string(binCenter1) + ")";
        pGrid._bin_ids.push_back(binInfo);
        if (estimateR) {
          pGrid._reference_r =
              static_cast<float>(constrain.medium(AxisDirection::AxisR));
        }
      }
    }
  }
  return {pSurfaces, pGrid, highlightIndices};
}

/// @brief Convert the single delegate if it is of the type of the reference
///
/// @note It will do nothing if the type does not match
///
/// @tparam surface_container the surfaces to be drawn
/// @tparam instance_type the reference instance type
///
/// @param gctx The Geometry context of this operation
/// @param surfaces The surfaces to be converted
/// @param cOptions the conversion options
/// @param sgi [in,out] the proto indexed grid to be converted
/// @param delegate the delegate to be translated
/// @param refInstance the reference input type from the reference Axes
template <typename surface_container, typename instance_type>
void convert(const GeometryContext& gctx, const surface_container& surfaces,
             const Options& cOptions, ProtoIndexedSurfaceGrid& sgi,
             const Experimental::InternalNavigationDelegate& delegate,
             [[maybe_unused]] const instance_type& refInstance) {
  using GridType =
      typename instance_type::template grid_type<std::vector<std::size_t>>;
  // Defining a Delegate type
  using DelegateType = Experimental::IndexedSurfacesAllPortalsNavigation<
      GridType, Experimental::IndexedSurfacesNavigation>;
  using SubDelegateType = Experimental::IndexedSurfacesNavigation<GridType>;

  // Get the instance
  const auto* instance = delegate.instance();
  auto castedDelegate = dynamic_cast<const DelegateType*>(instance);
  if (castedDelegate != nullptr) {
    // Get the surface updator
    auto indexedSurfaces = std::get<SubDelegateType>(castedDelegate->updators);
    auto [pSurfaces, pGrid, pIndices] =
        convertImpl(gctx, surfaces, indexedSurfaces, cOptions);
    std::get<0u>(sgi) = pSurfaces;
    std::get<1u>(sgi) = pGrid;
    std::get<2u>(sgi) = pIndices;
  }
}

/// @brief Unrolling function for catching the right instance
///
/// @note parameters are as of the `convertImpl` method
template <typename surface_container, typename... Args>
void unrollConvert(const GeometryContext& gctx,
                   const surface_container& surfaces, const Options& cOptions,
                   ProtoIndexedSurfaceGrid& sgi,
                   const Experimental::InternalNavigationDelegate& delegate,
                   TypeList<Args...> /*unused*/) {
  (convert(gctx, surfaces, cOptions, sgi, delegate, Args{}), ...);
}

/// Convert a surface array into needed constituents
///
/// @param gtcx is the geometry context of the conversion call
/// @param surfaces the container of surfaces
/// @param indexGrid the indexGrid delegate
/// @param cOptions the conversion options
///
/// @note this is the entry point of the conversion, i.e. top of the
/// unrolling loop
///
/// @return a collection of proto surface object and a grid, and associations
template <typename surface_container>
ProtoIndexedSurfaceGrid convert(
    const GeometryContext& gctx, const surface_container& surfaces,
    const Experimental::InternalNavigationDelegate& delegate,
    const Options& cOptions) {
  // Prep work what is to be filled
  std::vector<ProtoSurface> pSurfaces;
  ProtoGrid pGrid;
  std::vector<std::vector<std::size_t>> indices;
  ProtoIndexedSurfaceGrid sgi = {pSurfaces, pGrid, indices};
  // Convert if dynamic cast happens to work
  unrollConvert(gctx, surfaces, cOptions, sgi, delegate,
                GridAxisGenerators::PossibleAxes{});
  // Return the newly filled ones
  return sgi;
}

}  // namespace IndexedSurfacesConverter

namespace View {

/// Convert into an acts::svg::object with an XY view
///
/// @param pIndexGrid is the proto object
/// @param identification is the to be translated id_ for actsvg
///
/// @return an svg object that can be written out directly
static inline actsvg::svg::object xy(const ProtoIndexedSurfaceGrid& pIndexGrid,
                                     const std::string& identification) {
  auto [pSurfaces, pGrid, pIndices] = pIndexGrid;

  // Create the master object
  actsvg::svg::object xyIndexedGrid;
  xyIndexedGrid._id = identification;
  xyIndexedGrid._tag = "g";

  // The surfaces
  std::vector<actsvg::svg::object> sObs;
  for (const auto& s : pSurfaces) {
    if (pGrid._type == actsvg::proto::grid::e_z_phi) {
      sObs.push_back(Acts::Svg::View::zrphi(s, s._name));
    } else {
      sObs.push_back(Acts::Svg::View::xy(s, s._name));
    }
  }

  // The grid as provided
  auto gOb =
      actsvg::display::grid(identification + std::string("_grid"), pGrid);

  // The connectors
  actsvg::connectors::connect_action(gOb._sub_objects, sObs, pIndices);

  // Add them all
  xyIndexedGrid.add_objects(sObs);

  auto xmax = xyIndexedGrid._x_range[1u];
  // The association info boxes
  for (auto [ig, gTile] : enumerate(gOb._sub_objects)) {
    // Target surface text
    std::vector<std::string> binText;
    binText.push_back("Source:");
    binText.push_back(pGrid._bin_ids[ig]);
    binText.push_back("Target:");
    for (const auto [is, sis] : enumerate(pIndices[ig])) {
      const auto& ps = pSurfaces[sis];
      std::string oInfo = std::string("- object: ") + std::to_string(sis);
      if (ps._aux_info.contains("center")) {
        for (const auto& ci : ps._aux_info.at("center")) {
          oInfo += ci;
        }
      }
      binText.push_back(oInfo);
    }
    // Make the connected text
    std::string cTextId =
        identification + std::string("_ct_") + std::to_string(ig);
    auto cText = actsvg::draw::connected_text(
        cTextId, {static_cast<actsvg::scalar>(1.1 * xmax), 0}, binText,
        actsvg::style::font{}, actsvg::style::transform{}, gTile);
    xyIndexedGrid.add_object(cText);
  }
  xyIndexedGrid.add_object(gOb);
  // Return the grid
  return xyIndexedGrid;
}

/// Fake zphi method, delegate to xy
///
/// @param pIndexGrid is the proto object
/// @param identification is the to be translated id_ for actsvg
///
/// @note this works, because the actual display at the end is 2D
/// in an x-y plane and the parameters are appropriately defined
///
/// @return an svg object that can be written out directly
static inline actsvg::svg::object zphi(
    const ProtoIndexedSurfaceGrid& pIndexGrid,
    const std::string& identification) {
  return xy(pIndexGrid, identification);
}

}  // namespace View
}  // namespace Acts::Svg
