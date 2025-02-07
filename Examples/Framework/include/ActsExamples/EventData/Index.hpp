// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <algorithm>
#include <cstdint>

#include <boost/container/flat_map.hpp>
#include <boost/version.hpp>

namespace ActsExamples {

/// Index type to reference elements in a container.
///
/// We do not expect to have more than 2^32 elements in any given container so a
/// fixed sized integer type is sufficient.
using Index = std::uint32_t;

/// Store elements that are identified by an index, e.g. in another container.
///
/// Each index can have zero or more associated elements. A typical case could
/// be to store all generating particles for a hit where the hit is identified
/// by its index in the hit container.
template <typename value_t>
using IndexMultimap = boost::container::flat_multimap<Index, value_t>;

/// Store the inverse of an index multimap, i.e. from a -> {b...} to b ->
/// {a...}.
///
/// @note This assumes that the value in the initial multimap is itself a
///   sortable index-like object, as would be the case when mapping e.g.
///   hit ids to particle ids/ barcodes.
template <typename value_t>
using InverseMultimap = boost::container::flat_multimap<value_t, Index>;

/// Invert the multimap, i.e. from a -> {b...} to b -> {a...}
template <typename value_t>
inline InverseMultimap<value_t> invertIndexMultimap(
    const IndexMultimap<value_t>& multimap) {
  // switch key-value without enforcing the new ordering (linear copy)
  typename InverseMultimap<value_t>::sequence_type unordered;
  unordered.reserve(multimap.size());
  for (auto&& [index, value] : multimap) {
    // value is now the key and the index is now the value
    unordered.emplace_back(value, index);
  }

  // adopting the unordered sequence will reestablish the correct order
  InverseMultimap<value_t> inverse;
#if BOOST_VERSION < 107800
  for (const auto& i : unordered) {
    inverse.insert(i);
  }
#else
  std::ranges::sort(unordered);
  inverse.insert(boost::container::ordered_range_t{}, unordered.begin(),
                 unordered.end());
#endif

  return inverse;
}

}  // namespace ActsExamples
