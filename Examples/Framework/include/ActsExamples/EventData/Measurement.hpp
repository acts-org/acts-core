// This file is part of the Acts project.
//
// Copyright (C) 2020-2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MeasurementHelpers.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/SubspaceHelpers.hpp"
#include "Acts/EventData/Types.hpp"
#include "Acts/EventData/detail/CalculateResiduals.hpp"
#include "Acts/EventData/detail/ParameterTraits.hpp"
#include "Acts/EventData/detail/PrintParameters.hpp"

#include <array>
#include <cstddef>
#include <iosfwd>
#include <type_traits>
#include <variant>
#include <vector>

#include <boost/container/static_vector.hpp>

namespace ActsExamples {

/// A measurement of a variable-size subspace of the full parameters.
///
/// @tparam indices_t Parameter index type, determines the full parameter space
///
/// The measurement intentionally does not store a pointer/reference to the
/// reference object in the geometry hierarchy, i.e. the surface or volume. The
/// reference object can already be identified via the geometry identifier
/// provided by the source link. Since a measurement **must** be anchored within
/// the geometry hierarchy, all measurement surfaces and volumes **must**
/// provide valid geometry identifiers. In all use-cases, e.g. Kalman filtering,
/// a pointer/reference to the reference object is available before the
/// measurement is accessed; e.g. the propagator provides the surface pointer
/// during navigation, which is then used to lookup possible measurements.
///
/// The pointed-to geometry object would differ depending on the parameter type.
/// This means either, that there needs to be an additional variable type or
/// that a pointer to a base object is stored (requiring a `dynamic_cast` later
/// on). Both variants add additional complications. Since the geometry object
/// is not required anyway (as discussed above), not storing it removes all
/// these complications altogether.
template <typename indices_t>
class VariableSizeMeasurement {
 public:
  static constexpr std::size_t kFullSize =
      Acts::detail::kParametersSize<indices_t>;

  using Scalar = Acts::ActsScalar;

  using SubspaceIndex = std::uint8_t;
  using SubspaceIndices =
      boost::container::static_vector<SubspaceIndex, kFullSize>;

  /// Vector type containing for measured parameter values.
  template <std::size_t dim>
  using ParametersVector = Eigen::Matrix<Scalar, dim, 1>;
  template <std::size_t dim>
  using ParametersVectorMap = Eigen::Map<ParametersVector<dim>>;
  template <std::size_t dim>
  using ConstParametersVectorMap = Eigen::Map<const ParametersVector<dim>>;
  using EffectiveParametersVector = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>;
  using EffectiveParametersVectorMap = Eigen::Map<EffectiveParametersVector>;
  using ConstEffectiveParametersVectorMap =
      Eigen::Map<const EffectiveParametersVector>;

  /// Matrix type for the measurement covariance.
  template <std::size_t dim>
  using CovarianceMatrix = Eigen::Matrix<Scalar, dim, dim>;
  template <std::size_t dim>
  using CovarianceMatrixMap = Eigen::Map<CovarianceMatrix<dim>>;
  template <std::size_t dim>
  using ConstCovarianceMatrixMap = Eigen::Map<const CovarianceMatrix<dim>>;
  using EffectiveCovarianceMatrix =
      Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>;
  using EffectiveCovarianceMatrixMap = Eigen::Map<EffectiveCovarianceMatrix>;
  using ConstEffectiveCovarianceMatrixMap =
      Eigen::Map<const EffectiveCovarianceMatrix>;

  using FullParametersVector = Acts::ActsVector<kFullSize>;
  using FullCovarianceMatrix = Acts::ActsSquareMatrix<kFullSize>;

  using ProjectionMatrix = Eigen::Matrix<Scalar, Eigen::Dynamic, kFullSize>;
  using ExpansionMatrix = Eigen::Matrix<Scalar, kFullSize, Eigen::Dynamic>;

