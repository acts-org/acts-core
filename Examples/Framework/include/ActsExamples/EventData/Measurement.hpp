// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MeasurementHelpers.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/SubspaceHelpers.hpp"
#include "Acts/EventData/Types.hpp"
#include "Acts/EventData/detail/CalculateResiduals.hpp"
#include "Acts/EventData/detail/ParameterTraits.hpp"
#include "Acts/EventData/detail/PrintParameters.hpp"
#include "ActsExamples/EventData/MeasurementConcept.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <iosfwd>
#include <type_traits>
#include <variant>
#include <vector>

#include <boost/container/static_vector.hpp>

namespace ActsExamples {

template <typename Derived, std::size_t FullSize, bool ReadOnly>
class MeasurementProxyBase;
template <std::size_t FullSize, std::size_t Size, bool ReadOnly>
class FixedMeasurementProxy;
template <std::size_t FullSize, bool ReadOnly>
class VariableMeasurementProxy;

template <std::size_t Size>
using FixedBoundMeasurementProxy =
    FixedMeasurementProxy<Acts::eBoundSize, Size, false>;
template <std::size_t Size>
using ConstFixedBoundMeasurementProxy =
    FixedMeasurementProxy<Acts::eBoundSize, Size, true>;
using VariableBoundMeasurementProxy =
    VariableMeasurementProxy<Acts::eBoundSize, false>;
using ConstVariableBoundMeasurementProxy =
    VariableMeasurementProxy<Acts::eBoundSize, true>;

/// @brief A container to store and access measurements
///
/// This container stores measurements of different sizes and provides
/// access to them through fixed-size and variable-size proxies.
///
/// The measurements are stored densely in a flat buffer and the proxies
/// provide access to the individual measurements.
class MeasurementContainer {
 public:
  using Index = std::size_t;
  template <std::size_t Size>
  using FixedProxy = FixedMeasurementProxy<Acts::eBoundSize, Size, false>;
  template <std::size_t Size>
  using ConstFixedProxy = FixedMeasurementProxy<Acts::eBoundSize, Size, true>;
  using VariableProxy = VariableMeasurementProxy<Acts::eBoundSize, false>;
  using ConstVariableProxy = VariableMeasurementProxy<Acts::eBoundSize, true>;

  MeasurementContainer();

  /// @brief Get the number of measurements
  /// @return The number of measurements
  std::size_t size() const;

  /// @brief Reserve space for a number of measurements
  /// @param size The number of measurements to reserve space for
  void reserve(std::size_t size);

  /// @brief Add a measurement of a given size
  /// @param size The size of the measurement
  /// @return The index of the added measurement
  Index addMeasurement(std::uint8_t size);

  /// @brief Get a variable-size measurement proxy
  /// @param index The index of the measurement
  /// @return The variable-size measurement proxy
  VariableProxy getMeasurement(Index index);
  /// @brief Get a const variable-size measurement proxy
  /// @param index The index of the measurement
  /// @return The const variable-size measurement proxy
  ConstVariableProxy getMeasurement(Index index) const;

  /// @brief Get a fixed-size measurement proxy
  /// @tparam Size The size of the measurement
  /// @param index The index of the measurement
  /// @return The fixed-size measurement proxy
  template <std::size_t Size>
  FixedProxy<Size> getMeasurement(Index index) {
    return FixedProxy<Size>{*this, index};
  }
  /// @brief Get a const fixed-size measurement proxy
  /// @tparam Size The size of the measurement
  /// @param index The index of the measurement
  /// @return The const fixed-size measurement proxy
  template <std::size_t Size>
  ConstFixedProxy<Size> getMeasurement(Index index) const {
    return ConstFixedProxy<Size>{*this, index};
  }

  /// @brief Make a measurement of a given size
  /// @param size The size of the measurement
  /// @return The variable-size measurement proxy
  VariableProxy makeMeasurement(std::uint8_t size);
  /// @brief Make a fixed-size measurement
  /// @tparam Size The size of the measurement
  /// @return The fixed-size measurement proxy
  template <std::size_t Size>
  FixedProxy<Size> makeMeasurement() {
    return getMeasurement<Size>(addMeasurement(Size));
  }

  template <typename... Args>
  VariableProxy emplaceMeasurement(std::uint8_t size, Args&&... args);

  template <std::size_t Size, typename... Args>
  FixedProxy<Size> emplaceMeasurement(Args&&... args);

  template <bool Const>
  class IteratorImpl {
   public:
    using value_type =
        std::conditional_t<Const, ConstVariableProxy, VariableProxy>;
    using reference = value_type;
    using pointer = value_type*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    using Container = std::conditional_t<Const, const MeasurementContainer,
                                         MeasurementContainer>;

