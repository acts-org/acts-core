// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Binned SP Group Iterator

template <typename grid_t>
Acts::BinnedGroupIterator<grid_t>::BinnedGroupIterator(
    const Acts::BinnedGroup<grid_t>& group,
    std::array<std::size_t, Acts::BinnedGroupIterator<grid_t>::DIM> index,
    std::array<std::vector<std::size_t>, Acts::BinnedGroupIterator<grid_t>::DIM>
        navigation)
    : m_group(group), m_gridItr(group.grid(), index, navigation) {
  std::array<std::size_t, DIM> endline{};
  for (std::size_t i(0ul); i < DIM; ++i) {
    endline[i] = navigation[i].size();
  }
  m_gridItrEnd = typename grid_t::local_iterator_t(m_group->grid(), endline,
                                                   std::move(navigation));
  findNotEmptyBin();
}

template <typename grid_t>
bool Acts::BinnedGroupIterator<grid_t>::operator==(
    const Acts::BinnedGroupIterator<grid_t>& other) const {
  return m_group.ptr == other.m_group.ptr && m_gridItr == other.m_gridItr;
}

template <typename grid_t>
Acts::BinnedGroupIterator<grid_t>&
Acts::BinnedGroupIterator<grid_t>::operator++() {
  ++m_gridItr;
  findNotEmptyBin();
  return *this;
}

template <typename grid_t>
std::tuple<boost::container::small_vector<std::size_t,
                                          Acts::detail::ipow(3, grid_t::DIM)>,
           std::size_t,
           boost::container::small_vector<std::size_t,
                                          Acts::detail::ipow(3, grid_t::DIM)>>
Acts::BinnedGroupIterator<grid_t>::operator*() const {
  /// Get the global and local position from current iterator. This is the bin
  /// with the middle candidate And we know this is not an empty bin
  std::array<std::size_t, DIM> localPosition = m_gridItr.localBinsIndices();
  std::size_t global_index =
      m_group->grid().globalBinFromLocalBins(localPosition);

  /// Get the neighbouring bins
  boost::container::small_vector<std::size_t, Acts::detail::ipow(3, DIM)>
      bottoms =
          m_group->m_bottomBinFinder->findBins(localPosition, m_group->grid());
  boost::container::small_vector<std::size_t, Acts::detail::ipow(3, DIM)> tops =
      m_group->m_topBinFinder->findBins(localPosition, m_group->grid());

  // GCC12+ in Release throws an overread warning here due to the move.
  // This is from inside boost code, so best we can do is to suppress it.
#if defined(__GNUC__) && __GNUC__ >= 12 && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif
  return std::make_tuple(std::move(bottoms), global_index, std::move(tops));
#if defined(__GNUC__) && __GNUC__ >= 12 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

template <typename grid_t>
void Acts::BinnedGroupIterator<grid_t>::findNotEmptyBin() {
  if (m_gridItr == m_gridItrEnd) {
    return;
  }
  /// Iterate on the grid till we find a not-empty bin
  /// We start from the current bin configuration and move forward
  std::size_t dimCollection = (*m_gridItr).size();
  // Check if the current global bin is masked. This only makes sense if
  // we have not reached the end of the mask
  bool passesMask = false;
  if (m_gridItr != m_gridItrEnd) {
    passesMask = m_group->mask().at(m_gridItr.globalBinIndex());
  }
  // loop and only stop when we find a non-empty bin which is not masked
  while ((dimCollection == 0ul || !passesMask) && ++m_gridItr != m_gridItrEnd) {
    dimCollection = (*m_gridItr).size();
    if (dimCollection == 0ul) {
      continue;
    }
    passesMask = m_group->mask().at(m_gridItr.globalBinIndex());
  }
}
