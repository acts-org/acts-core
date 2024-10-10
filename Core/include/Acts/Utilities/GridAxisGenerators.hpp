// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/TypeList.hpp"

#include <array>
#include <tuple>
#include <vector>

/// Axis generators are used to allow defining different grid types
/// for indexed geometry objects.
///
/// The call operator() API allows to plug axis generators into
/// dedicated code snippets and create fitting axis types on the fly
/// which then turn into concrete Grid types.
namespace Acts::GridAxisGenerators {

/// @brief  Templated base generator for equidistant axis as a tuple - 1D
///
/// @tparam aType the type of the axis (Bound, Closed, Open)
template <Acts::AxisBoundaryType aType>
struct Eq {
  /// Broadcast the return_type
  using return_type =
      std::tuple<Acts::Axis<Acts::AxisType::Equidistant, aType>>;

  /// Broadcast the grid type
  template <typename T>
  using grid_type =
      Acts::Grid<T, Acts::Axis<Acts::AxisType::Equidistant, aType>>;

  std::array<ActsScalar, 2u> range = {};
  std::size_t nBins = 0u;

  /// Call operator that generates the Axis
  return_type operator()() const {
    Acts::Axis<Acts::AxisType::Equidistant, aType> eAxis(range[0u], range[1u],
                                                         nBins);
    return {eAxis};
  }
};

// All 1D equidistant options
using EqBound = Eq<Acts::AxisBoundaryType::Bound>;
using EqOpen = Eq<Acts::AxisBoundaryType::Open>;
using EqClosed = Eq<Acts::AxisBoundaryType::Closed>;

/// @brief  Templated base generator for variable axis as a tuple - 1D
///
/// @tparam aType the type of the axis (Bound, Closed, Open)
template <Acts::AxisBoundaryType aType>
struct Var {
  /// Broadcast the return_type
  using return_type = std::tuple<Acts::Axis<Acts::AxisType::Variable, aType>>;

  /// Broadcast the grid type
  template <typename T>
  using grid_type = Acts::Grid<T, Acts::Axis<Acts::AxisType::Variable, aType>>;

  std::vector<ActsScalar> edges = {};

  /// Call operator that generates the Axis
  return_type operator()() const {
    Acts::Axis<Acts::AxisType::Variable, aType> vAxis(edges);
    return {vAxis};
  }
};

// All 1D variable options
using VarBound = Var<Acts::AxisBoundaryType::Bound>;
using VarOpen = Var<Acts::AxisBoundaryType::Open>;
using VarClosed = Var<Acts::AxisBoundaryType::Closed>;

/// @brief  Templated base generator for two equidistant axes as a tuple - 2D
///
/// @tparam aType the type of the first axis (Bound, Closed, Open)
/// @tparam bType the type of the second axis (Bound, Closed, Open)
///
template <Acts::AxisBoundaryType aType, Acts::AxisBoundaryType bType>
struct EqEq {
  /// Broadcast the return_type
  using return_type =
      std::tuple<Acts::Axis<Acts::AxisType::Equidistant, aType>,
                 Acts::Axis<Acts::AxisType::Equidistant, bType>>;

  /// Broadcast the grid type
  template <typename T>
  using grid_type =
      Acts::Grid<T, Acts::Axis<Acts::AxisType::Equidistant, aType>,
                 Acts::Axis<Acts::AxisType::Equidistant, bType>>;

  std::array<ActsScalar, 2u> range0 = {};
  std::size_t nBins0 = 0u;
  std::array<ActsScalar, 2u> range1 = {};
  std::size_t nBins1 = 1u;

  /// Call operator that generates the Axis
  return_type operator()() const {
    // Create the two axis
    Acts::Axis<Acts::AxisType::Equidistant, aType> aEq(range0[0u], range0[1u],
                                                       nBins0);
    Acts::Axis<Acts::AxisType::Equidistant, bType> bEq(range1[0u], range1[1u],
                                                       nBins1);
    return std::tie(aEq, bEq);
  }
};

// All 2D EqEq options
using EqBoundEqBound =
    EqEq<Acts::AxisBoundaryType::Bound, Acts::AxisBoundaryType::Bound>;
using EqBoundEqOpen =
    EqEq<Acts::AxisBoundaryType::Bound, Acts::AxisBoundaryType::Open>;
using EqBoundEqClosed =
    EqEq<Acts::AxisBoundaryType::Bound, Acts::AxisBoundaryType::Closed>;
using EqOpenEqBound =
    EqEq<Acts::AxisBoundaryType::Open, Acts::AxisBoundaryType::Bound>;
using EqOpenEqOpen =
    EqEq<Acts::AxisBoundaryType::Open, Acts::AxisBoundaryType::Open>;
using EqOpenEqClosed =
    EqEq<Acts::AxisBoundaryType::Open, Acts::AxisBoundaryType::Closed>;
using EqClosedEqBound =
    EqEq<Acts::AxisBoundaryType::Closed, Acts::AxisBoundaryType::Bound>;
using EqClosedEqOpen =
    EqEq<Acts::AxisBoundaryType::Closed, Acts::AxisBoundaryType::Open>;
using EqClosedEqClosed =
    EqEq<Acts::AxisBoundaryType::Closed, Acts::AxisBoundaryType::Closed>;

/// @brief  Templated base generator for equidistant / variable axes as a tuple - 2D
///
/// @tparam aType the type of the first axis (Bound, Closed, Open)
/// @tparam bType the type of the second axis (Bound, Closed, Open)
///
template <Acts::AxisBoundaryType aType, Acts::AxisBoundaryType bType>
struct EqVar {
  /// Broadcast the return_type
  using return_type = std::tuple<Acts::Axis<Acts::AxisType::Equidistant, aType>,
                                 Acts::Axis<Acts::AxisType::Variable, bType>>;

