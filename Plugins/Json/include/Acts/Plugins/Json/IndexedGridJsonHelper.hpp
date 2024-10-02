// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Json/AlgebraJsonConverter.hpp"
#include "Acts/Plugins/Json/GridJsonConverter.hpp"
#include "Acts/Plugins/Json/UtilitiesJsonConverter.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"

#include <tuple>

namespace Acts {
using namespace GridAxisGenerators;

namespace IndexedGridJsonHelper {

/// @brief The actual conversion method
///
/// @param indexGrid is the index grid to be written
/// @param detray is a flag indicating detray writout
/// @param checkSwap is a flag indicating if the axes should be swapped
template <typename index_grid>
nlohmann::json convertImpl(const index_grid& indexGrid, bool detray = false,
                           bool checkSwap = false) {
  nlohmann::json jIndexedGrid;

  // Axis swapping (detray version)
  bool swapAxes = checkSwap;

  // Fill the casts
  nlohmann::json jCasts;
  // 1D casts
  if constexpr (index_grid::grid_type::DIM == 1u) {
    jCasts.push_back(indexGrid.casts[0u]);
  }
  // 1D casts
  if constexpr (index_grid::grid_type::DIM == 2u) {
    jCasts.push_back(indexGrid.casts[0u]);
    jCasts.push_back(indexGrid.casts[1u]);
    // Check for axis swap (detray version)
    swapAxes = checkSwap && (indexGrid.casts[0u] == BinningValue::binZ &&
                             indexGrid.casts[1u] == BinningValue::binPhi);
  }
  jIndexedGrid["casts"] = jCasts;
  jIndexedGrid["transform"] =
      Transform3JsonConverter::toJson(indexGrid.transform);
  if (detray) {
    jIndexedGrid = GridJsonConverter::toJsonDetray(indexGrid.grid, swapAxes);
  } else {
    jIndexedGrid["grid"] = GridJsonConverter::toJson(indexGrid.grid);
  }
  return jIndexedGrid;
}

/// @brief Helper method to be used for the creation of surface updators
/// and volume updates
///
/// @tparam updator_type
/// @tparam generator_type
///
/// @param jUpdater The corresponding json object
/// @param jIndicator the string indicator which one it is
///
/// @return the updator type
template <typename updator_type, typename generator_type>
updator_type generateFromJson(const nlohmann::json& jUpdater,
                              const std::string& jIndicator) {
  generator_type generator;

  using ValueType = typename generator_type::value_type;

  /// Helper extractor for equidistant axis
  /// @param jAxis is the axis
  auto eqExtractor = [](const nlohmann::json& jAxis)
      -> std::tuple<std::array<ActsScalar, 2u>, std::size_t> {
    std::array<ActsScalar, 2u> range = jAxis["range"];
    std::size_t bins = jAxis["bins"];
    return std::make_tuple(range, bins);
  };

  /// Helper extractor for variable axis
  /// @param jAxis the axis
  auto vExtractor = [](const nlohmann::json& jAxis) -> std::vector<ActsScalar> {
    std::vector<ActsScalar> vEx(jAxis["boundaries"]);
    return vEx;
  };

  // Peek into the json object to understand what to do
  if (jUpdater["type"] == jIndicator) {
    if (jUpdater.find("grid") != jUpdater.end()) {
      Transform3 transform =
          Transform3JsonConverter::fromJson(jUpdater["transform"]);
      auto jGrid = jUpdater["grid"];
      auto jCasts = jUpdater["casts"].get<std::vector<BinningValue>>();
      auto jAxes = jGrid["axes"];

      // 1D cases
      if (jAxes.size() == 1u) {
        BinningValue bValue = jCasts[0u];
        auto jAxis = jAxes[0u];

        AxisType axisType = jAxis["type"];
        AxisBoundaryType axisBoundaryType = jAxis["boundary_type"];

        // Equidistant axis
        if (axisType == AxisType::Equidistant) {
          auto [range, bins] = eqExtractor(jAxis);
          if (axisBoundaryType == AxisBoundaryType::Closed) {
            EqClosed ecAG{range, bins};
            auto grid =
                GridJsonConverter::fromJson<EqClosed, ValueType>(jGrid, ecAG);
            return generator.createUpdater(std::move(grid), {bValue},
                                           transform);
          } else {
            EqBound ebAG{range, bins};
            auto grid =
                GridJsonConverter::fromJson<EqBound, ValueType>(jGrid, ebAG);
            return generator.createUpdater(std::move(grid), {bValue},
                                           transform);
          }
        } else {
          // Variable type
          if (axisBoundaryType == AxisBoundaryType::Closed) {
            VarClosed vcAG{vExtractor(jAxis)};
            auto grid =
                GridJsonConverter::fromJson<VarClosed, ValueType>(jGrid, vcAG);
            return generator.createUpdater(std::move(grid), {bValue},
                                           transform);
          } else {
            VarBound vbAG{vExtractor(jAxis)};
            auto grid =
                GridJsonConverter::fromJson<VarBound, ValueType>(jGrid, vbAG);
            return generator.createUpdater(std::move(grid), {bValue},
                                           transform);
          }
        }
      } else if (jAxes.size() == 2u) {
        // This currently writes out only the main options of 2D grids
        // nota bene: it assumes if one axis is closed, it is axis B

        BinningValue bValueA = jCasts[0u];
        BinningValue bValueB = jCasts[1u];
        auto jAxisA = jAxes[0u];
        auto jAxisB = jAxes[1u];

        AxisType axisTypeA = jAxisA["type"];
        AxisType axisTypeB = jAxisB["type"];
        AxisBoundaryType axisBoundaryTypeB = jAxisB["boundary_type"];

        if (axisBoundaryTypeB != AxisBoundaryType::Closed) {
          // First axis equidistant
          if (axisTypeA == AxisType::Equidistant) {
            auto [rangeA, binsA] = eqExtractor(jAxisA);
            if (axisTypeB == AxisType::Equidistant) {
              auto [rangeB, binsB] = eqExtractor(jAxisB);
              EqBoundEqBound ebebAG{rangeA, binsA, rangeB, binsB};
              auto grid =
                  GridJsonConverter::fromJson<EqBoundEqBound, ValueType>(
                      jGrid, ebebAG);
              return generator.createUpdater(std::move(grid),
                                             {bValueA, bValueB}, transform);
            } else {
              EqBoundVarBound ebvbAG{rangeA, binsA, vExtractor(jAxisB)};
              auto grid =
                  GridJsonConverter::fromJson<EqBoundVarBound, ValueType>(
                      jGrid, ebvbAG);
              return generator.createUpdater(std::move(grid),
                                             {bValueA, bValueB}, transform);
            }
          } else {
            if (axisTypeB == AxisType::Equidistant) {
              auto [rangeB, binsB] = eqExtractor(jAxisB);
              VarBoundEqBound vbebAG{vExtractor(jAxisA), rangeB, binsB};
              auto grid =
                  GridJsonConverter::fromJson<VarBoundEqBound, ValueType>(
                      jGrid, vbebAG);
              return generator.createUpdater(std::move(grid),
                                             {bValueA, bValueB}, transform);
            } else {
              VarBoundVarBound vbvbAG{vExtractor(jAxisA), vExtractor(jAxisB)};
              auto grid =
                  GridJsonConverter::fromJson<VarBoundVarBound, ValueType>(
                      jGrid, vbvbAG);
              return generator.createUpdater(std::move(grid),
                                             {bValueA, bValueB}, transform);
            }
          }
        } else {
          // Closed cases
          if (axisTypeA == AxisType::Equidistant) {
            auto [rangeA, binsA] = eqExtractor(jAxisA);
            if (axisTypeB == AxisType::Equidistant) {
              auto [rangeB, binsB] = eqExtractor(jAxisB);
              EqBoundEqClosed ebecAG{rangeA, binsA, rangeB, binsB};
              auto grid =
                  GridJsonConverter::fromJson<EqBoundEqClosed, ValueType>(
                      jGrid, ebecAG);
              return generator.createUpdater(std::move(grid),
                                             {bValueA, bValueB}, transform);
            } else {
              EqBoundVarClosed ebvcAG{rangeA, binsA, vExtractor(jAxisB)};
              auto grid =
                  GridJsonConverter::fromJson<EqBoundVarClosed, ValueType>(
                      jGrid, ebvcAG);
              return generator.createUpdater(std::move(grid),
                                             {bValueA, bValueB}, transform);
            }
          } else {
            if (axisTypeB == AxisType::Equidistant) {
              auto [rangeB, binsB] = eqExtractor(jAxisB);
              VarBoundEqClosed vbecAG{vExtractor(jAxisA), rangeB, binsB};
              auto grid =
                  GridJsonConverter::fromJson<VarBoundEqClosed, ValueType>(
                      jGrid, vbecAG);
              return generator.createUpdater(std::move(grid),
                                             {bValueA, bValueB}, transform);
            } else {
              VarBoundVarClosed vbvcAG{vExtractor(jAxisA), vExtractor(jAxisB)};
              auto grid =
                  GridJsonConverter::fromJson<VarBoundVarClosed, ValueType>(
                      jGrid, vbvcAG);
              return generator.createUpdater(std::move(grid),
                                             {bValueA, bValueB}, transform);
            }
          }
        }
      }
    }
  }
  // The return object
  updator_type updator;
  return updator;
}

}  // namespace IndexedGridJsonHelper
}  // namespace Acts
