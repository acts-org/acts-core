// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/Types.hpp"
#include "Acts/Utilities/AlgebraHelpers.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <bitset>
#include <span>

#include <boost/container/static_vector.hpp>

namespace Acts {

template <typename Container>
inline static bool checkSubspaceIndices(const Container& container,
                                        std::size_t fullSize,
                                        std::size_t subspaceSize) {
  if (subspaceSize > fullSize) {
    return false;
  }
  if (container.size() != subspaceSize) {
    return false;
  }
  for (auto it = container.begin(); it != container.end();) {
    auto index = *it;
    if (index >= fullSize) {
      return false;
    }
    ++it;
    if (std::find(it, container.end(), index) != container.end()) {
      return false;
    }
  }
  return true;
}

template <typename Derived, std::size_t FullSize>
class SubspaceHelperBase {
 public:
  static constexpr std::size_t kFullSize = FullSize;

  using FullSquareMatrix = ActsSquareMatrix<kFullSize>;

  bool empty() const { return self().empty(); }
  std::size_t size() const { return self().size(); }

  auto operator[](std::size_t i) const { return self()[i]; }

  auto begin() const { return self().begin(); }
  auto end() const { return self().end(); }

  FullSquareMatrix fullProjector() const {
    FullSquareMatrix result = FullSquareMatrix::Zero();
    for (auto [i, index] : enumerate(*this)) {
      result(i, index) = 1;
    }
    return result;
  }

  FullSquareMatrix fullExpander() const {
    FullSquareMatrix result = FullSquareMatrix::Zero();
    for (auto [i, index] : enumerate(*this)) {
      result(index, i) = 1;
    }
    return result;
  }

  ProjectorBitset projectorBitset() const
    requires(kFullSize <= 8)
  {
    return matrixToBitset(fullProjector()).to_ullong();
  }

