// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <tuple>

namespace Acts {

/// Function that allows to zip some ranges to be used in a range-based for
/// loop. When wanting to mutate the entries, the result must be captured by
/// value:
///
/// for(auto [a, b, c] : zip(ra, rb, rc)) { a+=2; }
///
/// @tparam R The ranges type pack
/// @param r The ranges parameter pack
/// @note the behaviour is undefined if the ranges do not have equal range
template <typename... R>
auto zip(R &&...r) {
  struct It {
    std::tuple<decltype(r.begin())...> iterators;
    static_assert(std::tuple_size_v<decltype(iterators)> > 0);

    using reference = std::tuple<decltype(*r.begin())...>;

    auto operator++() {
      std::apply([](auto &...args) { (++args, ...); }, iterators);
      return *this;
    }

    auto operator!=(const It &other) const {
      return std::get<0>(iterators) != std::get<0>(other.iterators);
    }

    reference operator*() {
      return std::apply([](auto &...args) { return reference{*args...}; },
                        iterators);
    }
  };

  struct Zip {
    It b, e;

    auto begin() { return b; }
    auto end() { return e; }
  };

  auto begin = std::make_tuple(r.begin()...);
  auto end = std::make_tuple(r.end()...);
  return Zip{It{begin}, It{end}};
}

}  // namespace Acts
