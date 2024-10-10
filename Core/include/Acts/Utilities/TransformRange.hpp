// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <iterator>
#include <type_traits>
#include <utility>

namespace Acts::detail {

template <typename Callable, typename iterator_t, bool force_const>
struct TransformRangeIterator;

/// This type implements a transforming range over a container.
/// It functions like a view, where all element access is passed through
/// a user-defined callable, that can process values, like dereferencing them
/// or calling a specific method.
///
/// @note The range and associated iterator maintain const-ness of the input,
///       i.e. if a const-qualified container is passed, the range and iterators
///       will not return any mutable references, even if they are mutable
///       themselves
/// @note The range and iterator assume the the callables return references
///
/// @tparam Callable The callable to apply to each element.
/// @tparam container_t The container to wrap.
template <typename Callable, typename container_t>
struct TransformRange {
 private:
  using internal_value_type = typename container_t::value_type;

  static_assert(std::is_reference_v<decltype(Callable::apply(
                    std::declval<internal_value_type>()))>,
                "The callable must return a reference type");

  using raw_value_type = std::remove_reference_t<decltype(Callable::apply(
      std::declval<internal_value_type>()))>;

 public:
  /// The underlying value type that is returned by the range.
  /// If the input container has const-qualification, the range does
  /// not expose mutable values
  using value_type =
      std::conditional_t<std::is_const_v<container_t>,
                         std::add_const_t<raw_value_type>, raw_value_type>;

  using reference = value_type&;
  using const_reference = const value_type&;

  using iterator = TransformRangeIterator<
      Callable,
      std::conditional_t<std::is_const_v<container_t>,
                         typename container_t::const_iterator,
                         typename container_t::iterator>,
      std::is_const_v<container_t>>;
  using const_iterator =
      TransformRangeIterator<Callable, typename container_t::const_iterator,
                             true>;

  /// Construct a transforming range from a container. The first argument is
  /// only used for type-deduction
  /// @param container The container to wrap
  TransformRange(Callable&& /*callable*/, container_t& container)
      : m_container(&container) {}

  /// Construct a transforming range from a construct
  /// @param container The container to wrap
  explicit TransformRange(container_t& container) : m_container(&container) {}

  /// Access the i-th element of the underlying container, applying the
  /// callable
  /// @param i The index of the element to access
  /// @return Reference to the transformed i-th element
  reference operator[](std::size_t i) {
    return Callable::apply((*m_container)[i]);
  }

  /// Access the i-th element of the underlying container, applying the
  /// callable
  /// @param i The index of the element to access
  /// @return Const-reference to the transformed i-th element
  const_reference operator[](std::size_t i) const {
    return std::as_const(Callable::apply((*m_container)[i]));
  }

  /// Access the i-th element of the underlying container, applying the
  /// callable
  /// @param i The index of the element to access
  /// @return Reference to the transformed i-th element
  reference at(std::size_t i) { return Callable::apply(m_container->at(i)); }

  /// Access the i-th element of the underlying container, applying the
  /// callable
  /// @param i The index of the element to access
  /// @return Const-reference to the transformed i-th element
  const_reference at(std::size_t i) const {
    return std::as_const(Callable::apply(m_container->at(i)));
  }

  /// Return an iterator to the beginning of the underlying container
  /// @return Iterator to the beginning of the range
  iterator begin() { return iterator{m_container->begin()}; }

  /// Return an iterator past the end of the underlying container
  /// @return Iterator past the end of the range
  iterator end() { return iterator{m_container->end()}; }

  /// Return a const-iterator to the beginning of the underlying container
  /// @return Const-iterator to the beginning of the range
  const_iterator begin() const { return const_iterator{m_container->begin()}; }

  /// Return a const-iterator past the end of the underlying container
  /// @return Const-iterator past the end of the range
  const_iterator cbegin() const { return begin(); }

  /// Return a const-iterator to the beginning of the underlying container
  /// @return Const-iterator to the beginning of the range
  const_iterator end() const { return const_iterator{m_container->end()}; }

  /// Return a const-iterator past the end of the underlying container
  /// @return Const-iterator past the end of the range
  const_iterator cend() const { return end(); }

  /// Return the size of the underlying container
  /// @return The size of the underlying container
  std::size_t size() const { return m_container->size(); }

  /// Check if the underlying container is empty
  /// @return True if the underlying container is empty
  bool empty() const { return m_container->empty(); }

 private:
  container_t* m_container;
};

/// This type is associated with @c TransformRange and implements the iterator
/// for the range. It applies the callable to the value that is dereferences.
/// It also maintains const-ness, if instructed, by returning const references
/// only.
/// @tparam Callable The callable to apply to the value
/// @tparam iterator_t The iterator type of the underlying container
template <typename Callable, typename iterator_t, bool force_const>
struct TransformRangeIterator {
 private:
  using internal_value_type = typename std::iter_value_t<iterator_t>;

  using raw_value_type = std::remove_reference_t<decltype(Callable::apply(
      std::declval<internal_value_type>()))>;

 public:
  /// The underlying value type that is returned by the iterator.
  /// If @c force_const is set to true, the iterator will only return
  /// const-qualified references
  using value_type =
      std::conditional_t<force_const, std::add_const_t<raw_value_type>,
                         raw_value_type>;

  using difference_type = typename std::iter_difference_t<iterator_t>;
  using pointer = std::remove_reference_t<value_type>*;
  using reference = value_type&;
  using iterator_category = std::forward_iterator_tag;

  /// Construct an iterator from an underlying iterator
  explicit TransformRangeIterator(iterator_t iterator) : m_iterator(iterator) {}

  /// Return a reference to the value that is transformed by the callable
  /// @return Reference to the transformed value
  reference operator*() { return Callable::apply(*m_iterator); }

  /// Return a const-reference to the value that is transformed by the callable
  /// @return Const-reference to the transformed value
  reference operator*() const { return Callable::apply(*m_iterator); }

  /// Advance the iterator
  /// @return Reference to the iterator
  TransformRangeIterator& operator++() {
    ++m_iterator;
    return *this;
  }

  /// Compare two iterators for equality
  /// @param other The other iterator to compare to
  bool operator==(const TransformRangeIterator& other) const {
    return m_iterator == other.m_iterator;
  }

 private:
  iterator_t m_iterator;
};

/// Callable that dereferences a value
struct Dereference {
  template <typename input_t>
  constexpr static decltype(auto) apply(input_t&& value) {
    return *value;
  }
};

/// Callable that const-dereferences a value
struct ConstDereference {
  template <typename input_t>
  constexpr static decltype(auto) apply(input_t&& value) {
    return std::as_const(*value);
  }
};

/// Callable that calls the @c get method of a value
struct DotGet {
  template <typename input_t>
  constexpr static decltype(auto) apply(input_t&& value) {
    return value.get();
  }
};

}  // namespace Acts::detail
