// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

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

// Define some functions
template <typename value_t>
void pushBackOrInsertAtEnd(
    Acts::detail::isCollectionThatSupportsPushBack auto& storage,
    value_t&& value) {
  storage.push_back(std::forward<value_t>(value));
}

template <std::ranges::range storage_t, typename value_t>
  requires(!Acts::detail::isCollectionThatSupportsPushBack<storage_t> &&
           Acts::detail::isCollectionThatSupportsInsert<storage_t>)
void pushBackOrInsertAtEnd(storage_t& storage, value_t&& value) {
  storage.insert(std::ranges::end(storage), std::forward<value_t>(value));
}

}  // namespace Acts::detail
