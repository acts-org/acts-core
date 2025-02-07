// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <concepts>
#include <functional>
#include <type_traits>

namespace Acts::Concepts {
/// @brief Concept that is true if T is the same as any of Ts.
template <typename T, typename... Ts>
concept same_as_any_of = (std::same_as<T, Ts> || ...);

/// @brief Concept that is equivalent to `is_nothrow_move_constructible`.
/// @todo Convert this to a "real" concept.
template <typename T>
concept nothrow_move_constructible = std::is_nothrow_move_constructible_v<T>;

/// @brief Concept that is true if T is an arithmetic type.
template <typename T>
concept arithmetic = std::integral<T> || std::floating_point<T>;

/// @brief Concept that is satisfied iff both of its arguments decay to the
/// same type.
template <typename T1, typename T2>
concept decayed_same_as = std::same_as<std::decay_t<T1>, std::decay_t<T2> >;

/// @brief Concept that is satisfied iff type T is callable with arguments
/// Args... and returns type U
template <auto Callable, typename U, typename... Args>
concept invocable_and_returns = requires(Args... args) {
  { std::invoke(Callable, args...) } -> std::same_as<U>;
};
}  // namespace Acts::Concepts
