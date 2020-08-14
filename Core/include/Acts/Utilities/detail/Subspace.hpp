// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include <Eigen/Core>

namespace Acts {
namespace detail {

/// @defgroup subspace Linear subspace definitions
///
/// All types in this group define a linear subspace of a larger vector space
/// with the following properties: the subspace is defined by a set of axis
/// indices in the full space, i.e. there is no need for a non-trivial
/// projection matrix, all subspace axis indices are unique and there are no
/// duplicated ones, and the set of axis indices must be ordered, i.e. the axis
/// order in the subspace is the same as in the full space. The last requirement
/// is not strictly required by the implementation, but is still added to
/// simplify reasoning.
///
/// Only the size of the subspace are defined by the types at compile time.
/// Which axes comprise the subspace is defined at runtime. This allows to use
/// fixed-size computations (selected at compile time) but avoids the
/// combinatorial explosion of also having to handle all possible combination of
/// axes at compile time. This was tried previously and resulted in sizable
/// resource consumption at compile time without runtime benefits.
///
/// All types are intentionally using `size_t` as their template values, instead
/// of the more specific index enums, to reduce the number of templates. This is
/// fully compatible as the index enums are required to be convertible to
/// `size_t`.
///
/// All types intentionally only define the subspace but not how vectors
/// and matrices are stored to avoid unnecessary coupling between components,
/// i.e here between the pure definition and the storage.
///
/// All types provide `.projectVector(...)` and `.exandVector(...)` methods to
/// convert to/from the subspace. They also provide `.projector()` and
/// `.expander()` methods to create projection and expansion matrices if they
/// are required explicitely. For the specific subspace requirements listed
/// above, the projection and expansion matrices are transpose to each other. In
/// the general case, this does not have to be the case and users are encouraged
/// to use `.projector()` and `.expander()` instead of e.g.
/// `.projector().transpose()`.
///
/// @{

/// Fixed-size subspace representation.
///
/// @tparam kFullSize Size of the full vector space
/// @tparam kSize Size of the subspace
template <size_t kFullSize, size_t kSize>
class FixedSizeSubspace {
  static_assert(kFullSize <= static_cast<size_t>(UINT8_MAX),
                "Full vector space size is larger than the supported range");
  static_assert(1u <= kSize, "Subspace size must be at least 1");
  static_assert(kSize <= kFullSize,
                "Subspace can only be as large as the full space");

  template <typename source_t>
  using SubspaceVectorFor = Eigen::Matrix<typename source_t::Scalar, kSize, 1>;
  template <typename source_t>
  using FullspaceVectorFor =
      Eigen::Matrix<typename source_t::Scalar, kFullSize, 1>;
  template <typename scalar_t>
  using ProjectionMatrix = Eigen::Matrix<scalar_t, kSize, kFullSize>;
  template <typename scalar_t>
  using ExpansionMatrix = Eigen::Matrix<scalar_t, kFullSize, kSize>;

  // the functionality could also be implemented using a std::bitset where each
  // bit corresponds to an axis in the fullspace and set bits indicate which
  // bits make up the subspace. this would be a more compact representation but
  // complicates the implementation since we can not easily iterate over the
  // indices of the subspace. storing the subspace indices directly requires a
  // bit more memory but is easier to work with. for our typical use cases with
  // n<=8, this still takes only 64bit of memory.
  std::array<uint8_t, kSize> m_axes;

 public:
  /// Construct from an array of axis indices.
  ///
  /// @tparam index_t Scalar index type
  /// @param indices Ordered array of unique axis indices
  template <typename index_t>
  constexpr FixedSizeSubspace(const std::array<index_t, kSize>& indices) {
    // verify inputs
    for (auto i = 0u; i < kSize; ++i) {
      assert((indices[i] < kFullSize) and
             "Axis indices must be within the full space");
      if ((i + 1u) < kSize) {
        assert((indices[i] < indices[i + 1u]) and
               "Axis indices must be strongly ordered and unique");
      }
    }
    // assign stored axis
    for (auto i = 0u; i < kSize; ++i) {
      m_axes[i] = static_cast<uint8_t>(indices[i]);
    }
  }
  // The subset can not be constructed w/o defining its axis indices.
  FixedSizeSubspace() = delete;
  FixedSizeSubspace(const FixedSizeSubspace&) = default;
  FixedSizeSubspace(FixedSizeSubspace&&) = default;
  FixedSizeSubspace& operator=(const FixedSizeSubspace&) = default;
  FixedSizeSubspace& operator=(FixedSizeSubspace&&) = default;

  /// Size of the subspace.
  static constexpr size_t size() { return kSize; }
  /// Size of the full vector space.
  static constexpr size_t fullSize() { return kFullSize; }

