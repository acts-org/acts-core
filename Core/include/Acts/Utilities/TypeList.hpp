// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace Acts {

/// @brief type list implementation
/// @see https://www.codingwiththomas.com/blog/getting-started-with-typelists
template <typename... Ts>
struct TypeList {};

namespace Types {

/// Number of types in the list
/// @{
template <typename = void>
struct getSize {};

template <typename... Ts>
struct getSize<TypeList<Ts...>>
    : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <typename L>
constexpr std::size_t size{getSize<L>()};
/// @}

/// Access the first type
/// @{
template <typename = void>
struct getFront {};

template <typename T, typename... Ts>
struct getFront<TypeList<T, Ts...>> {
  using type = T;
};

template <typename L>
using front = typename getFront<L>::type;
/// @}

/// Access the last type
/// @{
template <typename = void>
struct getBack {};

template <typename T, typename... Ts>
struct getBack<TypeList<T, Ts...>> {
  using type = std::conditional_t<sizeof...(Ts) == 0, T,
                                  typename getBack<TypeList<Ts...>>::type>;
};
// Base case
template <>
struct getBack<TypeList<>> {
  using type = void;
};
template <typename L>
using back = typename getBack<L>::type;
/// @}

/// Append a type
/// @{
template <typename N, typename = void>
struct doPushBack {};

template <typename N, typename... Ts>
struct doPushBack<N, TypeList<Ts...>> {
  using type = TypeList<Ts..., N>;
};
template <typename L, typename N>
using push_back = typename doPushBack<N, L>::type;
/// @}

/// Prepend a type
/// @{
template <typename N, typename = void>
struct doPushFront {};

template <typename N, typename... Ts>
struct doPushFront<N, TypeList<Ts...>> {
  using type = TypeList<N, Ts...>;
};

template <typename L, typename N>
using push_front = typename doPushFront<N, L>::type;
/// @}

/// Filter a list of types
/// @{
template <template <typename> typename Pred, typename... Ts>
struct filter {};

template <template <typename> typename Pred>
struct filter<Pred> {
  using type = TypeList<>;
};

template <typename T, typename... Ts, template <typename> typename Pred>
struct filter<Pred, T, Ts...> {
  using _next_type = typename filter<Pred, Ts...>::type;
  using type =
      std::conditional_t<Pred<T>::value, push_front<_next_type, T>, _next_type>;
};
/// @}

/// Apply a typelist to a type function
/// @{
template <template <typename...> typename F, typename T>
struct apply {};

template <template <typename...> typename F, typename... Ts>
struct apply<F, TypeList<Ts...>> {
  using type = F<Ts...>;
};
/// @}

/// Apply a mapping function to a type list
/// @{
template <template <typename> typename F, typename T>
struct map {};

template <template <typename> typename F, typename... Ts>
struct map<F, TypeList<Ts...>> {
  using type = TypeList<typename F<Ts>::type...>;
};
/// @}

/// Count the number of times a type occurs in a type list
/// @{
template <typename T, typename L>
struct count {};

template <typename T>
struct count<T, TypeList<>> {
  static constexpr std::size_t value = 0;
};

template <typename T, typename U, typename... Us>
struct count<T, TypeList<U, Us...>> {
  static constexpr std::size_t value =
      std::conditional_t<std::is_same_v<T, U>,
                         std::integral_constant<std::size_t, 1>,
                         std::integral_constant<std::size_t, 0>>::value +
      count<T, TypeList<Us...>>::value;
};
/// @}

}  // namespace Types
}  // namespace Acts