    IteratorImpl(Container& container, std::size_t index)
        : m_container(container), m_index(index) {}

    reference operator*() const { return m_container.getMeasurement(m_index); }

    pointer operator->() const { return &operator*(); }

    IteratorImpl& operator++() {
      ++m_index;
      return *this;
    }

    IteratorImpl operator++(int) {
      auto copy = *this;
      ++*this;
      return copy;
    }

    bool operator==(const IteratorImpl& other) const {
      return m_index == other.m_index;
    }

   private:
    Container& m_container;
    Index m_index;
  };

  using iterator = IteratorImpl<false>;
  using const_iterator = IteratorImpl<true>;

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;

 public:
  struct MeasurementEntry {
    std::size_t subspaceIndexOffset{};
    std::size_t parameterOffset{};
    std::size_t covarianceOffset{};
    std::uint8_t size{};
  };

  std::vector<MeasurementEntry> m_entries;

  std::vector<std::optional<Acts::SourceLink>> m_sourceLinks;
  std::vector<std::uint8_t> m_subspaceIndices;
  std::vector<double> m_parameters;
  std::vector<double> m_covariances;
};

/// @brief Base class for measurement proxies
///
/// This class provides common functionality for fixed-size and variable-size
/// measurement proxies.
///
/// @tparam Derived The derived measurement proxy class
/// @tparam FullSize The full size of the measurement
/// @tparam ReadOnly Whether the proxy is read-only
template <typename Derived, std::size_t FullSize, bool ReadOnly>
class MeasurementProxyBase {
 public:
  using Index = MeasurementContainer::Index;
  using SubspaceIndex = std::uint8_t;
  using Scalar = double;

  using FullVector = Acts::ActsVector<FullSize>;
  using FullSquareMatrix = Acts::ActsSquareMatrix<FullSize>;

  using Container = std::conditional_t<ReadOnly, const MeasurementContainer,
                                       MeasurementContainer>;

  MeasurementProxyBase(Container& container_, Index index_)
      : m_container(&container_), m_index(index_) {}
  template <typename OtherDerived, bool OtherReadOnly>
  MeasurementProxyBase(
      const MeasurementProxyBase<OtherDerived, FullSize, OtherReadOnly>& other)
    requires(ReadOnly == OtherReadOnly || ReadOnly)
      : m_container(&other.container()), m_index(other.index()) {}

  /// @brief Get the container of the measurement
  /// @return The container of the measurement
  Container& container() const { return *m_container; }
  /// @brief Get the index of the measurement
  /// @return The index of the measurement
  Index index() const { return m_index; }

  /// @brief Get the size of the measurement
  /// @return The size of the measurement
  std::size_t size() const { return container().m_entries.at(m_index).size; }

  /// @brief Check if the measurement contains a subspace index
  /// @param i The subspace index
  /// @return True if the measurement contains the subspace index
  template <typename indices_t>
  bool contains(indices_t i) const {
    return self().subspaceHelper().contains(i);
  }

  /// @brief Get the index of a subspace index in the measurement
  /// @param i The subspace index
  /// @return The index of the subspace index in the measurement
  template <typename indices_t>
  std::size_t indexOf(indices_t i) const {
    return self().subspaceHelper().indexOf(i);
  }

  /// @brief Set the source link of the measurement
  /// @param sourceLink The source link
  void setSourceLink(const Acts::SourceLink& sourceLink)
    requires(!ReadOnly)
  {
    container().m_sourceLinks.at(m_index) = sourceLink;
  }

  /// @brief Get the source link of the measurement
  /// @return The source link
  const Acts::SourceLink& sourceLink() const {
    return container().m_sourceLinks.at(m_index).value();
  }

  /// @brief Set the subspace indices of the measurement
  /// @param indices The subspace indices
  template <typename IndexContainer>
  void setSubspaceIndices(const IndexContainer& indices)
    requires(!ReadOnly)
  {
    assert(Acts::checkSubspaceIndices(indices, FullSize, size()) &&
           "Invalid indices");
    std::transform(indices.begin(), indices.end(),
                   self().subspaceIndexVector().begin(),
                   [](auto index) { return static_cast<Index>(index); });
  }

  /// @brief Get the measurement as a full-size vector
  /// @return The full-size measurement vector
  FullVector fullParameters() const {
    return self().subspaceHelper().expandVector(self().parameters());
  }

  /// @brief Get the covariance as a full-size square matrix
  /// @return The full-size covariance matrix
  FullSquareMatrix fullCovariance() const {
    return self().subspaceHelper().expandMatrix(self().covariance());
  }