  /// Axis indices that comprise the subspace.
  ///
  /// The specific container and index type should be considered an
  /// implementation detail. Users should treat the return type as a generic
  /// container whose elements are convertible to `size_t`.
  constexpr const std::array<uint8_t, kSize>& indices() const { return m_axes; }

  /// Check if the given axis index in the full space is part of the subspace.
  constexpr bool contains(size_t index) const {
    bool isContained = false;
    for (auto a : m_axes) {
      isContained |= (a == index);
    }
    return isContained;
  }

  /// Project a full vector into the subspace.
  ///
  /// @tparam fullspace_vector_t Veector type in the full space
  /// @param full Vector in the full space
  /// @return Subspace vector w/ just the configured axis components
  ///
  /// @note Always returns a column vector regardless of the input
  template <typename fullspace_vector_t>
  auto projectVector(const Eigen::MatrixBase<fullspace_vector_t>& full) const
      -> SubspaceVectorFor<fullspace_vector_t> {
    EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(fullspace_vector_t, kFullSize);

    SubspaceVectorFor<fullspace_vector_t> sub;
    for (auto i = 0u; i < kSize; ++i) {
      sub[i] = full[m_axes[i]];
    }
    return sub;
  }

  /// Expand a subspace vector into the full space.
  ///
  /// @tparam subspace_vector_t Subspace vector type
  /// @param sub Subspace vector
  /// @return Vector in the full space w/ zeros for non-subspace components
  ///
  /// @note Always returns a column vector regardless of the input
  template <typename subspace_vector_t>
  auto expandVector(const Eigen::MatrixBase<subspace_vector_t>& sub) const
      -> FullspaceVectorFor<subspace_vector_t> {
    EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(subspace_vector_t, kSize);

    FullspaceVectorFor<subspace_vector_t> full;
    full.setZero();
    for (auto i = 0u; i < kSize; ++i) {
      full[m_axes[i]] = sub[i];
    }
    return full;
  }

  /// Projection matrix that maps from the full space into the subspace.
  ///
  /// @tparam scalar_t Scalar type for the projection matrix
  template <typename scalar_t>
  auto projector() const -> ProjectionMatrix<scalar_t> {
    ProjectionMatrix<scalar_t> proj;
    proj.setZero();
    for (auto i = 0u; i < kSize; ++i) {
      proj(i, m_axes[i]) = 1;
    }
    return proj;
  }

  /// Expansion matrix that maps from the subspace into the full space.
  ///
  /// @tparam scalar_t Scalar type of the generated expansion matrix
  template <typename scalar_t>
  auto expander() const -> ExpansionMatrix<scalar_t> {
    ExpansionMatrix<scalar_t> expn;
    expn.setZero();
    for (auto i = 0u; i < kSize; ++i) {
      expn(m_axes[i], i) = 1;
    }
    return expn;
  }
};

/// Variable-size subspace representation.
///
/// @tparam kFullSize Size of the full vector space
/// @tparam kMaxSize Maximum size of the subspace
template <size_t kFullSize, size_t kMaxSize = kFullSize>
class VariableSizeSubspace {
  // must be strictly less than UINT8_MAX since UINT8_MAX is used as a special
  // value to indicate the dynamic size
  static_assert(kFullSize < static_cast<size_t>(UINT8_MAX),
                "Full vector space size is larger than the supported range");
  static_assert(kMaxSize <= kFullSize,
                "Maximum subspace can only be as large as the full space");

  template <typename source_t>
  using SubspaceVectorFor =
      Eigen::Matrix<typename source_t::Scalar, Eigen::Dynamic, 1,
                    Eigen::AutoAlign, kMaxSize, 1>;
  template <typename source_t>
  using FullspaceVectorFor =
      Eigen::Matrix<typename source_t::Scalar, kFullSize, 1>;
  template <typename scalar_t>
  using ExpansionMatrix = Eigen::Matrix<scalar_t, kFullSize, Eigen::Dynamic,
                                        Eigen::AutoAlign, kFullSize, kMaxSize>;
  template <typename scalar_t>
  using ProjectionMatrix = Eigen::Matrix<scalar_t, Eigen::Dynamic, kFullSize,
                                         Eigen::AutoAlign, kMaxSize, kFullSize>;

  // see FixedSizeSubspace for details on the implementation.
  std::array<uint8_t, kMaxSize> m_axes;