  /// Construct from source link, subset indices, and measured data.
  ///
  /// @tparam parameters_t Input parameters vector type
  /// @tparam covariance_t Input covariance matrix type
  /// @param source The link that connects to the underlying detector readout
  /// @param subspaceIndices Which parameters are measured
  /// @param params Measured parameters values
  /// @param cov Measured parameters covariance
  ///
  /// @note The indices must be ordered and must describe/match the content
  ///   of parameters and covariance.
  template <typename other_indices_t, std::size_t kSize, typename parameters_t,
            typename covariance_t>
  VariableSizeMeasurement(
      Acts::SourceLink source,
      const std::array<other_indices_t, kSize>& subspaceIndices,
      const Eigen::MatrixBase<parameters_t>& params,
      const Eigen::MatrixBase<covariance_t>& cov)
      : m_source(std::move(source)) {
    static_assert(kSize == parameters_t::RowsAtCompileTime,
                  "Parameter size mismatch");
    static_assert(kSize == covariance_t::RowsAtCompileTime,
                  "Covariance rows mismatch");
    static_assert(kSize == covariance_t::ColsAtCompileTime,
                  "Covariance cols mismatch");

    m_subspaceIndices.resize(subspaceIndices.size());
    std::transform(subspaceIndices.begin(), subspaceIndices.end(),
                   m_subspaceIndices.begin(), [](auto index) {
                     return static_cast<SubspaceIndex>(index);
                   });

    parameters<kSize>() = params;
    covariance<kSize>() = cov;
  }
  /// A measurement can only be constructed with valid parameters.
  VariableSizeMeasurement() = delete;
  VariableSizeMeasurement(const VariableSizeMeasurement&) = default;
  VariableSizeMeasurement(VariableSizeMeasurement&&) = default;
  ~VariableSizeMeasurement() = default;
  VariableSizeMeasurement& operator=(const VariableSizeMeasurement&) = default;
  VariableSizeMeasurement& operator=(VariableSizeMeasurement&&) = default;

  /// Source link that connects to the underlying detector readout.
  const Acts::SourceLink& sourceLink() const { return m_source; }

  constexpr std::size_t size() const { return m_subspaceIndices.size(); }

  /// Check if a specific parameter is part of this measurement.
  bool contains(indices_t i) const {
    return std::find(m_subspaceIndices.begin(), m_subspaceIndices.end(), i) !=
           m_subspaceIndices.end();
  }

  std::size_t indexOf(indices_t i) const {
    auto it = std::find(m_subspaceIndices.begin(), m_subspaceIndices.end(), i);
    assert(it != m_subspaceIndices.end());
    return std::distance(m_subspaceIndices.begin(), it);
  }

  /// The measurement indices
  const SubspaceIndices& subspaceIndices() const { return m_subspaceIndices; }

  template <std::size_t dim>
  Acts::SubspaceIndices<dim> subspaceIndices() const {
    assert(dim == size());
    Acts::SubspaceIndices<dim> result;
    std::copy(m_subspaceIndices.begin(), m_subspaceIndices.end(),
              result.begin());
    return result;
  }

  Acts::BoundSubspaceIndices boundSubsetIndices() const
    requires(std::is_same_v<indices_t, Acts::BoundIndices>)
  {
    Acts::BoundSubspaceIndices result = Acts::kBoundSubspaceIndicesInvalid;
    std::copy(m_subspaceIndices.begin(), m_subspaceIndices.end(),
              result.begin());
    return result;
  }

  template <std::size_t dim>
  ConstParametersVectorMap<dim> parameters() const {
    assert(dim == size());
    return ConstParametersVectorMap<dim>{m_params.data()};
  }
  template <std::size_t dim>
  ParametersVectorMap<dim> parameters() {
    assert(dim == size());
    return ParametersVectorMap<dim>{m_params.data()};
  }
  ConstEffectiveParametersVectorMap parameters() const {
    return ConstEffectiveParametersVectorMap{m_params.data(),
                                             static_cast<Eigen::Index>(size())};
  }
  EffectiveParametersVectorMap parameters() {
    return EffectiveParametersVectorMap{m_params.data(),
                                        static_cast<Eigen::Index>(size())};
  }

