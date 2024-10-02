// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/EventData/detail/PrintParameters.hpp"

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <array>
#include <cstddef>
#include <iomanip>
#include <ostream>
#include <string>

namespace {

constexpr std::array<const char*, Acts::eBoundSize> makeBoundNames() {
  std::array<const char*, Acts::eBoundSize> names = {nullptr};
  // must be set by index since the order is user-configurable
  names[Acts::eBoundLoc0] = "loc0:";
  names[Acts::eBoundLoc1] = "loc1:";
  names[Acts::eBoundTime] = "time:";
  names[Acts::eBoundPhi] = "phi:";
  names[Acts::eBoundTheta] = "theta:";
  names[Acts::eBoundQOverP] = "q/p:";
  return names;
}

constexpr std::array<const char*, Acts::eFreeSize> makeFreeNames() {
  std::array<const char*, Acts::eFreeSize> names = {nullptr};
  // must be set by index since the order is user-configurable
  names[Acts::eFreePos0] = "pos0:";
  names[Acts::eFreePos1] = "pos1:";
  names[Acts::eFreePos2] = "pos2:";
  names[Acts::eFreeTime] = "time:";
  names[Acts::eFreeDir0] = "dir0:";
  names[Acts::eFreeDir1] = "dir1:";
  names[Acts::eFreeDir2] = "dir2:";
  names[Acts::eFreeQOverP] = "q/p:";
  return names;
}

constexpr std::array<std::size_t, 8> kMonotonic = {
    0, 1, 2, 3, 4, 5, 6, 7,
};

constexpr std::size_t kNamesMaxSize = 6;

/// Print parameters and associated covariance.
///
/// @param os Output stream
/// @param names Container with all names
/// @param nameIndices Identify the name for each parameter value
/// @param params Parameter values
/// @param cov Covariance matrix
///
/// The output format format is
///
///     name0: value0 +- stddev0  corr00
///     name1: value1 +- stddev1  corr10 corr11
///     name2: value2 +- stddev2  corr20 corr21 corr22
///     ...
///
/// w/o a newline for the last line for better compatibility with the logging
/// macros.
template <typename names_container_t, typename indices_container_t,
          typename parameters_t, typename covariance_t>
void printParametersCovariance(std::ostream& os, const names_container_t& names,
                               const indices_container_t& nameIndices,
                               const Eigen::MatrixBase<parameters_t>& params,
                               const Eigen::MatrixBase<covariance_t>& cov) {
  EIGEN_STATIC_ASSERT_VECTOR_ONLY(parameters_t);

  // save stream formatting state
  auto flags = os.flags();
  auto precision = os.precision();

  // compute the standard deviations
  auto stddev = cov.diagonal().cwiseSqrt().eval();

  for (Eigen::Index i = 0; i < params.size(); ++i) {
    // no newline after the last line. e.g. the log macros automatically add a
    // newline and having a finishing newline would lead to empty lines.
    if (0 < i) {
      os << '\n';
    }
    // show name
    os << std::setw(kNamesMaxSize) << std::left << names[nameIndices[i]];
    // show value
    os << " ";
    os << std::defaultfloat << std::setprecision(4);
    os << std::setw(10) << std::right << params[i];
    // show standard deviation
    os << " +- ";
    os << std::setw(10) << std::left << stddev[i];
    // show lower-triangular part of the correlation matrix
    os << " ";
    os << std::fixed << std::setprecision(3);
    for (Eigen::Index j = 0; j <= i; ++j) {
      auto corr = cov(i, j) / (stddev[i] * stddev[j]);
      os << " " << std::setw(6) << std::right << corr;
    }
  }

  // restore previous stream formatting state
  os.flags(flags);
  os.precision(precision);
}

/// Print parameters only.
///
/// @param os Output stream
/// @param names Container with all names
/// @param nameIndices Identify the name for each parameter value
/// @param params Parameter values
///
/// The output format format is
///
///     name0: value0
///     name1: value1
///     name2: value2
///     ...
///
/// w/o a newline for the last line for better compatibility with the logging
/// macros.
template <typename names_container_t, typename indices_container_t,
          typename parameters_t>
void printParameters(std::ostream& os, const names_container_t& names,
                     const indices_container_t& nameIndices,
                     const Eigen::MatrixBase<parameters_t>& params) {
  EIGEN_STATIC_ASSERT_VECTOR_ONLY(parameters_t);

  // save stream formatting state
  auto flags = os.flags();
  auto precision = os.precision();

  for (Eigen::Index i = 0; i < params.size(); ++i) {
    // no newline after the last line. e.g. the log macros automatically add a
    // newline and having a finishing newline would lead to empty lines.
    if (0 < i) {
      os << '\n';
    }
    // show name
    os << std::setw(kNamesMaxSize) << std::left << names[nameIndices[i]];
    // show value
    os << " ";
    os << std::defaultfloat << std::setprecision(4);
    os << std::setw(10) << std::right << params[i];
  }

  // restore previous stream formatting state
  os.flags(flags);
  os.precision(precision);
}

using ParametersMap = Eigen::Map<const Acts::ActsDynamicVector>;
using CovarianceMap = Eigen::Map<const Acts::ActsDynamicMatrix>;

}  // namespace

void Acts::detail::printBoundParameters(std::ostream& os,
                                        const Acts::Surface& surface,
                                        const Acts::BoundVector& params,
                                        const Acts::BoundSquareMatrix* cov) {
  if (cov != nullptr) {
    printParametersCovariance(os, makeBoundNames(), kMonotonic, params, *cov);
  } else {
    printParameters(os, makeBoundNames(), kMonotonic, params);
  }
  os << "\non surface " << surface.geometryId() << " of type "
     << surface.name();
}

void Acts::detail::printFreeParameters(std::ostream& os,
                                       const Acts::FreeVector& params,
                                       const Acts::FreeMatrix* cov) {
  if (cov != nullptr) {
    printParametersCovariance(os, makeFreeNames(), kMonotonic, params, *cov);
  } else {
    printParameters(os, makeFreeNames(), kMonotonic, params);
  }
}

void Acts::detail::printMeasurement(std::ostream& os, BoundIndices size,
                                    const std::uint8_t* indices,
                                    const ActsScalar* params,
                                    const ActsScalar* cov) {
  auto s = static_cast<Eigen::Index>(size);
  printParametersCovariance(os, makeBoundNames(), indices,
                            ParametersMap(params, s), CovarianceMap(cov, s, s));
}

void Acts::detail::printMeasurement(std::ostream& os, FreeIndices size,
                                    const std::uint8_t* indices,
                                    const ActsScalar* params,
                                    const ActsScalar* cov) {
  auto s = static_cast<Eigen::Index>(size);
  printParametersCovariance(os, makeFreeNames(), indices,
                            ParametersMap(params, s), CovarianceMap(cov, s, s));
}
