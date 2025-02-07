// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <tuple>
#include <utility>

namespace Acts {
/// Helper utility to allow indexed enumeration with structured binding
///
/// Usage:
///
/// for (auto [ i, value ] = enumerate(container) ) { ... };
///
/// with 'container' any stl-like container
///
template <typename container_type,
          typename container_type_iter =
              decltype(std::begin(std::declval<container_type>())),
          typename = decltype(std::end(std::declval<container_type>()))>
constexpr auto enumerate(container_type &&iterable) {
  struct iterator {
    std::size_t i;
    container_type_iter iter;

    bool operator!=(const iterator &rhs) const { return iter != rhs.iter; }

    /** Increase index and iterator at once */
    void operator++() {
      ++i;
      ++iter;
    }

    /** Tie them together for returning */
    auto operator*() const { return std::tie(i, *iter); }
  };
  struct iterable_wrapper {
    container_type iterable;
    auto begin() { return iterator{0, std::begin(iterable)}; }
    auto end() { return iterator{0, std::end(iterable)}; }
  };
  return iterable_wrapper{std::forward<container_type>(iterable)};
}

}  // namespace Acts