  template <std::size_t dim>
  ConstCovarianceMatrixMap<dim> covariance() const {
    assert(dim == size());
    return ConstCovarianceMatrixMap<dim>{m_cov.data()};
  }
  template <std::size_t dim>
  CovarianceMatrixMap<dim> covariance() {
    assert(dim == size());
    return CovarianceMatrixMap<dim>{m_cov.data()};
  }
  ConstEffectiveCovarianceMatrixMap covariance() const {
    return ConstEffectiveCovarianceMatrixMap{m_cov.data(),
                                             static_cast<Eigen::Index>(size()),
                                             static_cast<Eigen::Index>(size())};
  }
  EffectiveCovarianceMatrixMap covariance() {
    return EffectiveCovarianceMatrixMap{m_cov.data(),
                                        static_cast<Eigen::Index>(size()),
                                        static_cast<Eigen::Index>(size())};
  }

  FullParametersVector fullParameters() const {
    FullParametersVector result = FullParametersVector::Zero();
    for (std::size_t i = 0; i < size(); ++i) {
      result[m_subspaceIndices[i]] = parameters()[i];
    }
    return result;
  }

  FullCovarianceMatrix fullCovariance() const {
    FullCovarianceMatrix result = FullCovarianceMatrix::Zero();
    for (std::size_t i = 0; i < size(); ++i) {
      for (std::size_t j = 0; j < size(); ++j) {
        result(m_subspaceIndices[i], m_subspaceIndices[j]) = covariance()(i, j);
      }
    }
    return result;
  }

 private:
  Acts::SourceLink m_source;
  SubspaceIndices m_subspaceIndices;
  std::array<Scalar, kFullSize> m_params{};
  std::array<Scalar, kFullSize * kFullSize> m_cov{};
};

/// Construct a variable-size measurement for the given indices.
///
/// @tparam parameters_t Input parameters vector type
/// @tparam covariance_t Input covariance matrix type
/// @tparam indices_t Parameter index type, determines the full parameter space
/// @tparam tail_indices_t Helper types required to support variadic arguments;
///   all types must be convertibale to `indices_t`.
/// @param source The link that connects to the underlying detector readout
/// @param params Measured parameters values
/// @param cov Measured parameters covariance
/// @param index0 Required parameter index, measurement must be at least 1d
/// @param tailIndices Additional parameter indices for larger measurements
/// @return Variable-size measurement w/ the correct type and given inputs
///
/// @note The indices must be ordered and must be consistent with the content of
/// parameters and covariance.
template <typename parameters_t, typename covariance_t, typename indices_t,
          typename... tail_indices_t>
VariableSizeMeasurement<indices_t> makeVariableSizeMeasurement(
    Acts::SourceLink source, const Eigen::MatrixBase<parameters_t>& params,
    const Eigen::MatrixBase<covariance_t>& cov, indices_t index0,
    tail_indices_t... tailIndices) {
  using IndexContainer = std::array<indices_t, 1u + sizeof...(tail_indices_t)>;
  return {std::move(source), IndexContainer{index0, tailIndices...}, params,
          cov};
}

/// Type that can hold all possible bound measurements.
using BoundVariableMeasurement = VariableSizeMeasurement<Acts::BoundIndices>;

/// Variable measurement type that can contain all possible combinations.
using Measurement = BoundVariableMeasurement;

/// Container of measurements.
///
/// In contrast to the source links, the measurements themself must not be
/// orderable. The source links stored in the measurements are treated
/// as opaque here and no ordering is enforced on the stored measurements.
using MeasurementContainer = std::vector<Measurement>;

}  // namespace ActsExamples
