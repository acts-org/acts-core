// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <type_traits>

namespace Acts {

/// @enum TrackStateFlag
///
/// This enum describes the type of TrackState
enum TrackStateFlag {
  MeasurementFlag = 0,
  ParameterFlag = 1,
  OutlierFlag = 2,
  HoleFlag = 3,
  MaterialFlag = 4,
  SharedHitFlag = 5,
  NumTrackStateFlags = 6
};

class ConstTrackStateType;

/// View type over a bitset stored in a 64 bit integer
/// This view allows modifications.
class TrackStateType {
 public:
  using raw_type = std::uint64_t;
  static constexpr std::size_t kRawBits =
      std::numeric_limits<std::make_unsigned_t<raw_type>>::digits;

  // Delete default constructor
  TrackStateType() = delete;

  /// Constructor from a reference to the underlying value container
  /// @param raw the value container
  TrackStateType(raw_type& raw) : m_raw{&raw} { assert(m_raw != nullptr); }

  // Delete copy constructor
  TrackStateType(const TrackStateType&) = delete;

  // Delete move constructor
  TrackStateType(TrackStateType&&) = delete;

  // Disable assignment
  TrackStateType& operator=(const TrackStateType&) = delete;

  // Disable move assignment
  TrackStateType& operator=(TrackStateType&&) = delete;

  /// Assign the value from another set of flags
  /// @param other the other set of flags to assign
  /// @return this object
  TrackStateType& operator=(const ConstTrackStateType& other);

  /// Return if the bit at position @p pos is 1
  /// @param pos the bit position
  /// @return if the bit at @p pos is one or not
  bool test(std::size_t pos) const {
    std::bitset<kRawBits> bs{*m_raw};
    return bs.test(pos);
  }

  /// Change the value of the bit at position @p pos to @p value.
  /// @param pos the position of the bit to change
  /// @param value the value to change the bit to
  void set(std::size_t pos, bool value = true) {
    std::bitset<kRawBits> bs{*m_raw};
    bs.set(pos, value);
    *m_raw = bs.to_ullong();
  }

  /// Change the value of the bit at position at @p pos to @c false
  /// @param pos the position of the bit to change
  void reset(std::size_t pos) { set(pos, false); }

  friend std::ostream& operator<<(std::ostream& os, const TrackStateType& t) {
    std::bitset<kRawBits> bs{*t.m_raw};
    std::bitset<TrackStateFlag::NumTrackStateFlags> trunc;
    for (std::size_t i = 0; i < TrackStateFlag::NumTrackStateFlags; i++) {
      trunc[i] = bs[i];
    }
    // SharedhitMaterialHoleOutlierParameterMeasurement
    os << "SMHOPM=" << trunc;
    return os;
  }

 private:
  raw_type* m_raw{nullptr};
};

/// View type over a bitset stored in a 64 bit integer
/// This view does not allow modifications
class ConstTrackStateType {
 public:
  using raw_type = std::uint64_t;
  static constexpr std::size_t kRawBits =
      std::numeric_limits<std::make_unsigned_t<raw_type>>::digits;

  // Delete default constructor
  ConstTrackStateType() = delete;

  /// Constructor from a reference to the underlying value container
  /// @param raw the value container
  ConstTrackStateType(const raw_type& raw) : m_raw{&raw} {
    assert(m_raw != nullptr);
  }

  // Disable copy constructor
  ConstTrackStateType(const ConstTrackStateType& other) = delete;

  // Delete move constructor
  ConstTrackStateType(ConstTrackStateType&& other) = delete;

  // Disable assignment
  ConstTrackStateType& operator=(const ConstTrackStateType&) = delete;

  // Disable move assignment
  ConstTrackStateType& operator=(ConstTrackStateType&&) = delete;

  /// Return if the bit at position @p pos is 1
  /// @param pos the bit position
  /// @return if the bit at @p pos is one or not
  bool test(std::size_t pos) const {
    std::bitset<kRawBits> bs{*m_raw};
    return bs.test(pos);
  }

  friend std::ostream& operator<<(std::ostream& os,
                                  const ConstTrackStateType& t) {
    std::bitset<kRawBits> bs{*t.m_raw};
    std::bitset<TrackStateFlag::NumTrackStateFlags> trunc;
    for (std::size_t i = 0; i < TrackStateFlag::NumTrackStateFlags; i++) {
      trunc[i] = bs[i];
    }
    // SharedhitMaterialHoleOutlierParameterMeasurement
    os << "SMHOPM=" << trunc;
    return os;
  }

 private:
  friend class TrackStateType;
  const raw_type* m_raw{nullptr};
};

inline TrackStateType& TrackStateType::operator=(
    const ConstTrackStateType& other) {
  *m_raw = *other.m_raw;
  return *this;
}

}  // namespace Acts
