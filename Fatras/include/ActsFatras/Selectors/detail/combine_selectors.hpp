// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace ActsFatras::detail {

/// Combine multiple selectors with a configurable combine function.
template <bool Initial, typename Combine, typename... Selectors>
class CombineSelectors {
  static_assert(0u < sizeof...(Selectors),
                "Must combine at least one selector");

 public:
  /// Call all configured selectors and combine the result.
  ///
  /// @param[in] things the inputs that will be selected
  /// @return a boolean indicating the combined selection decision
  ///
  /// @tparam Ts the types of the selected inputs
  template <typename... Ts>
  bool operator()(const Ts &...things) const {
    static_assert(
        (true && ... && std::is_same_v<bool, decltype(Selectors()(things...))>),
        "Not all selectors conform to the expected interface (bool)(const "
        "T&...)");
    return impl(std::index_sequence_for<Selectors...>(), things...);
  }

  /// Access a specific selector by index.
  template <std::size_t I>
  std::tuple_element_t<I, std::tuple<Selectors...>> &get() {
    return std::get<I>(m_selectors);
  }
  /// Access a specific selector by type.
  template <typename Selector>
  Selector &get() {
    return std::get<Selector>(m_selectors);
  }

 private:
  std::tuple<Selectors...> m_selectors;

  template <std::size_t... Is, typename... Ts>
  bool impl(std::index_sequence<Is...> /*indices*/, const Ts &...things) const {
    Combine combine;
    // compute status for all selectors
    bool status[] = {std::get<Is>(m_selectors)(things...)...};
    // reduce over the combine function with configured initial value
    bool ret = Initial;
    for (bool value : status) {
      ret = combine(ret, value);
    }
    return ret;
  }
};

}  // namespace ActsFatras::detail
