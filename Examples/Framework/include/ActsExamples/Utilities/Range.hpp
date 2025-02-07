// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <iterator>
#include <utility>

namespace ActsExamples {

/// A wrapper around a pair of iterators to simplify range-based loops.
///
/// Some standard library algorithms return pairs of iterators to identify
/// a sub-range. This wrapper simplifies the iteration and should be used as
/// follows:
///
///     for (auto x : makeRange(std::equal_range(...)) {
///         ...
///     }
///
template <typename Iterator>
class Range {
 public:
  Range(Iterator b, Iterator e) : m_begin(b), m_end(e) {}
  Range(Range&&) noexcept = default;
  Range(const Range&) = default;
  ~Range() = default;
  Range& operator=(Range&&) noexcept = default;
  Range& operator=(const Range&) = default;

  Iterator begin() const { return m_begin; }
  Iterator end() const { return m_end; }
  bool empty() const { return m_begin == m_end; }
  std::size_t size() const { return std::distance(m_begin, m_end); }

 private:
  Iterator m_begin;
  Iterator m_end;
};

template <typename Iterator>
Range<Iterator> makeRange(Iterator begin, Iterator end) {
  return Range<Iterator>(begin, end);
}

template <typename Iterator>
Range<Iterator> makeRange(std::pair<Iterator, Iterator> range) {
  return Range<Iterator>(range.first, range.second);
}

}  // namespace ActsExamples
