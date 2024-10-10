// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Utilities/Range.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace ActsExamples {

/// Proxy for iterating over groups of elements within a container.
///
/// @note Each group will contain at least one element.
///
/// Consecutive elements with the same key (as defined by the KeyGetter) are
/// placed in one group. The proxy should always be used as part of a
/// range-based for loop. In combination with structured bindings to reduce the
/// boilerplate, the group iteration can be written as
///
///     for (auto&& [key, elements] : GroupBy<...>(...)) {
///         // do something with just the key
///         ...
///
///         // iterate over the group elements
///         for (const auto& element : elements) {
///             ...
///         }
///     }
///
template <typename Iterator, typename KeyGetter>
class GroupBy {
 public:
  /// The key type that identifies elements within a group.
  using Key = std::decay_t<decltype(KeyGetter()(*Iterator()))>;
  /// A Group is an iterator range with the associated key.
  using Group = std::pair<Key, Range<Iterator>>;
  /// Iterator type representing the end of the groups.
  ///
  /// The end iterator will not be dereferenced in C++17 range-based loops. It
  /// can thus be a simpler type without the overhead of the full group iterator
  /// below.
  using GroupEndIterator = Iterator;
  /// Iterator type representing a group of elements.
  class GroupIterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using value_type = Group;
    using difference_type = std::ptrdiff_t;
    using pointer = Group*;
    using reference = Group&;

    constexpr GroupIterator(const GroupBy& groupBy, Iterator groupBegin)
        : m_groupBy(groupBy),
          m_groupBegin(groupBegin),
          m_groupEnd(groupBy.findEndOfGroup(groupBegin)) {}
    /// Pre-increment operator to advance to the next group.
    constexpr GroupIterator& operator++() {
      // make the current end the new group beginning
      std::swap(m_groupBegin, m_groupEnd);
      // find the end of the next group starting from the new beginning
      m_groupEnd = m_groupBy.findEndOfGroup(m_groupBegin);
      return *this;
    }
    /// Post-increment operator to advance to the next group.
    constexpr GroupIterator operator++(int) {
      GroupIterator retval = *this;
      ++(*this);
      return retval;
    }
    /// Dereference operator that returns the pointed-to group of elements.
    constexpr Group operator*() const {
      const Key key = (m_groupBegin != m_groupEnd)
                          ? m_groupBy.m_keyGetter(*m_groupBegin)
                          : Key();
      return {key, makeRange(m_groupBegin, m_groupEnd)};
    }

   private:
    const GroupBy& m_groupBy;
    Iterator m_groupBegin;
    Iterator m_groupEnd;

    friend constexpr bool operator==(const GroupIterator& lhs,
                                     const GroupEndIterator& rhs) {
      return lhs.m_groupBegin == rhs;
    }
  };

  /// Construct the group-by proxy for an iterator range.
  constexpr GroupBy(Iterator begin, Iterator end,
                    KeyGetter keyGetter = KeyGetter())
      : m_begin(begin), m_end(end), m_keyGetter(std::move(keyGetter)) {}
  constexpr GroupIterator begin() const {
    return GroupIterator(*this, m_begin);
  }
  constexpr GroupEndIterator end() const { return m_end; }
  constexpr bool empty() const { return m_begin == m_end; }

 private:
  Iterator m_begin;
  Iterator m_end;
  KeyGetter m_keyGetter;

  /// Find the end of the group that starts at the given position.
  ///
  /// This uses a linear search from the start position and thus has linear
  /// complexity in the group size. It does not assume any ordering of the
  /// underlying container and is a cache-friendly access pattern.
  constexpr Iterator findEndOfGroup(Iterator start) const {
    // check for end that we can safely dereference the start iterator.
    if (start == m_end) {
      return start;
    }
    // search the first element that does not share a key with the start.
    return std::find_if_not(std::next(start), m_end,
                            [this, start](const auto& x) {
                              return m_keyGetter(x) == m_keyGetter(*start);
                            });
  }
};

/// Construct the group-by proxy for a container.
template <typename Container, typename KeyGetter>
auto makeGroupBy(const Container& container, KeyGetter keyGetter)
    -> GroupBy<decltype(std::begin(container)), KeyGetter> {
  return {std::begin(container), std::end(container), std::move(keyGetter)};
}

}  // namespace ActsExamples
