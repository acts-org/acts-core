// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Plugins/Json/TrackParametersJsonConverter.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/GridAccessHelpers.hpp"
#include "Acts/Utilities/IAxis.hpp"

#include <iostream>

// Custom Json encoder/decoders.
namespace Acts {

/// @cond
NLOHMANN_JSON_SERIALIZE_ENUM(Acts::AxisBoundaryType,
                             {{Acts::AxisBoundaryType::Bound, "Bound"},
                              {Acts::AxisBoundaryType::Open, "Open"},
                              {Acts::AxisBoundaryType::Closed, "Closed"}})

NLOHMANN_JSON_SERIALIZE_ENUM(Acts::AxisType,
                             {{Acts::AxisType::Equidistant, "Equidistant"},
                              {Acts::AxisType::Variable, "Variable"}})

/// @endcond

namespace AxisJsonConverter {

/// Convert an axis to json
//
/// @param ia the axis
///
/// @return a json object to represent the axis
nlohmann::json toJson(const IAxis& ia);

/// Convert an axis to json - detray style
///
/// @param ia the axis
///
/// @return a json object to represent the axis in detray json
nlohmann::json toJsonDetray(const IAxis& ia);

}  // namespace AxisJsonConverter

namespace GridAccessJsonConverter {

/// Convert a global to local access to json
///
/// @param globalToGridLocal the global to grid local access
///
/// @return a json object to represent global class
nlohmann::json toJson(const GridAccess::IGlobalToGridLocal& globalToGridLocal);

/// Create a global grid to local instance
///
/// @param jGlobalToGridLocal the json snippet
///
/// @return a newly created object
std::unique_ptr<const GridAccess::IGlobalToGridLocal> globalToGridLocalFromJson(
    const nlohmann::json& jGlobalToGridLocal);

/// Create the delegate directly
///
/// @param jGlobalToGridLocal the json snippet
///
/// This is the usual workflow, as the connect method can be called on
/// the concreate type
///
/// @note the dimension of the delegate has to be known by peeking
/// into the json object
GridAccess::GlobalToGridLocal1DimDelegate globalToGridLocal1DimDelegateFromJson(
    const nlohmann::json& jGlobalToGridLocal);

/// Create the delegate directly
///
/// @param jGlobalToGridLocal the json snippet
///
/// This is the usual workflow, as the connect method can be called on
/// the concreate type
///
/// @note the dimension of the delegate has to be known by peeking
/// into the json object
GridAccess::GlobalToGridLocal2DimDelegate globalToGridLocal2DimDelegateFromJson(
    const nlohmann::json& jGlobalToGridLocal);

/// Convert a local to local access to json
///
/// @param boundToGridLocal the local to local access
///
/// @return a json object to represent local class
nlohmann::json toJson(const GridAccess::IBoundToGridLocal& boundToGridLocal);

/// Create a local grid to local instance
///
/// @param jBoundToGridLocal the json snippet
///
/// @return a newly created object
std::unique_ptr<GridAccess::IBoundToGridLocal> boundToGridLocalFromJson(
    const nlohmann::json& jBoundToGridLocal);

/// Create the delegate directly
///
/// @param jBoundToGridLocal the json snippe
///
/// This is the usual workflow, as the connect method can be called on
/// the concreate type
///
/// @note the dimension of the delegate has to be known by peeking
/// into the json object
GridAccess::BoundToGridLocal1DimDelegate boundToGridLocal1DimDelegateFromJson(
    const nlohmann::json& jBoundToGridLocal);

/// Create the delegate directly
///
/// @param jBoundToGridLocal the json snippe
///
/// This is the usual workflow, as the connect method can be called on
/// the concreate type
///
/// @note the dimension of the delegate has to be known by peeking
/// into the json object
GridAccess::BoundToGridLocal2DimDelegate boundToGridLocal2DimDelegateFromJson(
    const nlohmann::json& jBoundToGridLocal);

}  // namespace GridAccessJsonConverter

namespace GridJsonConverter {

/// @brief Templated grid conversion to json
///
/// @tparam grid_type the type of the grid
/// @param grid the grid object
///
/// @return a json object to represent the grid
template <typename grid_type>
nlohmann::json toJson(const grid_type& grid) {
  nlohmann::json jGrid;

  auto axes = grid.axes();
  nlohmann::json jAxes;
  for (unsigned int ia = 0u; ia < grid_type::DIM; ++ia) {
    auto jAxis = AxisJsonConverter::toJson(*axes[ia]);
    jAxes.push_back(jAxis);
  }
  jGrid["axes"] = jAxes;

  nlohmann::json jData;
  // 1D connections
  if constexpr (grid_type::DIM == 1u) {
    for (unsigned int ib0 = 1u; ib0 <= axes[0u]->getNBins(); ++ib0) {
      typename grid_type::index_t lbin;
      lbin[0u] = ib0;
      jData.push_back(std::tie(lbin, grid.atLocalBins(lbin)));
    }
  }
  // 2D connections
  if constexpr (grid_type::DIM == 2u) {
    for (unsigned int ib0 = 1u; ib0 <= axes[0u]->getNBins(); ++ib0) {
      for (unsigned int ib1 = 1u; ib1 <= axes[1u]->getNBins(); ++ib1) {
        typename grid_type::index_t lbin;
        lbin[0u] = ib0;
        lbin[1u] = ib1;
        jData.push_back(std::tie(lbin, grid.atLocalBins(lbin)));
      }
    }
  }
  jGrid["data"] = jData;

  return jGrid;
}

/// @brief Templated grid conversion to json
///
/// @tparam grid_type the type of the grid
/// @param grid the grid object
/// @param swapAxis - this is needed for detray
///
/// @note detray has a different offset for the
/// local indices, it starts at 0
///
/// @return a json object to represent the grid
template <typename grid_type>
nlohmann::json toJsonDetray(const grid_type& grid, bool swapAxis = false) {
  nlohmann::json jGrid;
  // Get the grid axes & potentially swap them
  auto axes = grid.axes();
  if (swapAxis && grid_type::DIM == 2u) {
    std::swap(axes[0u], axes[1u]);
  }

  nlohmann::json jAxes;

  // Fill the axes in the order they are
  for (unsigned int ia = 0u; ia < grid_type::DIM; ++ia) {
    auto jAxis = AxisJsonConverter::toJsonDetray(*axes[ia]);
    jAxis["label"] = ia;
    jAxes.push_back(jAxis);
  }
  jGrid["axes"] = jAxes;

  nlohmann::json jData;
  // 1D connections
  if constexpr (grid_type::DIM == 1u) {
    for (unsigned int ib0 = 1u; ib0 <= axes[0u]->getNBins(); ++ib0) {
      // Lookup bin
      typename grid_type::index_t lbin;
      lbin[0u] = ib0;
      nlohmann::json jBin;
      jBin["content"] = grid.atLocalBins(lbin);
      // Corrected bin for detray
      lbin[0u] = ib0 - 1u;
      jBin["loc_index"] = lbin;
      jData.push_back(jBin);
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
        nlohmann::json jBin;
        jBin["content"] = grid.atLocalBins(lbin);
        // Corrected bin for detray
        lbin[0u] = ib0 - 1u;
        lbin[1u] = ib1 - 1u;
        jBin["loc_index"] = lbin;
        jData.push_back(jBin);
      }
    }
  }
  jGrid["bins"] = jData;

