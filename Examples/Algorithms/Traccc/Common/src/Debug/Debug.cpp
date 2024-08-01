// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Acts include(s)
#include "Acts/Definitions/Algebra.hpp"

// Acts Examples include(s)
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/Traccc/Common/Conversion/MeasurementConversion.hpp"

// System include(s).
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <sstream>

// This file is for debugging and for getting the matching between two
// collections of measurements as a string.

namespace ActsExamples::Traccc::Common::Measurement {

namespace {

/// @returns a nicely formatted string of a vector representing a point.
std::string toString(const Acts::ActsVector<2>& vec) {
  std::stringstream ss;
  ss << "(" << vec[0] << ", " << vec[1] << ")";
  return ss.str();
}

/// @brief Structure to hold table data
struct MeasurementMatchRow {
  std::size_t idx1;
  Acts::ActsVector<2> local1;
  Acts::ActsVector<2> variance1;

  std::size_t idx2;
  Acts::ActsVector<2> local2;
  Acts::ActsVector<2> variance2;

  Acts::ActsScalar distanceLocal;
};

/// @brief Creates a table with data and measurements aligned according to the index map.
/// @param measurements1 the measurements (1).
/// @param measurements2 the measurements (2).
/// @param indexMap the index map: measurements1 indices -> measurement2 indices.
/// The index map describes which elements are related in the two measurement
/// collections.
/// @return a vector of MeasurementMatchRow.
auto createTable(
    const std::vector<ActsExamples::BoundVariantMeasurement>& measurements1,
    const std::vector<ActsExamples::BoundVariantMeasurement>& measurements2,
    const std::map<std::size_t, std::size_t>& indexMap) {
  std::vector<MeasurementMatchRow> table;
  for (std::size_t idx1 = 0; idx1 < measurements1.size(); ++idx1) {
    MeasurementMatchRow row;
    auto measurement1 = measurements1[idx1];
    row.idx1 = idx1;
    row.local1 = Conversion::getLocal(measurement1);
    row.variance1 = Conversion::getVariance(measurement1);

    auto idx2 = indexMap.at(idx1);
    auto measurement2 = measurements2[idx2];
    row.idx2 = idx2;
    row.local2 = Conversion::getLocal(measurement2);
    row.variance2 = Conversion::getVariance(measurement2);

    row.distanceLocal = (row.local1 - row.local2).norm();
    table.push_back(row);
  }
  return table;
}

}  // namespace

/// @brief Creates a string with the data of the measurements and their relation according to the index map.
/// @param measurements1 the measurements (1).
/// @param measurements2 the measurements (2).
/// @param indexMap the index map: measurements1 indices -> measurement2 indices.
/// The index map describes which elements are related in the two measurement
/// collections.
/// @return a string formatted as a table.
std::string pairingStatistics(
    const std::vector<ActsExamples::BoundVariantMeasurement>& measurements1,
    const std::vector<ActsExamples::BoundVariantMeasurement>& measurements2,
    const std::map<std::size_t, std::size_t>& indexMap) {
  auto table = createTable(measurements1, measurements2, indexMap);

  std::stringstream ss;

  // Column headers
  ss << std::setw(6) << "Idx1" << std::setw(25) << "Local1" << std::setw(35)
     << "Variance1" << std::setw(20) << "Idx2" << std::setw(25) << "Local2"
     << std::setw(35) << "Variance2" << std::setw(25) << "Distance Local"
     << std::endl;

  // Line separator
  ss << std::string(173, '-') << std::endl;

  // Print each row
  for (const auto& row : table) {
    ss << std::setw(6) << row.idx1 << std::setw(25) << toString(row.local1)
       << std::setw(35) << toString(row.variance1) << std::setw(20) << row.idx2
       << std::setw(25) << toString(row.local2) << std::setw(35)
       << toString(row.variance2) << std::setw(25) << std::fixed
       << std::setprecision(2) << row.distanceLocal << std::endl;
  }
  return ss.str();
}
}  // namespace ActsExamples::Traccc::Common::Measurement