 public:
  /// Construct from a sequence of axis indices.
  ///
  /// @tparam index_iterator_t Forward iterator type dereferencable to an index
  /// @param beg Iterator to the beginning of a sequence of indices
  /// @param end Iterator to the end of a sequence of indices
  template <typename index_iterator_t>
  constexpr VariableSizeSubspace(index_iterator_t beg, index_iterator_t end) {
    // verify inputs
    assert((std::distance(beg, end) < static_cast<ssize_t>(kMaxSize)) and
           "The number of input axis indices must be less or equal to the "
           "maximum subspace size");
    for (auto cur = beg; cur != end; ++cur) {
      // this also checks *cur < UINT8_MAX since kFullSize must < UINT8_MAX
      assert((*cur < kFullSize) and
             "Axis indices must be within the full space");
      if (std::next(cur) != end) {
        assert((*cur < *std::next(cur)) and
               "Axis indices must be strongly ordered and unique");
      }
    }
    // assign input axis
    auto i = 0u;
    for (auto cur = beg; cur != end; ++cur, ++i) {
      m_axes[i] = static_cast<uint8_t>(*cur);
    }
    // invalidate unset axis
    for (; i < kMaxSize; ++i) {
      m_axes[i] = UINT8_MAX;
    }
  }
  // The subspace can not be constructed w/o defining its axis indices.
  VariableSizeSubspace() = delete;
  VariableSizeSubspace(const VariableSizeSubspace&) = default;
  VariableSizeSubspace(VariableSizeSubspace&&) = default;
  VariableSizeSubspace& operator=(const VariableSizeSubspace&) = default;
  VariableSizeSubspace& operator=(VariableSizeSubspace&&) = default;

  /// Size of the subspace.
  constexpr size_t size() const {
    size_t n = 0u;
    for (auto a : m_axes) {
      n += (a != UINT8_MAX);
    }
    return n;
  }
  /// Maximum size of the subspace.
  static constexpr size_t maxSize() { return kMaxSize; }
  /// Size of the full vector space.
  static constexpr size_t fullSize() { return kFullSize; }

  // TODO add .indices() accessor. the size is variable at runtime, but bounded.
  //   could be implemented by using something like `span<..>` on the stored
  //   axis indices, or `Range` from the examples.

  /// Check if the given axis index in the full space is part of the subspace.
  constexpr bool contains(size_t index) const {
    bool isContained = false;
    for (auto a : m_axes) {
      isContained |= ((a != UINT8_MAX) and (a == index));
    }
    return isContained;
  }

  /// Project a full vector into the subspace.
  ///
  /// @tparam fullspace_vector_t Veector type in the full space
  /// @param full Vector in the full space
  /// @return Subspace vector w/ just the configured axis components
  ///
  /// @note Always returns a column vector regardless of the input
  template <typename fullspace_vector_t>
  auto projectVector(const Eigen::MatrixBase<fullspace_vector_t>& full) const
      -> SubspaceVectorFor<fullspace_vector_t> {
    EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(fullspace_vector_t, kFullSize);

    SubspaceVectorFor<fullspace_vector_t> sub(size());
    for (auto i = 0u; (i < kFullSize) and (m_axes[i] != UINT8_MAX); ++i) {
      sub[i] = full[m_axes[i]];
    }
    return sub;
  }

  /// Expand a subspace vector into the full space.
  ///
  /// @tparam subspace_t Subspace vector type
  /// @param sub Subspace vector
  /// @return Vector in the full space w/ zeros for non-subspace components
  ///
  /// @note Always returns a column vector regardless of the input
  template <typename subspace_vector_t>
  auto expandVector(const Eigen::MatrixBase<subspace_vector_t>& sub) const
      -> FullspaceVectorFor<subspace_vector_t> {
    EIGEN_STATIC_ASSERT_VECTOR_ONLY(subspace_vector_t);

    FullspaceVectorFor<subspace_vector_t> full;
    full.setZero();
    for (auto i = 0u; (i < kFullSize) and (m_axes[i] != UINT8_MAX); ++i) {
      full[m_axes[i]] = sub[i];
    }
    return full;
  }

  /// Projection matrix that maps from the full space into the subspace.
  ///
  /// @tparam scalar_t Scalar type of the generated expansion matrix
  template <typename scalar_t>
  auto projector() const -> ProjectionMatrix<scalar_t> {
    ProjectionMatrix<scalar_t> proj(size(), kFullSize);
    proj.setZero();
    for (auto i = 0u; (i < kFullSize) and (m_axes[i] != UINT8_MAX); ++i) {
      proj(i, m_axes[i]) = 1;
    }
    return proj;
  }

  /// Expansion matrix that maps from the subspace into the full space.
  ///
  /// @tparam scalar_t Scalar type of the generated expansion matrix
  template <typename scalar_t>
  auto expander() const -> ExpansionMatrix<scalar_t> {
    ExpansionMatrix<scalar_t> expn(kFullSize, size());
    expn.setZero();
    for (auto i = 0u; (i < kFullSize) and (m_axes[i] != UINT8_MAX); ++i) {
      expn(m_axes[i], i) = 1;
    }
    return expn;
  }
};

/// @}

}  // namespace detail
}  // namespace Acts
