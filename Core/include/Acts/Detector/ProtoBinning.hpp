// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Acts::Experimental {

/// @brief  Simple helper class to define a binning structure
///
/// @note no checks are performed on the consistency, this is
/// only for convenience that the binning can be defined and then
/// translated into concrete axis types
struct ProtoBinning {
  /// The binning value of this
  BinningValue binValue;
  /// The axis type: equidistant or variable
  Acts::AxisType axisType = Acts::AxisType::Equidistant;
  /// The axis boundary type: Open, Bound or Closed
  Acts::AxisBoundaryType boundaryType = Acts::AxisBoundaryType::Bound;
  /// The binning edges
  std::vector<ActsScalar> edges = {};
  /// An expansion for the filling (in bins)
  std::size_t expansion = 0u;
  /// Indication if this is an auto-range binning
  bool autorange = false;

  /// Convenience constructors - for variable binning
  ///
  /// @param bValue the value/cast in which this is binned
  /// @param bType the axis boundary type
  /// @param e the bin edges (variable binning)
  /// @param exp the expansion (in bins)
  ProtoBinning(BinningValue bValue, Acts::AxisBoundaryType bType,
               const std::vector<ActsScalar>& e, std::size_t exp = 0u)
      : binValue(bValue),
        axisType(Acts::AxisType::Variable),
        boundaryType(bType),
        edges(e),
        expansion(exp) {
    if (edges.size() < 2u) {
      throw std::invalid_argument(
          "ProtoBinning: Invalid binning, at least two edges are needed.");
    }
  }

  /// Convenience constructors - for equidistant binning
  ///
  /// @param bValue the value/cast in which this is binned
  /// @param bType the axis boundary type
  /// @param minE the lowest edge of the binning
  /// @param maxE the highest edge of the binning
  /// @param nbins the number of bins
  /// @param exp the expansion (in bins)
  ProtoBinning(BinningValue bValue, Acts::AxisBoundaryType bType,
               ActsScalar minE, ActsScalar maxE, std::size_t nbins,
               std::size_t exp = 0u)
      : binValue(bValue), boundaryType(bType), expansion(exp) {
    if (minE >= maxE) {
      std::string msg = "ProtoBinning: Invalid binning for value '";
      msg += binningValueName(bValue);
      msg += "', min edge (" + std::to_string(minE) + ") ";
      msg += " needs to be smaller than max edge (";
      msg += std::to_string(maxE) + ").";
      throw std::invalid_argument(msg);
    }
    if (nbins < 1u) {
      throw std::invalid_argument(
          "ProtoBinning: Invalid binning, at least one bin is needed.");
    }

    ActsScalar stepE = (maxE - minE) / nbins;
    edges.reserve(nbins + 1);
    for (std::size_t i = 0; i <= nbins; i++) {
      edges.push_back(minE + i * stepE);
    }
  }

  /// Placeholder constructors - for equidistant binning
  ///
  /// @note this is designed to give a binning prescription
  /// when the actual extent is not yet evaluated, only works
  /// for equidistant binning obviously
  ///
  /// @param bValue the value/cast in which this is binned
  /// @param bType the axis boundary type
  /// @param nbins the number of bins
  /// @param exp the expansion (in bins)
  ProtoBinning(BinningValue bValue, Acts::AxisBoundaryType bType,
               std::size_t nbins, std::size_t exp = 0u)
      : binValue(bValue),
        boundaryType(bType),
        edges(nbins + 1, 0.),
        expansion(exp),
        autorange(true) {}

  // Return the number of bins
  std::size_t bins() const { return edges.size() - 1u; }

  // Screen output
  std::string toString() const {
    std::stringstream ss;
    ss << "ProtoBinning: " << bins() << " bins in "
       << binningValueName(binValue);
    ss << (axisType == Acts::AxisType::Variable ? ", variable "
                                                : ", equidistant ");
    if (!autorange) {
      ss << "within [" << edges.front() << ", " << edges.back() << "] ";
    } else {
      ss << "within automatic range";
    }
    return ss.str();
  }
};

/// @brief A binning description, it helps for screen output
struct BinningDescription {
  /// Convert the binning description into a bin utility
  ///
  /// @param binUtility the bin utility to be converted into a BinningDescription
  static BinningDescription fromBinUtility(const BinUtility& binUtility) {
    BinningDescription bDesc;
    for (const auto& bData : binUtility.binningData()) {
      // One proto binning per binning data
      Acts::AxisBoundaryType boundaryType =
          bData.option == open ? Acts::AxisBoundaryType::Bound
                               : Acts::AxisBoundaryType::Closed;
      std::vector<ActsScalar> edges;
      if (bData.type == equidistant) {
        bDesc.binning.push_back(ProtoBinning(bData.binvalue, boundaryType,
                                             bData.min, bData.max, bData.bins(),
                                             0u));

      } else {
        std::for_each(bData.boundaries().begin(), bData.boundaries().end(),
                      [&](ActsScalar edge) { edges.push_back(edge); });
        bDesc.binning.push_back(
            ProtoBinning(bData.binvalue, boundaryType, edges, 0u));
      }
    }
    return bDesc;
  }

  /// Convert to a BinUtility - only basic types are supported
  ///
  BinUtility toBinUtility() const {
    BinUtility binUtility;
    for (const auto& b : binning) {
      Acts::BinningOption bOption =
          b.boundaryType == Acts::AxisBoundaryType::Bound ? Acts::open
                                                          : Acts::closed;
      if (b.axisType == Acts::AxisType::Equidistant) {
        binUtility += BinUtility(b.bins(), b.edges.front(), b.edges.back(),
                                 bOption, b.binValue);
      } else {
        std::vector<float> edges;
        std::for_each(b.edges.begin(), b.edges.end(),
                      [&](ActsScalar edge) { edges.push_back(edge); });
        binUtility += BinUtility(edges, bOption, b.binValue);
      }
    }
    return binUtility;
  }

  /// The contained binnings
  std::vector<ProtoBinning> binning;

  // Screen output
  std::string toString() const {
    std::stringstream ss;
    ss << "BinningDescription: " << binning.size() << "D" << std::endl;
    for (const auto& b : binning) {
      ss << "  " << b.toString() << std::endl;
    }
    return ss.str();
  }
};

}  // namespace Acts::Experimental
