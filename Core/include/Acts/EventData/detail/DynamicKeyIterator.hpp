// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/detail/DynamicColumn.hpp"
#include "Acts/Utilities/HashedString.hpp"

#include <unordered_map>

namespace Acts::detail {

template <typename C>
class DynamicKeyIterator {
  using map_t = std::unordered_map<HashedString, std::unique_ptr<C>>;

 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = HashedString;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;

  DynamicKeyIterator(typename map_t::const_iterator it) : m_it{it} {}
  DynamicKeyIterator() = default;

  DynamicKeyIterator& operator++() {
    m_it++;
    return *this;
  }

  DynamicKeyIterator operator++(int /*value*/) {
    auto copy = *this;
    ++(copy.m_it);
    return copy;
  }

  bool operator==(const DynamicKeyIterator& other) const {
    return m_it == other.m_it;
  }

  value_type operator*() const { return m_it->first; }

 private:
  typename map_t::const_iterator m_it;
};

static_assert(std::forward_iterator<DynamicKeyIterator<int>>,
              "DynamicKeyIterator<int> does not fulfill std::forward_iterator");

template <typename C>
class DynamicKeyRange {
 public:
  DynamicKeyRange(DynamicKeyIterator<C> begin, DynamicKeyIterator<C> end)
      : m_begin{begin}, m_end{end} {}

  DynamicKeyIterator<C> begin() const { return m_begin; }
  DynamicKeyIterator<C> end() const { return m_end; }

 private:
  DynamicKeyIterator<C> m_begin;
  DynamicKeyIterator<C> m_end;
};

}  // namespace Acts::detail
