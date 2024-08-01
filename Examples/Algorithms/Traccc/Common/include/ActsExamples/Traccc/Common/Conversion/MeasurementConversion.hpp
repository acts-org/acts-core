// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Plugin include(s)
#include "Acts/Plugins/Traccc/Detail/AlgebraConversion.hpp"

// Acts include(s)
#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/EventData/Measurement.hpp"

// Acts Examples include(s)
#include "ActsExamples/EventData/IndexSourceLink.hpp"

// Detray include(s)
#include "detray/core/detector.hpp"
#include "detray/tracks/bound_track_parameters.hpp"

// Traccc include(s)
#include "traccc/definitions/qualifiers.hpp"
#include "traccc/definitions/track_parametrization.hpp"
#include "traccc/edm/measurement.hpp"
#include "traccc/edm/track_state.hpp"

// System include(s)
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <variant>
#include <vector>

namespace ActsExamples::Traccc::Common::Conversion {

/// @brief Converts a traccc bound index to an Acts bound index.
/// @param tracccBoundIndex the traccc bound index.
/// @returns an Acts bound index.
Acts::BoundIndices boundIndex(const traccc::bound_indices tracccBoundIndex);

/// @brief Creates an Acts measurement from a traccc measurement.
/// @tparam the dimension of the Acts measurement (subspace size).
/// @param m the traccc measurement.
/// @param sl the Acts source link to use for the Acts measurement.
/// @returns an Acts measurement with data copied from the traccc measurement
/// and with its source link set to the one provided to the function.
template <std::size_t dim>
inline ActsExamples::FixedSizeMeasurement<Acts::BoundIndices, dim> measurement(
    const traccc::measurement& m, const Acts::SourceLink sl) {
  // Currently, all traccc measurements have dim 2.
  if constexpr (dim != 2) {
    std::string errorMsg =
        "Dimension is not 2 (dimension = " + std::to_string(m.meas_dim) + ")";
    throw std::runtime_error(errorMsg.c_str());
  }
  auto params = Acts::TracccPlugin::detail::toActsVector<dim>(m.local);
  std::array<Acts::BoundIndices, dim> indices;
  for (unsigned int i = 0; i < dim; i++) {
    indices[i] = boundIndex(traccc::bound_indices(m.subs.get_indices()[i]));
  }
  auto cov = Eigen::DiagonalMatrix<Acts::ActsScalar, static_cast<int>(dim)>(
                 Acts::TracccPlugin::detail::toActsVector<dim>(m.variance))
                 .toDenseMatrix();
  return ActsExamples::FixedSizeMeasurement<Acts::BoundIndices, dim>(
      std::move(sl), indices, params, cov);
}

/// @brief Creates an Acts bound variant measurement from a traccc measurement.
/// Using recursion, the functions determines the dimension of the traccc
/// measurement which is used for the Acts measurement that the bound variant
/// measurement holds. The dimension must lie between [0; max_dim].
/// @tparam max_dim the largest possible dimension of any measurement type in the variant (default = 4).
/// @param m the traccc measurement.
/// @param sl the Acts source link to use for the Acts measurement.
/// @returns an Acts bound variant measurement with data copied from the traccc measurement
/// and with its source link set to the one provided to the function.
template <std::size_t max_dim = 4UL>
inline ActsExamples::BoundVariantMeasurement boundVariantMeasurement(
    const traccc::measurement& m, const Acts::SourceLink sl) {
  if constexpr (max_dim == 0UL) {
    std::string errorMsg = "Invalid/mismatching measurement dimension: " +
                           std::to_string(m.meas_dim);
    throw std::runtime_error(errorMsg.c_str());
  } else {
    if (m.meas_dim == max_dim) {
      return measurement<max_dim>(m, sl);
    }
    return boundVariantMeasurement<max_dim - 1>(m, sl);
  }
}

/// @brief Gets the the local position of the measurement.
/// @param measurement the Acts measurement.
/// @returns A two-dimensional vector containing the local position.
/// The first item in the vector is local position on axis 0 and
/// I.e., [local position (axis 0), local position (axis 1)].
/// @note if the dimension is less than 2 then the remaining values are set to 0.
template <std::size_t dim>
inline Acts::ActsVector<2> getLocal(
    const ActsExamples::FixedSizeMeasurement<Acts::BoundIndices, dim>&
        measurement) {
  traccc::scalar loc0 = 0;
  traccc::scalar loc1 = 0;
  if constexpr (dim > Acts::BoundIndices::eBoundLoc0) {
    loc0 = measurement.parameters()(Acts::BoundIndices::eBoundLoc0);
  }
  if constexpr (dim > Acts::BoundIndices::eBoundLoc1) {
    loc1 = measurement.parameters()(Acts::BoundIndices::eBoundLoc1);
  }
  return Acts::ActsVector<2>(loc0, loc1);
}

/// @brief Get the the local position of the measurement.
/// @param measurement the Acts bound variant measurement.
/// @return A two-dimensional vector containing the local position.
/// I.e., [local position (axis 0), local position (axis 1)].
/// @note if the dimension is less than 2 then the remaining values are set to 0.
inline Acts::ActsVector<2> getLocal(
    const ActsExamples::BoundVariantMeasurement& measurement) {
  return std::visit([](auto& m) { return getLocal(m); }, measurement);
}

/// @brief Get the the variance of the measurement.
/// @param measurement the Acts measurement.
/// @return A two-dimensional vector containing the variance.
/// I.e., [variance (axis 0), variance (axis 1)].
/// @note if the dimension is less than 2 then the remaining values are set to 0.
template <std::size_t dim>
inline Acts::ActsVector<2> getVariance(
    const ActsExamples::FixedSizeMeasurement<Acts::BoundIndices, dim>&
        measurement) {
  traccc::scalar var0 = 0;
  traccc::scalar var1 = 0;
  if constexpr (dim >= Acts::BoundIndices::eBoundLoc0) {
    var0 = measurement.covariance()(Acts::BoundIndices::eBoundLoc0,
                                    Acts::BoundIndices::eBoundLoc0);
  }
  if constexpr (dim > Acts::BoundIndices::eBoundLoc1) {
    var1 = measurement.covariance()(Acts::BoundIndices::eBoundLoc1,
                                    Acts::BoundIndices::eBoundLoc1);
  }
  return Acts::ActsVector<2>(var0, var1);
}

/// @brief Get the the variance of the measurement.
/// @param measurement the Acts bound variant measurement.
/// @return A two-dimensional vector containing the variance.
/// I.e., [variance (axis 0), variance (axis 1)].
/// @note if the dimension is less than 2 then the remaining values are set to 0.
inline Acts::ActsVector<2> getVariance(
    const ActsExamples::BoundVariantMeasurement& measurement) {
  return std::visit([](auto& m) { return getVariance(m); }, measurement);
}

/// @brief Converts traccc measurements to acts measurements.
/// @param detector The detray detector,
/// @param measurements The traccc measurements,
/// @return A vector of Acts bound variant measurements.
/// @note The type IndexSourceLink is used for the measurements' source links.
template <typename detector_t, typename allocator_t>
inline auto createActsMeasurements(
    const detector_t& detector,
    const std::vector<traccc::measurement, allocator_t>& measurements) {
  std::vector<ActsExamples::BoundVariantMeasurement> measurementContainer;
  for (const traccc::measurement& m : measurements) {
    Acts::GeometryIdentifier moduleGeoId(
        detector.surface(m.surface_link).source);
    Index measurementIdx = measurementContainer.size();
    IndexSourceLink idxSourceLink{moduleGeoId, measurementIdx};
    measurementContainer.push_back(
        boundVariantMeasurement(m, Acts::SourceLink{idxSourceLink}));
  }
  return measurementContainer;
}

}  // namespace ActsExamples::Traccc::Common::Conversion