 private:
  const Derived& self() const { return static_cast<const Derived&>(*this); }
};

template <std::size_t FullSize, typename index_t = std::uint8_t>
class VariableSubspaceHelper
    : public SubspaceHelperBase<VariableSubspaceHelper<FullSize, index_t>,
                                FullSize> {
 public:
  static constexpr std::size_t kFullSize = FullSize;

  using IndexType = index_t;
  using Container = boost::container::static_vector<IndexType, FullSize>;

  template <typename OtherContainer>
  explicit VariableSubspaceHelper(const OtherContainer& indices) {
    assert(checkSubspaceIndices(indices, kFullSize, indices.size()) &&
           "Invalid indices");
    m_indices.resize(indices.size());
    std::transform(indices.begin(), indices.end(), m_indices.begin(),
                   [](auto index) { return static_cast<IndexType>(index); });
  }

  bool empty() const { return m_indices.empty(); }
  std::size_t size() const { return m_indices.size(); }

  IndexType operator[](std::size_t i) const { return m_indices[i]; }

  auto begin() const { return m_indices.begin(); }
  auto end() const { return m_indices.end(); }

 private:
  Container m_indices;
};

template <std::size_t FullSize, std::size_t SubspaceSize,
          typename index_t = std::uint8_t>
class FixedSubspaceHelper
    : public SubspaceHelperBase<
          FixedSubspaceHelper<FullSize, SubspaceSize, index_t>, FullSize> {
 public:
  static constexpr std::size_t kFullSize = FullSize;
  static constexpr std::size_t kSubspaceSize = SubspaceSize;
  static_assert(kSubspaceSize <= kFullSize, "Invalid subspace size");

  using Projector = ActsMatrix<kSubspaceSize, kFullSize>;
  using Expander = ActsMatrix<kFullSize, kSubspaceSize>;
  using Vector = ActsVector<kSubspaceSize>;
  using SquareMatrix = ActsSquareMatrix<kSubspaceSize>;
  template <std::size_t K>
  using ApplyLeftResult = ActsMatrix<kSubspaceSize, kSubspaceSize>;
  template <std::size_t N>
  using ApplyRightResult = ActsMatrix<kSubspaceSize, kSubspaceSize>;

  using IndexType = index_t;
  using Container = std::array<IndexType, kSubspaceSize>;

  template <typename OtherContainer>
  explicit FixedSubspaceHelper(const OtherContainer& indices) {
    assert(checkSubspaceIndices(indices, kFullSize, kSubspaceSize) &&
           "Invalid indices");
    std::transform(indices.begin(), indices.end(), m_indices.begin(),
                   [](auto index) { return static_cast<IndexType>(index); });
  }

  bool empty() const { return m_indices.empty(); }
  std::size_t size() const { return m_indices.size(); }

  IndexType operator[](std::uint32_t i) const { return m_indices[i]; }

  auto begin() const { return m_indices.begin(); }
  auto end() const { return m_indices.end(); }

  Projector projector() const {
    Projector result = Projector::Zero();
    for (auto [i, index] : enumerate(*this)) {
      result(i, index) = 1;
    }
    return result;
  }

  Expander expander() const {
    Expander result = Expander::Zero();
    for (auto [i, index] : enumerate(*this)) {
      result(index, i) = 1;
    }
    return result;
  }

  template <std::size_t K, typename Derived>
  ApplyLeftResult<K> applyLeftOf(
      const Eigen::DenseBase<Derived>& matrix) const {
    assert(matrix.rows() == kFullSize && "Invalid matrix size");
    ApplyLeftResult<K> result = ApplyLeftResult<K>::Zero();
    for (auto [i, indexI] : enumerate(*this)) {
      for (std::size_t j = 0; j < K; ++j) {
        result(i, j) = matrix(indexI, j);
      }
    }
    return result;
  }

  template <std::size_t N, typename Derived>
  ApplyRightResult<N> applyRightOf(
      const Eigen::DenseBase<Derived>& matrix) const {
    assert(matrix.cols() == kSubspaceSize && "Invalid matrix size");
    ApplyRightResult<N> result = ApplyRightResult<N>::Zero();
    for (std::size_t i = 0; i < N; ++i) {
      for (auto [j, indexJ] : enumerate(*this)) {
        result(i, j) = matrix(i, indexJ);
      }
    }
    return result;
  }

  template <typename Derived>
  Vector projectVector(const Eigen::DenseBase<Derived>& fullVector) const {
    assert(fullVector.size() == kFullSize && "Invalid full vector size");
    Vector result = Vector::Zero();
    for (auto [i, index] : enumerate(*this)) {
      result(i) = fullVector(index);
    }
    return result;
  }

  template <typename Derived>
  SquareMatrix projectMatrix(
      const Eigen::DenseBase<Derived>& fullMatrix) const {
    assert(fullMatrix.rows() == kFullSize && fullMatrix.cols() == kFullSize &&
           "Invalid full matrix size");
    SquareMatrix result = SquareMatrix::Zero();
    for (auto [i, indexI] : enumerate(*this)) {
      for (auto [j, indexJ] : enumerate(*this)) {
        result(i, j) = fullMatrix(indexI, indexJ);
      }
    }
    return result;
  }

 private:
  Container m_indices{};
};

template <std::size_t SubspaceSize>
using FixedBoundSubspaceHelper =
    FixedSubspaceHelper<Acts::eBoundSize, SubspaceSize, std::uint8_t>;
using VariableBoundSubspaceHelper =
    VariableSubspaceHelper<Acts::eBoundSize, std::uint8_t>;

template <std::size_t kFullSize, typename Derived>
SubspaceIndices<kFullSize> projectorToSubspaceIndices(
    const Eigen::DenseBase<Derived>& projector) {
  auto rows = static_cast<std::size_t>(projector.rows());
  auto cols = static_cast<std::size_t>(projector.cols());
  assert(cols == kFullSize && rows <= kFullSize && "Invalid projector size");
  SubspaceIndices<kFullSize> result;
  result.fill(kFullSize);
  for (std::size_t i = 0; i < rows; ++i) {
    for (std::size_t j = 0; j < cols; ++j) {
      assert((projector(i, j) == 0 || projector(i, j) == 1) &&
             "Invalid projector value");
      if (projector(i, j) == 1) {
        result[i] = j;
      }
    }
  }
  return result;
}

}  // namespace Acts
