// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <compare>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace Acts {
template <typename _Container, typename Value, bool Const>
class ContainerIndexIterator {
 public:
  using value_type = Value;
  using iterator_category = std::random_access_iterator_tag;
  using container_type =
      std::conditional_t<Const, const _Container, _Container>;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;

  ContainerIndexIterator() : m_container(nullptr), m_index(0) {}

  ContainerIndexIterator(container_type& container, std::size_t index)
      : m_container(&container), m_index(index) {}

  template <typename OtherValue, bool OtherConst>
  explicit ContainerIndexIterator(
      const ContainerIndexIterator<_Container, OtherValue, OtherConst>& o)
    requires(!OtherConst || Const)
      : m_container(o.m_container), m_index(o.m_index) {}

  value_type operator*() const {
    assert(m_container != nullptr);
    return m_container->at(m_index);
  }

  template <typename OtherValue, bool OtherConst>
  ContainerIndexIterator& operator=(
      const ContainerIndexIterator<_Container, OtherValue, OtherConst>& o)
    requires(!OtherConst || Const)
  {
    m_container = o.m_container;
    m_index = o.m_index;
    return *this;
  }

  ContainerIndexIterator& operator++() {
    ++m_index;
    return *this;
  }

  ContainerIndexIterator operator++(int) {
    auto copy = *this;
    ++*this;
    return copy;
  }

  ContainerIndexIterator& operator+=(const difference_type& i) {
    m_index += i;
    return *this;
  }

  friend ContainerIndexIterator operator+(const ContainerIndexIterator& t,
                                          const difference_type& i) {
    return ContainerIndexIterator(*t.m_container, t.m_index + i);
  }

  friend ContainerIndexIterator operator+(const difference_type& i,
                                          const ContainerIndexIterator& t) {
    return t + i;
  }

  ContainerIndexIterator& operator--() {
    --m_index;
    return *this;
  }

  ContainerIndexIterator operator--(int) {
    auto copy = *this;
    --*this;
    return copy;
  }

  ContainerIndexIterator& operator-=(const difference_type& i) {
    m_index -= i;
    return *this;
  }

  friend ContainerIndexIterator operator-(const ContainerIndexIterator& t,
                                          const difference_type& i) {
    return ContainerIndexIterator(*t.m_container, t.m_index - i);
  }

  template <typename OtherValue, bool OtherConst>
  friend difference_type operator-(
      const ContainerIndexIterator& t,
      const ContainerIndexIterator<_Container, OtherValue, OtherConst>& o) {
    assert(t.m_container == o.m_container);
    return t.m_index - o.m_index;
  }

  value_type operator[](const difference_type& i) const { return *(*this + i); }

  template <typename OtherValue, bool OtherConst>
  std::strong_ordering operator<=>(
      const ContainerIndexIterator<_Container, OtherValue, OtherConst>& o)
      const {
    if (m_container == o.m_container) {
      return m_index <=> o.m_index;
    } else {
      return m_container <=> o.m_container;
    }
  }

  template <typename OtherValue, bool OtherConst>
  bool operator==(const ContainerIndexIterator<_Container, OtherValue,
                                               OtherConst>& other) const {
    return m_container == other.m_container && m_index == other.m_index;
  }

 private:
  container_type* m_container;
  typename container_type::size_type m_index;
};
}  // namespace Acts
