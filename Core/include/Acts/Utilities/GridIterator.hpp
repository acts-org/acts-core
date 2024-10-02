// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/Holders.hpp"

#include <array>

namespace Acts {

/// @class GridGlobalIterator
/// Grid iterator using the global position. This iterates on all
/// the bins in the grid, including under- and over-flows
/// @tparam T The type stored in the grid bins
/// @tparam Axes ... The types of the axes in the grid
template <typename T, class... Axes>
class GridGlobalIterator {
 public:
  static constexpr std::size_t DIM = sizeof...(Axes);

  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;

  /// @brief Default constructor
  GridGlobalIterator() = default;
  /// @brief Constructor taking ownership of the grid is not allowed
  /// @param [in] grid The grid
  /// @param [in] idx The global bin
  GridGlobalIterator(Acts::Grid<T, Axes...>&& grid, std::size_t idx) = delete;
  /// @brief Constructor not taking ownership of the grid
  /// @param [in] grid The grid
  /// @param [in] idx The global bin
  ///
  /// @pre Global bin index must be a valid index for the grid
  GridGlobalIterator(const Acts::Grid<T, Axes...>& grid, std::size_t idx = 0ul);

  /// @brief Copy constructor
  /// @param [in] other The GlobalBinIterator to be copied
  GridGlobalIterator(const GridGlobalIterator<T, Axes...>& other) = default;
  /// @brief Copy assignment
  /// @param [in] other The GlobalBinIterator to be copied
  /// @return The new global bin iterator
  GridGlobalIterator<T, Axes...>& operator=(
      const GridGlobalIterator<T, Axes...>& other) = default;

  /// @brief Move constructor
  /// @param [in] other The GlobalBinIterator to be moved
  ///
  /// This will invalidate the other GlobalBinIterator
  GridGlobalIterator(GridGlobalIterator<T, Axes...>&& other) noexcept;
  /// @brief Move assignment
  /// @param [in] other The GlobalBinIterator to be moved
  /// @return The new global bin iterator
  ///
  /// This will invalidate the other GlobalBinIterator
  GridGlobalIterator<T, Axes...>& operator=(
      GridGlobalIterator<T, Axes...>&& other) noexcept;

  /// @brief Default destructor
  ~GridGlobalIterator() = default;

  /// @brief Equality operator
  /// @param [in] other The other GridGlobalIterator to be compared against this one
  /// @return The result of the comparison
  bool operator==(const GridGlobalIterator<T, Axes...>& other) const;
  /// @brief Comparison (<=>) operator
  /// @param [in] other The other GridGlobalIterator to be compared against this one
  /// @return The result of the comparison
  auto operator<=>(const GridGlobalIterator<T, Axes...>& other) const;

  /// @brief Increment this iterator with an offset
  /// @param [in] offset The increment value
  /// @return The incremented iterator
  GridGlobalIterator<T, Axes...>& operator+=(const std::size_t offset);
  /// @brief Decrement this iterator with an offset
  /// @param [in] offset The decrement value
  /// @return The decremented iterator
  GridGlobalIterator<T, Axes...>& operator-=(const std::size_t offset);
  /// @brief Create incremented iterator
  /// @param [in] offset The increment value
  /// @return The incremented iterator
  GridGlobalIterator<T, Axes...> operator+(const std::size_t offset) const;
  /// @brief Create decremented iterator
  /// @param [in] offset The decrement value
  /// @return The decremented iterator
  GridGlobalIterator<T, Axes...> operator-(const std::size_t offset) const;

  /// @brief Distance between two GridGlobalIterators
  /// @param [in] other The other GridGlobalIterator
  /// @return The distance between the two iterators
  ///
  /// This will compute the distance by comparing the global positions in the
  /// two iterators
  ///
  /// @pre The two iterators must have the same grid
  difference_type operator-(const GridGlobalIterator<T, Axes...>& other) const;
  /// @brief Return stored value at given global position
  /// @return The stored value in the grid from that given global position
  const value_type& operator*() const;

  /// @brief Increment operator (pre)
  /// @return The global iterator after the increment
  ///
  /// This will increase the global position by one
  GridGlobalIterator<T, Axes...>& operator++();
  /// @brief Increment operator (post)
  /// @return The global iterator before the increment
  ///
  /// This will increase the global position by one
  GridGlobalIterator<T, Axes...> operator++(int);

  /// @brief Retrieve the global bin index
  /// @return The current global bin index in the grid
  std::size_t globalBinIndex() const;
  /// @brief Retrieve the local bins indices
  /// @return The current local bins indexed in the grid
  std::array<std::size_t, DIM> localBinsIndices() const;

 private:
  /// @brief The grid on which we are iterating
  ///
  /// The iterator never takes ownership of the grid. If the grid gets
  /// invalidated (e.g. in a move operation) we can get undefined behaviours
  /// if the iterator gets used after being invalidated
  Acts::detail::RefHolder<const Acts::Grid<T, Axes...>> m_grid{nullptr};
  /// @brief The iteration index, corresponding to the global bin in the grid
  std::size_t m_idx{0ul};
};

/// @class GridLocalIterator
/// Grid iterator using the local position. This iterates on all
/// local bins in the grid, and can exclude under- and over-flows
/// Can also allow for custom navigation pattern along axes
/// @tparam T The type stored in the grid bins
/// @tparam Axes ... The types of the axes in the grid
template <typename T, class... Axes>
class GridLocalIterator {
 public:
  static constexpr std::size_t DIM = sizeof...(Axes);

  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;

