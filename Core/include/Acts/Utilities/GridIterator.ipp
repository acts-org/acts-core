// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

namespace Acts {
// Global Iterator
template <typename T, class... Axes>
GridGlobalIterator<T, Axes...>::GridGlobalIterator(
    const Acts::Grid<T, Axes...>& grid, std::size_t idx)
    : m_grid(&grid), m_idx(idx) {}

template <typename T, class... Axes>
GridGlobalIterator<T, Axes...>::GridGlobalIterator(
    GridGlobalIterator<T, Axes...>&& other) noexcept
    : m_grid(std::exchange(other.m_grid.ptr, nullptr)), m_idx(other.m_idx) {}

template <typename T, class... Axes>
GridGlobalIterator<T, Axes...>& GridGlobalIterator<T, Axes...>::operator=(
    GridGlobalIterator<T, Axes...>&& other) noexcept {
  m_grid.ptr = std::exchange(other.m_grid.ptr, nullptr);
  m_idx = other.m_idx;
  return *this;
}

template <typename T, class... Axes>
bool GridGlobalIterator<T, Axes...>::operator==(
    const GridGlobalIterator<T, Axes...>& other) const {
  // This will always return false if we are comparing two iterators from
  // different grids.
  // As such a loop from itrStart (from grid A) to itrStop (from grid B) will
  // never complete since itrStop will not be reachable from itrStart
  return (m_grid.ptr == other.m_grid.ptr) && m_idx == other.m_idx;
}

template <typename T, class... Axes>
auto GridGlobalIterator<T, Axes...>::operator<=>(
    const GridGlobalIterator<T, Axes...>& other) const {
  // This operator only makes sense if the two iterators we are comparing
  // are using the same grid
  assert(m_grid.ptr == other.m_grid.ptr);
  return m_idx <=> other.m_idx;
}

template <typename T, class... Axes>
GridGlobalIterator<T, Axes...>& GridGlobalIterator<T, Axes...>::operator+=(
    const std::size_t offset) {
  m_idx += offset;
  return *this;
}

template <typename T, class... Axes>
GridGlobalIterator<T, Axes...>& GridGlobalIterator<T, Axes...>::operator-=(
    const std::size_t offset) {
  m_idx -= offset;
  return *this;
}

template <typename T, class... Axes>
GridGlobalIterator<T, Axes...> GridGlobalIterator<T, Axes...>::operator+(
    const std::size_t offset) const {
  return {*m_grid, m_idx + offset};
}

template <typename T, class... Axes>
GridGlobalIterator<T, Axes...> GridGlobalIterator<T, Axes...>::operator-(
    const std::size_t offset) const {
  return {*m_grid, m_idx - offset};
}

template <typename T, class... Axes>
typename GridGlobalIterator<T, Axes...>::difference_type
GridGlobalIterator<T, Axes...>::operator-(
    const GridGlobalIterator<T, Axes...>& other) const {
  assert(m_grid.ptr == other.m_grid.ptr);
  assert(other <= *this);
  return m_idx - other.m_idx;
}

template <typename T, class... Axes>
const typename GridGlobalIterator<T, Axes...>::value_type&
GridGlobalIterator<T, Axes...>::operator*() const {
  return m_grid->at(m_idx);
}

template <typename T, class... Axes>
GridGlobalIterator<T, Axes...>& GridGlobalIterator<T, Axes...>::operator++() {
  ++m_idx;
  return *this;
}

template <typename T, class... Axes>
GridGlobalIterator<T, Axes...> GridGlobalIterator<T, Axes...>::operator++(int) {
  GridGlobalIterator<T, Axes...> output(*m_grid, m_idx++);
  return output;
}

template <typename T, class... Axes>
std::size_t GridGlobalIterator<T, Axes...>::globalBinIndex() const {
  return m_idx;
}

template <typename T, class... Axes>
std::array<std::size_t, GridGlobalIterator<T, Axes...>::DIM>
GridGlobalIterator<T, Axes...>::localBinsIndices() const {
  return m_grid->localBinsFromGlobalBin(m_idx);
}

// Local Iterator
template <typename T, class... Axes>
Acts::GridLocalIterator<T, Axes...>::GridLocalIterator(
    const Acts::Grid<T, Axes...>& grid,
    const std::array<std::size_t, DIM>& indices)
    : m_grid(&grid),
      m_numLocalBins(grid.numLocalBins()),
      m_currentIndex(indices) {
  // Since the user has not defined a custom navigation pattern, we tell the
  // iterator we want to iterate on all the local bins in ascending order from
  // 1ul to numLocalBin for that specific axis.
  for (std::size_t i(0); i < DIM; ++i) {
    m_navigationIndex[i].resize(m_numLocalBins[i]);
    std::iota(m_navigationIndex[i].begin(), m_navigationIndex[i].end(), 1ul);
  }
}

template <typename T, class... Axes>
Acts::GridLocalIterator<T, Axes...>::GridLocalIterator(
    const Acts::Grid<T, Axes...>& grid,
    const std::array<std::size_t, DIM>& indices,
    std::array<std::vector<std::size_t>, DIM> navigation)
    : m_grid(&grid),
      m_numLocalBins(grid.numLocalBins()),
      m_currentIndex(indices),
      m_navigationIndex(std::move(navigation)) {
  /// We can allow navigation on only a subset of bins.
  /// If the number of specified bins in the navigation for one axis is not
  /// zero then override the maximum number of navigation bins instead of using
  /// the total number of available bins in the axis
  for (std::size_t i(0ul); i < DIM; ++i) {
    /// We do not allow empty bin sequences
    if (m_navigationIndex[i].size() == 0) {
      throw std::invalid_argument(
          "Invalid navigation sequence in local grid iterator. No bins "
          "specified.");
    }
    /// Too many bins
    if (m_navigationIndex[i].size() > m_numLocalBins[i]) {
      throw std::invalid_argument(
          "Invalid navigation sequence in local grid iterator. Too many bins "
          "specified.");
    }
    m_numLocalBins[i] = m_navigationIndex[i].size();
  }
}

template <typename T, class... Axes>
Acts::GridLocalIterator<T, Axes...>::GridLocalIterator(
    Acts::GridLocalIterator<T, Axes...>&& other) noexcept
    : m_grid(std::exchange(other.m_grid.ptr, nullptr)),
      m_numLocalBins(other.m_numLocalBins),
      m_currentIndex(other.m_currentIndex),
      m_navigationIndex(std::move(other.m_navigationIndex)) {}

template <typename T, class... Axes>
Acts::GridLocalIterator<T, Axes...>&
Acts::GridLocalIterator<T, Axes...>::operator=(
    Acts::GridLocalIterator<T, Axes...>&& other) noexcept {
  m_grid.ptr = std::exchange(other.m_grid.ptr, nullptr);
  m_numLocalBins = other.m_numLocalBins;
  m_currentIndex = other.m_currentIndex;
  m_navigationIndex = std::move(other.m_navigationIndex);
  return *this;
}

template <typename T, class... Axes>
bool Acts::GridLocalIterator<T, Axes...>::operator==(
    const Acts::GridLocalIterator<T, Axes...>& other) const {
  // This will always return false if we are comparing two iterators from
  // different grids.
  // As such a loop from itrStart (from grid A) to itrStop (from grid B) will
  // never complete since itrStop will not be reachable from itrStart
  if (m_grid.ptr != other.m_grid.ptr) {
    return false;
  }

  for (std::size_t i(0); i < DIM; ++i) {
    if (m_currentIndex[i] != other.m_currentIndex[i]) {
      return false;
    }
  }

  return true;
}

template <typename T, class... Axes>
const typename Acts::GridLocalIterator<T, Axes...>::value_type&
Acts::GridLocalIterator<T, Axes...>::operator*() const {
  std::array<std::size_t, DIM> localPositionBin{};
  for (std::size_t i(0); i < DIM; ++i) {
    localPositionBin[i] = m_navigationIndex[i][m_currentIndex[i]];
  }
  return m_grid->atLocalBins(localPositionBin);
}

template <typename T, class... Axes>
GridLocalIterator<T, Axes...>& GridLocalIterator<T, Axes...>::operator++() {
  increment<DIM - 1>();
  return *this;
}

template <typename T, class... Axes>
GridLocalIterator<T, Axes...> GridLocalIterator<T, Axes...>::operator++(int) {
  GridLocalIterator<T, Axes...> output(*this);
  this->operator++();
  return output;
}

template <typename T, class... Axes>
template <std::size_t N>
void GridLocalIterator<T, Axes...>::increment() {
  // Check if the current local bin can be incremented, or we reached the end
  // of bins in the axis
  if (++m_currentIndex[N] < m_numLocalBins[N]) {
    return;
  }
  // We have reached the last bin in the axis, we set the position to 0ul and
  // try to increment another axis
  if constexpr (N != 0) {
    m_currentIndex[N] = 0;
    increment<N - 1>();
  } else {
    m_currentIndex = m_numLocalBins;
  }
}

template <typename T, class... Axes>
std::size_t GridLocalIterator<T, Axes...>::globalBinIndex() const {
  return m_grid->globalBinFromLocalBins(localBinsIndices());
}

template <typename T, class... Axes>
std::array<std::size_t, GridLocalIterator<T, Axes...>::DIM>
GridLocalIterator<T, Axes...>::localBinsIndices() const {
  std::array<std::size_t, DIM> output{};
  for (std::size_t i(0); i < DIM; ++i) {
    output[i] = m_navigationIndex[i][m_currentIndex[i]];
  }
  return output;
}

}  // namespace Acts
