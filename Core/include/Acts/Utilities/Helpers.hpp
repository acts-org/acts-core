// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#define ACTS_CHECK_BIT(value, mask) ((value & mask) == mask)

namespace Acts {

/// Helper function to unpack a vector of @c shared_ptr into a vector of raw
/// pointers
/// @tparam T the stored type
/// @param items The vector of @c shared_ptr
/// @return The unpacked vector
template <typename T>
std::vector<T*> unpack_shared_vector(
    const std::vector<std::shared_ptr<T>>& items) {
  std::vector<T*> rawPtrs;
  rawPtrs.reserve(items.size());
  for (const std::shared_ptr<T>& item : items) {
    rawPtrs.push_back(item.get());
  }
  return rawPtrs;
}

/// Helper function to unpack a vector of @c shared_ptr into a vector of raw
/// pointers (const version)
/// @tparam T the stored type
/// @param items The vector of @c shared_ptr
/// @return The unpacked vector
template <typename T>
std::vector<const T*> unpack_shared_vector(
    const std::vector<std::shared_ptr<const T>>& items) {
  std::vector<const T*> rawPtrs;
  rawPtrs.reserve(items.size());
  for (const std::shared_ptr<const T>& item : items) {
    rawPtrs.push_back(item.get());
  }
  return rawPtrs;
}

/// Helper function to unpack a vector of @c shared_ptr into a vector of raw
/// pointers
/// @tparam T the stored type
/// @param items The vector of @c shared_ptr
/// @return The unpacked vector
template <typename T>
std::vector<const T*> unpack_shared_const_vector(
    const std::vector<std::shared_ptr<T>>& items) {
  std::vector<const T*> rawPtrs;
  rawPtrs.reserve(items.size());
  for (const std::shared_ptr<T>& item : items) {
    rawPtrs.push_back(item.get());
  }
  return rawPtrs;
}

/// @brief Converts a vector to a fixed-size array with truncating or padding.
///
/// This function copies elements from the input vector into a fixed-size array.
/// If the vector contains more than `kDIM` elements, the array is truncated to
/// fit. If the vector contains fewer elements than `kDIM`, the remaining array
/// elements are value-initialized (default-initialized, i.e., filled with zero
/// or default values).
///
/// @tparam kDIM The size of the resulting array.
/// @tparam value_t The type of elements in the vector and the array.
/// @param vecvals The input vector to be converted to an array.
///
/// @return An array containing the first `kDIM` elements of the vector.
template <std::size_t kDIM, typename value_t>
std::array<value_t, kDIM> toArray(const std::vector<value_t>& vecvals) {
  std::array<value_t, kDIM> arr = {};
  std::copy_n(vecvals.begin(), std::min(vecvals.size(), kDIM), arr.begin());
  return arr;
}

/// @brief Dispatch a call based on a runtime value on a function taking the
/// value at compile time.
///
/// This function allows to write a templated functor, which accepts a @c std::size_t
/// like parameter at compile time. It is then possible to make a call to the
/// corresponding instance of the functor based on a runtime value. To achieve
/// this, the function essentially created a if cascade between @c N and @c
/// NMAX, attempting to find the right instance. Because the cascade is visible
/// to the compiler entirely, it should be able to optimize.
///
/// @tparam Callable Type which takes a std::size_t as a compile time param
/// @tparam N Value from which to start the dispatch chain, i.e. 0 in most cases
/// @tparam NMAX Maximum value up to which to attempt a dispatch
/// @param v The runtime value to dispatch on
/// @param args Additional arguments passed to @c Callable::invoke().
/// @note @c Callable is expected to have a static member function @c invoke
/// that is callable with @c Args
template <template <std::size_t> class Callable, std::size_t N,
          std::size_t NMAX, typename... Args>
auto template_switch(std::size_t v, Args&&... args) {
  if (v == N) {
    return Callable<N>::invoke(std::forward<Args>(args)...);
  }
  if (v == 0) {
    std::cerr << "template_switch<Fn, " << N << ", " << NMAX << ">(v=" << v
              << ") is not valid (v == 0 and N != 0)" << std::endl;
    std::abort();
  }
  if constexpr (N < NMAX) {
    return template_switch<Callable, N + 1, NMAX>(v,
                                                  std::forward<Args>(args)...);
  }
  std::cerr << "template_switch<Fn, " << N << ", " << NMAX << ">(v=" << v
            << ") is not valid (v > NMAX)" << std::endl;
  std::abort();
}

/// Alternative version of @c template_switch which accepts a generic
/// lambda and communicates the dimension via an integral constant type
/// @tparam N Value from which to start the dispatch chain, i.e. 0 in most cases
/// @tparam NMAX Maximum value up to which to attempt a dispatch
/// @param v The runtime value to dispatch on
/// @param func The lambda to invoke
/// @param args Additional arguments passed to @p func
template <std::size_t N, std::size_t NMAX, typename Lambda, typename... Args>
auto template_switch_lambda(std::size_t v, Lambda&& func, Args&&... args) {
  if (v == N) {
    return func(std::integral_constant<std::size_t, N>{},
                std::forward<Args>(args)...);
  }
  if (v == 0) {
    std::cerr << "template_switch<Fn, " << N << ", " << NMAX << ">(v=" << v
              << ") is not valid (v == 0 and N != 0)" << std::endl;
    std::abort();
  }
  if constexpr (N < NMAX) {
    return template_switch_lambda<N + 1, NMAX>(v, func,
                                               std::forward<Args>(args)...);
  }
  std::cerr << "template_switch<Fn, " << N << ", " << NMAX << ">(v=" << v
            << ") is not valid (v > NMAX)" << std::endl;
  std::abort();
}

/// Clamp a numeric value to another type, respecting range of the target type
/// @tparam T the target type
/// @tparam U the source type
/// @param value the value to clamp
/// @return the clamped value
template <typename T, typename U>
T clampValue(U value) {
  return std::clamp(value, static_cast<U>(std::numeric_limits<T>::lowest()),
                    static_cast<U>(std::numeric_limits<T>::max()));
}

/// Return range and medium of an unsorted numeric series
///
/// @tparam T a numeric series
///
/// @param tseries is the number series
///
/// @return [ range, medium ] in an tuple
template <typename T>
std::tuple<typename T::value_type, ActsScalar> range_medium(const T& tseries) {
  auto [minIt, maxIt] = std::ranges::minmax_element(tseries);
  typename T::value_type range = (*maxIt - *minIt);
  ActsScalar medium = static_cast<ActsScalar>((*maxIt + *minIt) * 0.5);
  return std::tie(range, medium);
}

template <typename enum_t>
constexpr std::underlying_type_t<enum_t> toUnderlying(enum_t value) {
  return static_cast<std::underlying_type_t<enum_t>>(value);
}

/// This can be replaced with C++23 to use the std::ranges::contains method
///
/// This function searches through the given range for a specified value
/// and returns `true` if the value is found, or `false` otherwise.
///
/// @tparam R The type of the range (e.g., vector, list, array).
/// @tparam T The type of the value to search for within the range.
///
/// @param range The range to search within. This can be any range-compatible container.
/// @param value The value to search for in the range.
///
/// @return `true` if the value is found within the range, `false` otherwise.
template <typename R, typename T>
bool rangeContainsValue(const R& range, const T& value) {
  return std::ranges::find(range, value) != std::ranges::end(range);
}

}  // namespace Acts
