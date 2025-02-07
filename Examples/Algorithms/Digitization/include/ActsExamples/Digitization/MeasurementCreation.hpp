// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/Measurement.hpp"

#include <array>
#include <cstddef>
#include <tuple>
#include <vector>

namespace ActsExamples {

/// Struct to identify digitized parameters
///
/// Is public as it is also used by the I/O system
struct DigitizedParameters {
  std::vector<Acts::BoundIndices> indices = {};
  std::vector<double> values = {};
  std::vector<double> variances = {};

  Cluster cluster;
};

/// Helper method for created a measurement from digitized parameters
///
/// @param container The measurement container to insert into
/// @param geometryId The geometry ID of the measurement surface
/// @param dParams The digitized parameters of variable size
///
/// To be used also by the e I/O system
///
/// @return the measurement proxy
ActsExamples::VariableBoundMeasurementProxy createMeasurement(
    MeasurementContainer& container, Acts::GeometryIdentifier geometryId,
    const DigitizedParameters& dParams) noexcept(false);

/// Construct the constituents of a measurement.
///
/// @tparam kMeasDIM the full dimension of the measurement
///
/// @param dParams the struct of arrays of parameters to be created
///
/// @return a tuple of constituents for a measurement
template <std::size_t kMeasDIM>
std::tuple<std::array<Acts::BoundIndices, kMeasDIM>, Acts::ActsVector<kMeasDIM>,
           Acts::ActsSquareMatrix<kMeasDIM>>
measurementConstituents(const DigitizedParameters& dParams) {
  std::array<Acts::BoundIndices, kMeasDIM> indices{};
  Acts::ActsVector<kMeasDIM> par;
  Acts::ActsSquareMatrix<kMeasDIM> cov =
      Acts::ActsSquareMatrix<kMeasDIM>::Identity();
  for (Eigen::Index ei = 0; ei < static_cast<Eigen::Index>(kMeasDIM); ++ei) {
    indices[ei] = dParams.indices[ei];
    par[ei] = dParams.values[ei];
    cov(ei, ei) = dParams.variances[ei];
  }
  return {indices, par, cov};
}

}  // namespace ActsExamples
