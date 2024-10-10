// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Seeding/BinnedGroupIterator.hpp"
#include "Acts/Utilities/GridBinFinder.hpp"
#include "Acts/Utilities/GridIterator.hpp"
#include "Acts/Utilities/Holders.hpp"

#include <memory>
#include <vector>

namespace Acts {

/// @class BinnedGroup
/// @tparam grid_t Type of the grid the group owns
///
/// The assumption is that the grid has ownership of the space points and
/// that the grid value_type (i.e. T in Grid<T, Axes ...>) is an iterable
/// object of space points, such as a vector< ... >
template <typename grid_t>
class BinnedGroup {
 public:
  friend BinnedGroupIterator<grid_t>;

  static constexpr std::size_t DIM = grid_t::DIM;

  /// @brief Default constructor
  BinnedGroup() = delete;

  /// brief Constructor
  BinnedGroup(grid_t&& grid, const Acts::GridBinFinder<DIM>& bottomFinder,
              const Acts::GridBinFinder<DIM>& topFinder,
              std::array<std::vector<std::size_t>, DIM> navigation =
                  std::array<std::vector<std::size_t>, DIM>());

  BinnedGroup(grid_t&& grid, std::vector<bool> mask,
              const Acts::GridBinFinder<DIM>& bottomFinder,
              const Acts::GridBinFinder<DIM>& topFinder,
              std::array<std::vector<std::size_t>, DIM> navigation =
                  std::array<std::vector<std::size_t>, DIM>());

  BinnedGroup(grid_t& grid, const Acts::GridBinFinder<DIM>& bottomFinder,
              const Acts::GridBinFinder<DIM>& topFinder,
              std::array<std::vector<std::size_t>, DIM> navigation =
                  std::array<std::vector<std::size_t>, DIM>()) = delete;

  /// @brief Copy constructor
  /// @param [in] other The BinnedGroup to copy
  BinnedGroup(const BinnedGroup<grid_t>& other) = delete;
  /// @brief Copy assignment
  /// @param [in] other The BinnedGroup to copy
  /// @return The copied BinnedGroup
  BinnedGroup<grid_t>& operator=(const BinnedGroup<grid_t>& other) = delete;

  /// @brief Move Constructor
  /// @param [in] other The BinnedGroup to move
  BinnedGroup(BinnedGroup<grid_t>&& other) noexcept = default;
  /// @brief Move Assignment
  /// @param [in] other The BinnedGroup to move
  /// @return The moved BinnedGroup
  BinnedGroup<grid_t>& operator=(BinnedGroup<grid_t>&& other) noexcept =
      default;

  /// @brief Default destructor
  ~BinnedGroup() = default;

  /// @brief Retrieve const reference to the Grid
  /// @return Const reference to the stored grid
  const grid_t& grid() const;
  /// @brief Retrieve mutable reference to the Grid
  /// @return Mutable reference to the stored grid
  grid_t& grid();

  /// @brief Retrieve the mask
  /// Only const accessor is supported
  /// @return The mask
  const std::vector<bool>& mask() const;

  /// @brief Get the begin iterator
  /// @return The iterator
  Acts::BinnedGroupIterator<grid_t> begin() const;
  /// @brief Get the end iterator
  /// @return The iterator
  Acts::BinnedGroupIterator<grid_t> end() const;

 private:
  /// @brief The N-dimentional grid
  grid_t m_grid;
  /// @brief The mask to be applied to the grid. The size of this vector
  /// corresponds to the global bins in the grid
  std::vector<bool> m_mask{};
  /// @brief The Grid Bin Finder for bottom candidates
  const Acts::GridBinFinder<DIM>* m_bottomBinFinder{nullptr};
  /// @brief The Grid Bin Finder for top candidates
  const Acts::GridBinFinder<DIM>* m_topBinFinder{nullptr};
  /// @brief Order of bins to loop over when searching for SPs
  std::array<std::vector<std::size_t>, DIM> m_bins{};
};

}  // namespace Acts

#include "Acts/Seeding/BinnedGroup.ipp"
