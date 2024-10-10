// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

namespace Acts {

template <typename grid_t>
BinnedGroup<grid_t>::BinnedGroup(
    grid_t&& grid,
    const Acts::GridBinFinder<Acts::BinnedGroup<grid_t>::DIM>& bottomFinder,
    const Acts::GridBinFinder<Acts::BinnedGroup<grid_t>::DIM>& topFinder,
    std::array<std::vector<std::size_t>, Acts::BinnedGroup<grid_t>::DIM>
        navigation)
    : m_grid(std::move(grid)),
      m_mask(m_grid.size(true), true),
      m_bottomBinFinder(&bottomFinder),
      m_topBinFinder(&topFinder),
      m_bins(std::move(navigation)) {
  /// If navigation is not defined for all axes, then we default that to a
  /// std::iota from 1ul
  std::array<std::size_t, DIM> numLocBins = m_grid.numLocalBins();
  for (std::size_t i(0ul); i < DIM; ++i) {
    if (!m_bins[i].empty()) {
      continue;
    }
    m_bins[i].resize(numLocBins[i]);
    std::iota(m_bins[i].begin(), m_bins[i].end(), 1ul);
  }
}

template <typename grid_t>
BinnedGroup<grid_t>::BinnedGroup(
    grid_t&& grid, std::vector<bool> mask,
    const Acts::GridBinFinder<Acts::BinnedGroup<grid_t>::DIM>& bottomFinder,
    const Acts::GridBinFinder<Acts::BinnedGroup<grid_t>::DIM>& topFinder,
    std::array<std::vector<std::size_t>, Acts::BinnedGroup<grid_t>::DIM>
        navigation)
    : m_grid(std::move(grid)),
      m_mask(std::move(mask)),
      m_bottomBinFinder(&bottomFinder),
      m_topBinFinder(&topFinder),
      m_bins(std::move(navigation)) {
  // Check the elements in the mask corresponds to all the global bins in the
  // grid so that we can check if a global bin is masked
  if (m_mask.size() != m_grid.size(true)) {
    throw std::invalid_argument(
        "Provided mask does not match the grid. The number of entries must "
        "correspond to the number of global bins in the grid.");
  }

  /// If navigation is not defined for all axes, then we default that to a
  /// std::iota from 1ul
  std::array<std::size_t, DIM> numLocBins = m_grid.numLocalBins();
  for (std::size_t i(0ul); i < DIM; ++i) {
    if (!m_bins[i].empty()) {
      continue;
    }
    m_bins[i].resize(numLocBins[i]);
    std::iota(m_bins[i].begin(), m_bins[i].end(), 1ul);
  }
}

template <typename grid_t>
const grid_t& BinnedGroup<grid_t>::grid() const {
  return m_grid;
}

template <typename grid_t>
grid_t& BinnedGroup<grid_t>::grid() {
  return m_grid;
}

template <typename grid_t>
const std::vector<bool>& BinnedGroup<grid_t>::mask() const {
  return m_mask;
}

template <typename grid_t>
Acts::BinnedGroupIterator<grid_t> BinnedGroup<grid_t>::begin() const {
  return Acts::BinnedGroupIterator<grid_t>(
      *this, std::array<std::size_t, Acts::BinnedGroup<grid_t>::DIM>(), m_bins);
}

template <typename grid_t>
Acts::BinnedGroupIterator<grid_t> BinnedGroup<grid_t>::end() const {
  std::array<std::size_t, Acts::BinnedGroup<grid_t>::DIM> endline{};
  for (std::size_t i(0ul); i < Acts::BinnedGroup<grid_t>::DIM; ++i) {
    endline[i] = m_bins[i].size();
  }
  return Acts::BinnedGroupIterator<grid_t>(*this, endline, m_bins);
}

}  // namespace Acts