  /// @brief Default constructor
  GridLocalIterator() = default;
  /// @brief Constructor taking ownership of the grid is not allowed
  /// @param [in] grid The grid
  /// @param [in] indices The local position
  GridLocalIterator(Acts::Grid<T, Axes...>&& grid,
                    const std::array<std::size_t, DIM>& indices) = delete;
  /// @brief Constructor taking ownership of the grid is not allowed
  /// @param [in] grid The grid
  /// @param [in] indices The local position
  /// @param [in] navigation The custom navigation pattern for each axis
  ///
  /// @pre None of the navigation vectors is allowed to be an empty vector
  GridLocalIterator(Acts::Grid<T, Axes...>&& grid,
                    const std::array<std::size_t, DIM>& indices,
                    std::array<std::vector<std::size_t>, DIM> navigation) =
      delete;
  /// @brief Constructor
  /// @param [in] grid The grid
  /// @param [in] indices The local position
  ///
  /// @pre The local bins must be a valid local position in the grid
  GridLocalIterator(const Acts::Grid<T, Axes...>& grid,
                    const std::array<std::size_t, DIM>& indices);
  /// @brief Constructor with custom navigation pattern
  /// @param [in] grid The grid
  /// @param [in] indices The local position
  /// @param [in] navigation The custom navigation pattern for each axis
  ///
  /// @pre The local bins must be a valid local position in the grid.
  /// The navigation pattern must be consistent with the local bins (i.e. size
  /// <= num bins in the axis) in the grid and have no repetitions.
  ///
  /// @pre None of the navigation vectors is allowed to be an empty vector
  GridLocalIterator(const Acts::Grid<T, Axes...>& grid,
                    const std::array<std::size_t, DIM>& indices,
                    std::array<std::vector<std::size_t>, DIM> navigation);

  /// @brief Copy constructor
  /// @param [in] other The GridLocalIterator to be copied
  GridLocalIterator(const GridLocalIterator<T, Axes...>& other) = default;
  /// @brief Copy assignment operator
  /// @param [in] other The GridLocalIterator to be copied
  /// @return The copied GridLocalIterator
  GridLocalIterator<T, Axes...>& operator=(
      const GridLocalIterator<T, Axes...>& other) = default;

  /// @brief Move constructor
  /// @param [in] other The GridLocalIterator to be moved
  ///
  /// This will invalidate the other GridLocalIterator
  GridLocalIterator(GridLocalIterator<T, Axes...>&& other) noexcept;
  /// @brief Move assignment operator
  /// @param [in] other The GridLocalIterator to be moved
  /// @return The moved GridLocalIterator
  ///
  /// This will invalidate the other GridLocalIterator
  GridLocalIterator<T, Axes...>& operator=(
      GridLocalIterator<T, Axes...>&& other) noexcept;

  /// @brief Default destructor
  ~GridLocalIterator() = default;

  /// @brief Equality operator
  /// @param [in] other The other GridLocalIterator to be compared against this one
  /// @return The result of the comparison
  bool operator==(const Acts::GridLocalIterator<T, Axes...>& other) const;

  /// @brief Return stored value at given local position
  /// @return The stored value in the grid from that given local position
  const value_type& operator*() const;

  /// @brief Increment operator (pre)
  /// @return The local iterator after the increment
  ///
  /// This will increase the local position by one
  GridLocalIterator<T, Axes...>& operator++();
  /// @brief Increment operator (post)
  /// @return The local iterator before the increment
  ///
  /// This will increase the local position by one
  GridLocalIterator<T, Axes...> operator++(int);

  /// @brief Retrieve the global bin index
  /// @return The current global bin index in the grid
  std::size_t globalBinIndex() const;
  /// @brief Retrieve the local position
  /// @return The current local position in the grid
  std::array<std::size_t, DIM> localBinsIndices() const;

 private:
  /// @brief Increment the local position
  /// @tparam N Current dimension
  template <std::size_t N>
  void increment();

 private:
  /// @brief The grid on which we are iterating
  ///
  /// The iterator never takes ownership of the grid. If the grid gets
  /// invalidated (e.g. in a move operation) we can get undefined behaviours
  /// if the iterator gets used after being invalidated
  Acts::detail::RefHolder<const Acts::Grid<T, Axes...>> m_grid{nullptr};
  /// @brief The maximum number of local bins in the grid. This does not include
  /// under- and over-flow bins
  std::array<std::size_t, DIM> m_numLocalBins{};
  /// @brief The current iteration position.
  ///
  /// This represent the position in the navigation pattern.
  /// For each axis, the current index goes from 0ul to the size of the
  /// corresponding navigation
  ///
  /// The local position in the gris is then obtained, for each axis i,
  /// via m_navigationIndex[m_currentIndex[i]]
  std::array<std::size_t, DIM> m_currentIndex{};
  /// @brief The custom navigation pattern in the grid
  ///
  /// This allows users to define any custom iteration sequence in all the
  /// different axes of the grid. If nothing is defined by the user, then
  /// a std::iota is used as the default starting with the 1ul bin (0ul) is
  /// the under-flow in the axis
  std::array<std::vector<std::size_t>, DIM> m_navigationIndex{};
};

template <typename T, class... Axes>
GridGlobalIterator(const Acts::Grid<T, Axes...>& grid,
                   std::size_t idx) -> GridGlobalIterator<T, Axes...>;

}  // namespace Acts

#include "Acts/Utilities/GridIterator.ipp"
