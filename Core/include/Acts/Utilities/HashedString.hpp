// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace Acts {
using HashedString = std::uint32_t;

// Adapted from https://gist.github.com/Lee-R/3839813
namespace detail {
// FNV-1a 32bit hashing algorithm.
constexpr HashedString fnv1a_32(char const* s, std::size_t count) {
  return count != 0u ? (fnv1a_32(s, count - 1) ^ s[count - 1]) * 16777619u
                     : 2166136261u;
}

constexpr HashedString fnv1a_32(std::string_view s) {
  return !s.empty() ? (fnv1a_32(s.substr(0, s.size() - 1)) ^ s[s.size() - 1]) *
                          16777619u
                    : 2166136261u;
}

constexpr int length(const char* str) {
  return *str != 0 ? 1 + length(str + 1) : 0;
}
}  // namespace detail

constexpr HashedString hashString(std::string_view s) {
  return detail::fnv1a_32(s);
}

namespace HashedStringLiteral {
constexpr HashedString operator"" _hash(char const* s, std::size_t count) {
  return detail::fnv1a_32(s, count);
}

}  // namespace HashedStringLiteral
}  // namespace Acts
