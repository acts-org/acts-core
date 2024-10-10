// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/GridIterator.hpp"
#include "Acts/Utilities/Holders.hpp"
#include "Acts/Utilities/detail/grid_helper.hpp"

#include <array>
#include <memory>
#include <tuple>

#include <boost/container/small_vector.hpp>

namespace Acts {
template <typename grid_t>
class BinnedGroup;

template <typename grid_t>
class BinnedGroupIterator {
 public:
  static constexpr std::size_t DIM = grid_t::DIM;

  /// @brief Constructor
  /// Never take the ownership of the group
  ///
  /// @param [in] group The group we are iterating on
  /// @param [in] index Current local position in the grid
  /// @param [in] navigation The navigation pattern in the grid
  BinnedGroupIterator(
      Acts::BinnedGroup<grid_t>&& group, std::array<std::size_t, DIM> index,
      std::array<std::vector<std::size_t>, DIM> navigation) = delete;

  /// @brief Constructor
  /// Never take the ownership of the group
  ///
  /// @param [in] group The group we are iterating on
  /// @param [in] index Current local position in the grid
  /// @param [in] navigation The navigation pattern in the grid
  BinnedGroupIterator(const Acts::BinnedGroup<grid_t>&& group,
                      std::array<std::size_t, DIM> index,
                      std::array<std::vector<std::size_t>, DIM> navigation) =
      delete;

  /// @brief Constructor
  /// @param [in] group The group we are iterating on
  /// @param [in] index Current local position in the grid
  /// @param [in] navigation The navigation pattern in the grid
  BinnedGroupIterator(const Acts::BinnedGroup<grid_t>& group,
                      std::array<std::size_t, DIM> index,
                      std::array<std::vector<std::size_t>, DIM> navigation);

  /// Do not allow Copy operations

  /// @brief Copy Constructor
  /// @param [in] other The BinnedGroupIterator to copy
  BinnedGroupIterator(const BinnedGroupIterator<grid_t>& other) = delete;
  /// @brief Copy assignment
  /// @param [in] other The BinnedGroupIterator to copy
  /// @return The copied BinnedGroupIterator
  BinnedGroupIterator<grid_t>& operator=(
      const BinnedGroupIterator<grid_t>& other) = delete;

  /// @brief Move Constructor
  /// @param [in] other The BinnedGroupIterator to move
  BinnedGroupIterator(BinnedGroupIterator<grid_t>&& other) noexcept = default;
  /// @brief Move assignment
  /// @param [in] other The BinnedGroupIterator to move
  /// @return The moved BinnedGroupIterator
  BinnedGroupIterator<grid_t>& operator=(BinnedGroupIterator&& other) noexcept =
      default;

  /// @brief Default Destructor
  ~BinnedGroupIterator() = default;

  /// @brief Equality operator
  /// @param [in] other The BinnedGroupIterator we are comparing against this one
  /// @return The result of the comparison
  bool operator==(const BinnedGroupIterator<grid_t>& other) const;

  /// @brief Increment the iterator by one (pre)
  /// @return The incremented iterator
  BinnedGroupIterator<grid_t>& operator++();

  /// @brief Return the current bin with the middle candidate, as well as all the
  /// bins with the possible bottom and top candidates
  ///
  /// @return The collection of all the bins in the grid
  std::tuple<boost::container::small_vector<std::size_t,
                                            Acts::detail::ipow(3, grid_t::DIM)>,
             std::size_t,
             boost::container::small_vector<std::size_t,
                                            Acts::detail::ipow(3, grid_t::DIM)>>
  operator*() const;

 private:
  /// @brief Move to the next not-empty bin in the grid
  void findNotEmptyBin();

 private:
  /// @brief The group that contains the grid and the bin finders
  Acts::detail::RefHolder<const Acts::BinnedGroup<grid_t>> m_group{nullptr};
  /// @brief Current N-dimentional grid iterator
  typename grid_t::local_iterator_t m_gridItr;
  /// @brief End iterator;
  typename grid_t::local_iterator_t m_gridItrEnd;
};

}  // namespace Acts

#include "Acts/Seeding/BinnedGroupIterator.ipp"
