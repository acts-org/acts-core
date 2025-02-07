// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <type_traits>

#define ACTS_DEFINE_ENUM_BITWISE_OPERATORS(enum_t)         \
  constexpr auto operator|(enum_t lhs, enum_t rhs) {       \
    return static_cast<enum_t>(                            \
        static_cast<std::underlying_type_t<enum_t>>(lhs) | \
        static_cast<std::underlying_type_t<enum_t>>(rhs)); \
  }                                                        \
                                                           \
  constexpr auto operator&(enum_t lhs, enum_t rhs) {       \
    return static_cast<enum_t>(                            \
        static_cast<std::underlying_type_t<enum_t>>(lhs) & \
        static_cast<std::underlying_type_t<enum_t>>(rhs)); \
  }                                                        \
                                                           \
  constexpr auto operator^(enum_t lhs, enum_t rhs) {       \
    return static_cast<enum_t>(                            \
        static_cast<std::underlying_type_t<enum_t>>(lhs) ^ \
        static_cast<std::underlying_type_t<enum_t>>(rhs)); \
  }                                                        \
                                                           \
  constexpr auto operator~(enum_t op) {                    \
    return static_cast<enum_t>(                            \
        ~static_cast<std::underlying_type_t<enum_t>>(op)); \
  }                                                        \
                                                           \
  constexpr auto& operator|=(enum_t& lhs, enum_t rhs) {    \
    lhs = lhs | rhs;                                       \
    return lhs;                                            \
  }                                                        \
                                                           \
  constexpr auto& operator&=(enum_t& lhs, enum_t rhs) {    \
    lhs = lhs & rhs;                                       \
    return lhs;                                            \
  }                                                        \
                                                           \
  constexpr enum_t& operator^=(enum_t& lhs, enum_t rhs) {  \
    lhs = lhs ^ rhs;                                       \
    return lhs;                                            \
  }