  /// Broadcast the grid type
  template <typename T>
  using grid_type =
      Acts::Grid<T, Acts::Axis<Acts::AxisType::Equidistant, aType>,
                 Acts::Axis<Acts::AxisType::Variable, bType>>;

  std::array<ActsScalar, 2u> range = {};
  std::size_t nBins = 0u;
  std::vector<ActsScalar> edges = {};

  /// Call operator that generates the Axis
  return_type operator()() const {
    Acts::Axis<Acts::AxisType::Equidistant, aType> eqA(range[0u], range[1u],
                                                       nBins);
    Acts::Axis<Acts::AxisType::Variable, bType> varB(edges);
    return std::tie(eqA, varB);
  }
};

// All 2D EqVar options
using EqBoundVarBound =
    EqVar<Acts::AxisBoundaryType::Bound, Acts::AxisBoundaryType::Bound>;
using EqBoundVarOpen =
    EqVar<Acts::AxisBoundaryType::Bound, Acts::AxisBoundaryType::Open>;
using EqBoundVarClosed =
    EqVar<Acts::AxisBoundaryType::Bound, Acts::AxisBoundaryType::Closed>;
using EqOpenVarBound =
    EqVar<Acts::AxisBoundaryType::Open, Acts::AxisBoundaryType::Bound>;
using EqOpenVarOpen =
    EqVar<Acts::AxisBoundaryType::Open, Acts::AxisBoundaryType::Open>;
using EqOpenVarClosed =
    EqVar<Acts::AxisBoundaryType::Open, Acts::AxisBoundaryType::Closed>;
using EqClosedVarBound =
    EqVar<Acts::AxisBoundaryType::Closed, Acts::AxisBoundaryType::Bound>;
using EqClosedVarOpen =
    EqVar<Acts::AxisBoundaryType::Closed, Acts::AxisBoundaryType::Open>;
using EqClosedVarClosed =
    EqVar<Acts::AxisBoundaryType::Closed, Acts::AxisBoundaryType::Closed>;

/// @brief  Templated base generator for a variable, equidistant axes tuple - 2D
///
/// @tparam aType the type of the first axis (Bound, Closed, Open)
/// @tparam bType the type of the second axis (Bound, Closed, Open)
///
template <Acts::AxisBoundaryType aType, Acts::AxisBoundaryType bType>
struct VarEq {
  /// Broadcast the return_type
  using return_type =
      std::tuple<Acts::Axis<Acts::AxisType::Variable, aType>,
                 Acts::Axis<Acts::AxisType::Equidistant, bType>>;

  /// Broadcast the grid type
  template <typename T>
  using grid_type = Acts::Grid<T, Acts::Axis<Acts::AxisType::Variable, aType>,
                               Acts::Axis<Acts::AxisType::Equidistant, bType>>;

  std::vector<ActsScalar> edges = {};
  std::array<ActsScalar, 2u> range = {};
  std::size_t nBins = 0u;