  return jGrid;
}

/// @brief Templated grid conversion from json
///
/// @tparam axis_generator_type the type of the axis generator
///         which determines the grid type
///
/// @param jGrid the json object to represent the grid
/// @param aGenerator the axis generator
///
/// @note the axis generator also defines the grid dimension
///
/// @return a grid object
template <typename axis_generator_type,
          typename value_type = std::vector<std::size_t>>
auto fromJson(const nlohmann::json& jGrid,
              const axis_generator_type& aGenerator) {
  // Generate the grid
  using GridType = typename axis_generator_type::template grid_type<value_type>;
  GridType grid(aGenerator());
  nlohmann::json jData = jGrid["data"];
  // Index filling
  if constexpr (GridType::DIM == 1u) {
    for (const auto& jd : jData) {
      std::array<std::size_t, 1u> lbin = jd[0u];
      if (!jd[1u].is_null()) {
        grid.atLocalBins(lbin) = jd[1u].get<value_type>();
      }
    }
  }
  if constexpr (GridType::DIM == 2u) {
    for (const auto& jd : jData) {
      std::array<std::size_t, 2u> lbin = jd[0u];
      if (!jd[1u].is_null()) {
        grid.atLocalBins(lbin) = jd[1u].get<value_type>();
      }
    }
  }
  return grid;
}

}  // namespace GridJsonConverter
}  // namespace Acts
