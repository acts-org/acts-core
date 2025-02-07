// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/SourceLink.hpp"

#include <ranges>

namespace Acts::detail {

// Define some concepts
template <typename external_t>
concept isCollectionThatSupportsPushBack =
    std::ranges::range<external_t> && requires {
      typename external_t::value_type;
    } && requires(external_t coll, typename external_t::value_type val) {
      coll.push_back(val);
    };

template <typename external_t>
concept isCollectionThatSupportsInsert =
    std::ranges::range<external_t> && requires {
      typename external_t::value_type;
    } && requires(external_t coll, typename external_t::value_type val) {
      coll.insert(std::ranges::end(coll), val);
    };

template <typename grid_t>
concept SourceLinkGrid =
    std::same_as<typename grid_t::value_type, std::vector<Acts::SourceLink>>;

// Define some functions
template <Acts::detail::isCollectionThatSupportsPushBack storage_t,
          typename value_t>
  requires requires(storage_t coll, value_t value) {
    coll.push_back(value);
    coll.push_back(std::move(value));
  }
void pushBackOrInsertAtEnd(storage_t& storage, value_t&& value) {
  storage.push_back(std::forward<value_t>(value));
}

template <std::ranges::range storage_t, typename value_t>
  requires(!Acts::detail::isCollectionThatSupportsPushBack<storage_t> &&
           Acts::detail::isCollectionThatSupportsInsert<storage_t> &&
           requires(storage_t coll, value_t value) {
             coll.insert(std::ranges::end(coll), value);
             coll.insert(std::ranges::end(coll), std::move(value));
           })
void pushBackOrInsertAtEnd(storage_t& storage, value_t&& value) {
  storage.insert(std::ranges::end(storage), std::forward<value_t>(value));
}

}  // namespace Acts::detail
