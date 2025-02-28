// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <array>
#include <cassert>
#include <climits>
#include <ostream>
#include <type_traits>
#include <utility>

namespace Acts {

/// A set of (hierarchical) indices bitpacked into a single value.
///
/// The underlying value is split into blocks of bits with variable size.
/// Each block is a level within the index hierarchy and can be set and
/// retrieved separately. The encoded MultiIndex can be ordered and compared
/// for equality. The ordering follows the hierarchy, i.e. indices are
/// first ordered by the highest level, then within the highest level by the
/// second level and so on.
template <typename T, std::size_t... BitsPerLevel>
class MultiIndex {
 public:
  static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>,
                "The underlying storage type must be an unsigned integer");
  static_assert(0 < sizeof...(BitsPerLevel),
                "At least one level must be defined");
  static_assert((sizeof(T) * CHAR_BIT) == (... + BitsPerLevel),
                "The sum of bits per level must match the underlying storage");

  /// The type of their underlying storage value.
  using Value = T;
  static constexpr std::size_t kNumLevels = sizeof...(BitsPerLevel);

  /// Construct a MultiIndex with all levels set to zero.
  static constexpr MultiIndex Zeros() { return MultiIndex(0u); }
  /// Construct a MultiIndex from values for multiple level.
  ///
  /// This functionality must be implemented as a static, named constructor
  /// to avoid confusion with other constructors. If it would be implemented
  /// as a regular constructor, constructing a MultiIndex from a single
  /// encoded value and encoding only the first level would have the same
  /// signature and could not be distinguished.
  template <typename... Us>
  static constexpr MultiIndex Encode(Us&&... us) {
    static_assert(sizeof...(Us) <= kNumLevels,
                  "Can only encode as many levels as in the MultiIndex");

    MultiIndex index(0u);
    std::size_t lvl = 0;
    for (Value val : std::array<Value, sizeof...(Us)>{us...}) {
      index.set(lvl++, val);
    }
    return index;
  }

  /// Construct a MultiIndex from an already encoded value.
  explicit constexpr MultiIndex(Value encoded) : m_value(encoded) {}
  /// Construct a default MultiIndex with undefined values for each level.
  MultiIndex() = default;
  MultiIndex(const MultiIndex&) = default;
  MultiIndex(MultiIndex&) = default;
  MultiIndex& operator=(const MultiIndex&) = default;
  MultiIndex& operator=(MultiIndex&&) noexcept = default;
  /// Allow setting the MultiIndex from an already encoded value.
  constexpr MultiIndex& operator=(Value encoded) {
    m_value = encoded;
    return *this;
  }

  /// Get the encoded value of all index levels.
  constexpr Value value() const { return m_value; }
  /// Get the value for the index level.
  constexpr Value level(std::size_t lvl) const {
    assert((lvl < kNumLevels) && "Index level outside allowed range");
    return (m_value >> shift(lvl)) & mask(lvl);
  }
  /// Set the value of the index level.
  constexpr MultiIndex& set(std::size_t lvl, Value val) {
    assert((lvl < kNumLevels) && "Index level outside allowed range");
    // mask of valid bits at the encoded positions for the index level
    Value shiftedMask = (mask(lvl) << shift(lvl));
    // value of the index level shifted to its encoded position
    Value shiftedValue = (val << shift(lvl));
    // combine existing values with the value for the index level
    m_value = (m_value & ~shiftedMask) | (shiftedValue & shiftedMask);
    return *this;
  }

  /// Create index with the selected level increased and levels below zeroed.
  constexpr MultiIndex makeNextSibling(std::size_t lvl) const {
    assert((lvl < kNumLevels) && "Index level outside allowed range");
    // remove lower levels by shifting the upper levels to the left edge
    Value upper = (m_value >> shift(lvl));
    // increase to create sibling and shift back to zero lower levels again
    return MultiIndex{(upper + 1u) << shift(lvl)};
  }
  /// Create index with every level below the selected level maximized.
  constexpr MultiIndex makeLastDescendant(std::size_t lvl) const {
    assert((lvl < kNumLevels) && "Index level outside allowed range");
    // mask everything below the selected level
    Value maskLower = (Value{1u} << shift(lvl)) - 1u;
    // replace the masked lower levels w/ ones
    return MultiIndex{(m_value & ~maskLower) | maskLower};
  }

  /// Get the number of bits for the associated level
  static constexpr std::size_t bits(std::size_t lvl) {
    assert((lvl < kNumLevels) && "Index level outside allowed range");
    return s_bits[lvl];
  }

 private:
  // per-level mask and right-most bit position for shifting
  static constexpr std::array<std::size_t, kNumLevels> s_bits{BitsPerLevel...};
  static constexpr std::size_t shift(std::size_t lvl) {
    std::size_t s = 0u;
    // sum up all bits below the requested level
    for (std::size_t i = (lvl + 1); i < s_bits.size(); ++i) {
      s += s_bits[i];
    }
    return s;
  }
  static constexpr Value mask(std::size_t lvl) {
    return (Value{1u} << s_bits[lvl]) - 1u;
  }

  Value m_value;

  friend constexpr bool operator<(MultiIndex lhs, MultiIndex rhs) {
    return lhs.m_value < rhs.m_value;
  }

  friend constexpr bool operator==(MultiIndex lhs, MultiIndex rhs) {
    return lhs.m_value == rhs.m_value;
  }

  friend inline std::ostream& operator<<(std::ostream& os, MultiIndex idx) {
    // one level is always defined
    os << idx.level(0u);
    for (std::size_t lvl = 1; lvl < kNumLevels; ++lvl) {
      os << '|' << idx.level(lvl);
    }
    return os;
  }
};

}  // namespace Acts

// specialize std::hash so MultiIndex can be used e.g. in an unordered_map
namespace std {
template <typename Storage, std::size_t... BitsPerLevel>
struct hash<Acts::MultiIndex<Storage, BitsPerLevel...>> {
  auto operator()(
      Acts::MultiIndex<Storage, BitsPerLevel...> idx) const noexcept {
    return std::hash<Storage>()(idx.value());
  }
};
}  // namespace std