  /// Call operator that generates the Axis
  return_type operator()() const {
    Acts::Axis<Acts::AxisType::Variable, aType> varA(edges);
    Acts::Axis<Acts::AxisType::Equidistant, bType> eqB(range[0u], range[1u],
                                                       nBins);
    return std::tie(varA, eqB);
  }
};

// All 2D VarEq options
using VarBoundEqBound =
    VarEq<Acts::AxisBoundaryType::Bound, Acts::AxisBoundaryType::Bound>;
using VarBoundEqOpen =
    VarEq<Acts::AxisBoundaryType::Bound, Acts::AxisBoundaryType::Open>;
using VarBoundEqClosed =
    VarEq<Acts::AxisBoundaryType::Bound, Acts::AxisBoundaryType::Closed>;
using VarOpenEqBound =
    VarEq<Acts::AxisBoundaryType::Open, Acts::AxisBoundaryType::Bound>;
using VarOpenEqOpen =
    VarEq<Acts::AxisBoundaryType::Open, Acts::AxisBoundaryType::Open>;
using VarOpenEqClosed =
    VarEq<Acts::AxisBoundaryType::Open, Acts::AxisBoundaryType::Closed>;
using VarClosedEqBound =
    VarEq<Acts::AxisBoundaryType::Closed, Acts::AxisBoundaryType::Bound>;
using VarClosedEqOpen =
    VarEq<Acts::AxisBoundaryType::Closed, Acts::AxisBoundaryType::Open>;
using VarClosedEqClosed =
    VarEq<Acts::AxisBoundaryType::Closed, Acts::AxisBoundaryType::Closed>;

/// @brief  Templated base generator for a two variable axes tuple - 2D
///
/// @tparam aType the type of the first axis (Bound, Closed, Open)
/// @tparam bType the type of the second axis (Bound, Closed, Open)
///
template <Acts::AxisBoundaryType aType, Acts::AxisBoundaryType bType>
struct VarVar {
  /// Broadcast the return_type
  using return_type = std::tuple<Acts::Axis<Acts::AxisType::Variable, aType>,
                                 Acts::Axis<Acts::AxisType::Variable, bType>>;

  /// Broadcast the grid type
  template <typename T>
  using grid_type = Acts::Grid<T, Acts::Axis<Acts::AxisType::Variable, aType>,
                               Acts::Axis<Acts::AxisType::Variable, bType>>;

  std::vector<ActsScalar> edges0 = {};
  std::vector<ActsScalar> edges1 = {};

  /// Call operator that generates the Axis
  return_type operator()() const {
    Acts::Axis<Acts::AxisType::Variable, aType> varA(edges0);
    Acts::Axis<Acts::AxisType::Variable, bType> varB(edges1);
    return std::tie(varA, varB);
  }
};

// All 2D VarVar options
using VarBoundVarBound =
    VarVar<Acts::AxisBoundaryType::Bound, Acts::AxisBoundaryType::Bound>;
using VarBoundVarOpen =
    VarVar<Acts::AxisBoundaryType::Bound, Acts::AxisBoundaryType::Open>;
using VarBoundVarClosed =
    VarVar<Acts::AxisBoundaryType::Bound, Acts::AxisBoundaryType::Closed>;
using VarOpenVarBound =
    VarVar<Acts::AxisBoundaryType::Open, Acts::AxisBoundaryType::Bound>;
using VarOpenVarOpen =
    VarVar<Acts::AxisBoundaryType::Open, Acts::AxisBoundaryType::Open>;
using VarOpenVarClosed =
    VarVar<Acts::AxisBoundaryType::Open, Acts::AxisBoundaryType::Closed>;
using VarClosedVarBound =
    VarVar<Acts::AxisBoundaryType::Closed, Acts::AxisBoundaryType::Bound>;
using VarClosedVarOpen =
    VarVar<Acts::AxisBoundaryType::Closed, Acts::AxisBoundaryType::Open>;
using VarClosedVarClosed =
    VarVar<Acts::AxisBoundaryType::Closed, Acts::AxisBoundaryType::Closed>;

// Generate the possible axes in this case
using PossibleAxes =
    TypeList<EqBound, EqOpen, EqClosed,
             // All 1D Var  options
             VarBound, VarOpen, VarClosed,
             // All 2D EqEq options
             EqBoundEqBound, EqBoundEqOpen, EqBoundEqClosed, EqOpenEqBound,
             EqOpenEqOpen, EqOpenEqClosed, EqClosedEqBound, EqClosedEqOpen,
             EqClosedEqClosed,
             // All 2D EqVar options
             EqBoundVarBound, EqBoundVarOpen, EqBoundVarClosed, EqOpenVarBound,
             EqOpenVarOpen, EqOpenVarClosed, EqClosedVarBound, EqClosedVarOpen,
             EqClosedVarClosed,
             // All 2D VarEq options
             VarBoundEqBound, VarBoundEqOpen, VarBoundEqClosed, VarOpenEqBound,
             VarOpenEqOpen, VarOpenEqClosed, VarClosedEqBound, VarClosedEqOpen,
             VarClosedEqClosed,
             // All 2D VarEq options
             VarBoundVarBound, VarBoundVarOpen, VarBoundVarClosed,
             VarOpenVarBound, VarOpenVarOpen, VarOpenVarClosed,
             VarClosedVarBound, VarClosedVarOpen, VarClosedVarClosed>;

}  // namespace Acts::GridAxisGenerators