  /// @brief Construct the measurement from a sourcelink, subspace vector,
  /// parameters, and covariance.
  ///
  template <typename Subspace, typename ParameterDerived,
            typename CovarianceDerived>
  void fill(const Acts::SourceLink& source_link, Subspace&& subspace,
            const Eigen::DenseBase<ParameterDerived>& parameters,
            const Eigen::DenseBase<CovarianceDerived>& covariance)
    requires(!ReadOnly)
  {
    setSourceLink(source_link);
    self().setSubspaceIndices(std::forward<Subspace>(subspace));
    self().parameters() = parameters;
    self().covariance() = covariance;
  }

  /// @brief Construct the measurement from a sourcelink, subspace vector,
  /// parameters, and covariance.
  ///
  template <MeasurementConcept OtherDerived>
  void fill(const OtherDerived& other)
    requires(!ReadOnly)
  {
    assert(size() == other.size() && "Size mismatch");
    fill(other.sourceLink(), other.subspaceIndexVector(), other.parameters(),
         other.covariance());
  }

  /// @brief Copy the data from another measurement
  /// @tparam OtherDerived The derived measurement proxy class of the other
  ///         measurement
  /// @param other The other measurement proxy
  template <typename OtherDerived>
  void copyFrom(const OtherDerived& other)
    requires(!ReadOnly) && requires {
      { this->fill(other) };
    }
  {
    fill(other);
  }

 protected:
  Derived& self() { return static_cast<Derived&>(*this); }
  const Derived& self() const { return static_cast<const Derived&>(*this); }

  Container* m_container;
  Index m_index;
};

/// @brief Fixed-size measurement proxy
///
/// This class provides access to a fixed-size measurement in a measurement
/// container.
///
/// @tparam FullSize The full size of the measurement
/// @tparam Size The size of the measurement
/// @tparam ReadOnly Whether the proxy is read-only
template <std::size_t FullSize, std::size_t Size, bool ReadOnly>
class FixedMeasurementProxy
    : public MeasurementProxyBase<
          FixedMeasurementProxy<FullSize, Size, ReadOnly>, FullSize, ReadOnly> {
 public:
  using Base =
      MeasurementProxyBase<FixedMeasurementProxy<FullSize, Size, ReadOnly>,
                           FullSize, ReadOnly>;
  using Index = typename Base::Index;
  using SubspaceIndex = typename Base::SubspaceIndex;
  using Scalar = typename Base::Scalar;
  using Container = typename Base::Container;

  using SubspaceHelper = Acts::FixedSubspaceHelper<FullSize, Size>;

  using SubspaceVector = Eigen::Matrix<SubspaceIndex, Size, 1>;
  using SubspaceVectorMap =
      std::conditional_t<ReadOnly, Eigen::Map<const SubspaceVector>,
                         Eigen::Map<SubspaceVector>>;

  using ParametersVector = Eigen::Matrix<Scalar, Size, 1>;
  using ParametersVectorMap =
      std::conditional_t<ReadOnly, Eigen::Map<const ParametersVector>,
                         Eigen::Map<ParametersVector>>;

  using CovarianceMatrix = Eigen::Matrix<Scalar, Size, Size>;
  using CovarianceMatrixMap =
      std::conditional_t<ReadOnly, Eigen::Map<const CovarianceMatrix>,
                         Eigen::Map<CovarianceMatrix>>;

  FixedMeasurementProxy(Container& container_, Index index_)
      : Base(container_, index_) {
    assert(container().m_entries.at(index()).size == Size && "Size mismatch");
  }
  template <typename OtherDerived, bool OtherReadOnly>
  FixedMeasurementProxy(
      const MeasurementProxyBase<OtherDerived, FullSize, OtherReadOnly>& other)
    requires(ReadOnly == OtherReadOnly || ReadOnly)
      : Base(other) {
    assert(container().m_entries.at(index()).size == Size && "Size mismatch");
  }

  using Base::container;
  using Base::index;

  /// @brief Get the size of the measurement
  /// @return The size of the measurement
  static constexpr std::size_t size() { return Size; }

  /// @brief Get the subspace helper for the measurement
  /// @return The subspace helper
  SubspaceHelper subspaceHelper() const {
    return SubspaceHelper{subspaceIndexVector()};
  }

  /// @brief Get the subspace indices of the measurement
  /// @return The subspace indices
  Acts::SubspaceIndices<Size> subspaceIndices() const {
    return subspaceHelper().indices();
  }

  /// @brief Get the subspace index vector of the measurement
  /// @return The subspace index vector
  SubspaceVectorMap subspaceIndexVector() const {
    return SubspaceVectorMap{
        container().m_subspaceIndices.data() +
        container().m_entries.at(index()).subspaceIndexOffset};
  }

  /// @brief Get the parameters of the measurement
  /// @return The parameters
  ParametersVectorMap parameters() const {
    return ParametersVectorMap{
        container().m_parameters.data() +
        container().m_entries.at(index()).parameterOffset};
  }

  /// @brief Get the covariance of the measurement
  /// @return The covariance
  CovarianceMatrixMap covariance() const {
    return CovarianceMatrixMap{
        container().m_covariances.data() +
        container().m_entries.at(index()).covarianceOffset};
  }
};

/// @brief Variable-size measurement proxy
///
/// This class provides access to a variable-size measurement in a measurement
/// container.
///
/// @tparam FullSize The full size of the measurement
/// @tparam ReadOnly Whether the proxy is read-only
template <std::size_t FullSize, bool ReadOnly>
class VariableMeasurementProxy
    : public MeasurementProxyBase<VariableMeasurementProxy<FullSize, ReadOnly>,
                                  FullSize, ReadOnly> {
 public:
  using Base =
      MeasurementProxyBase<VariableMeasurementProxy<FullSize, ReadOnly>,
                           FullSize, ReadOnly>;
  using Index = typename Base::Index;
  using SubspaceIndex = typename Base::SubspaceIndex;
  using Scalar = typename Base::Scalar;
  using Container = typename Base::Container;

  using SubspaceHelper = Acts::VariableSubspaceHelper<FullSize>;

  using SubspaceVector = Eigen::Matrix<SubspaceIndex, Eigen::Dynamic, 1>;
  using SubspaceVectorMap =
      std::conditional_t<ReadOnly, Eigen::Map<const SubspaceVector>,
                         Eigen::Map<SubspaceVector>>;

  using ParametersVector = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>;
  using ParametersVectorMap =
      std::conditional_t<ReadOnly, Eigen::Map<const ParametersVector>,
                         Eigen::Map<ParametersVector>>;

  using CovarianceMatrix =
      Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>;
  using CovarianceMatrixMap =
      std::conditional_t<ReadOnly, Eigen::Map<const CovarianceMatrix>,
                         Eigen::Map<CovarianceMatrix>>;

  VariableMeasurementProxy(Container& container_, Index index_)
      : Base(container_, index_) {}
  template <typename OtherDerived, bool OtherReadOnly>
  VariableMeasurementProxy(
      const MeasurementProxyBase<OtherDerived, FullSize, OtherReadOnly>& other)
    requires(ReadOnly == OtherReadOnly || ReadOnly)
      : Base(other) {}

  using Base::container;
  using Base::index;

  /// @brief Get the subspace helper for the measurement
  /// @return The subspace helper
  SubspaceHelper subspaceHelper() const {
    return SubspaceHelper{subspaceIndexVector()};
  }

  /// @brief Get the subspace indices of the measurement
  /// @return The subspace indices
  SubspaceVectorMap subspaceIndexVector() const {
    const auto size = static_cast<Eigen::Index>(this->size());
    return SubspaceVectorMap{
        container().m_subspaceIndices.data() +
            container().m_entries.at(index()).subspaceIndexOffset,
        size};
  }

  /// @brief Get the parameters of the measurement
  /// @return The parameters
  ParametersVectorMap parameters() const {
    const auto size = static_cast<Eigen::Index>(this->size());
    return ParametersVectorMap{
        container().m_parameters.data() +
            container().m_entries.at(index()).parameterOffset,
        size};
  }

  /// @brief Get the covariance of the measurement
  /// @return The covariance
  CovarianceMatrixMap covariance() const {
    const auto size = static_cast<Eigen::Index>(this->size());
    return CovarianceMatrixMap{
        container().m_covariances.data() +
            container().m_entries.at(index()).covarianceOffset,
        size, size};
  }
};

template <typename... Args>
MeasurementContainer::VariableProxy MeasurementContainer::emplaceMeasurement(
    std::uint8_t size, Args&&... args) {
  VariableProxy meas = makeMeasurement(size);

  meas.fill(std::forward<Args>(args)...);

  return meas;
}

template <std::size_t Size, typename... Args>
MeasurementContainer::FixedProxy<Size> MeasurementContainer::emplaceMeasurement(
    Args&&... args) {
  FixedProxy<Size> meas = makeMeasurement<Size>();

  meas.fill(std::forward<Args>(args)...);

  return meas;
}

}  // namespace ActsExamples
