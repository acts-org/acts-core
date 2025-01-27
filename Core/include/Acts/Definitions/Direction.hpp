// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <cassert>
#include <cstddef>
#include <iosfwd>
#include <string>

namespace Acts {

/// The direction is always with respect to a given momentum, surface normal or
/// other general axes
class Direction final {
 private:
  enum class Value : int {
    Negative = -1,
    Positive = 1,
  };

 public:
  static constexpr Direction Negative() { return Direction{Value::Negative}; }
  static constexpr Direction Positive() { return Direction{Value::Positive}; }

  static constexpr Direction Backward() { return Direction{Value::Negative}; }
  static constexpr Direction Forward() { return Direction{Value::Positive}; }

  static constexpr Direction OppositeNormal() {
    return Direction{Value::Negative};
  }
  static constexpr Direction AlongNormal() {
    return Direction{Value::Positive};
  }

  /// This turns a signed value into a direction. Will assert on zero.
  ///
  /// @param scalar is the signed value
  ///
  /// @return a direction enum
  static constexpr Direction fromScalar(double scalar) {
    assert(scalar != 0);
    return scalar >= 0 ? Positive() : Negative();
  }

  /// This turns a signed value into a direction and 0 will be handled as a
  /// positive direction. Only use this when you are convinced that the 0 case
  /// is properly handled downstream.
  ///
  /// @param scalar is the signed value
  ///
  /// @return a direction enum
  static constexpr Direction fromScalarZeroAsPositive(double scalar) {
    return scalar >= 0 ? Positive() : Negative();
  }

  /// Convert and index [0,1] to a direction e.g. for sorting in
  /// std::array<T, 2u>
  ///
  /// @param index is the direction at input
  static constexpr Direction fromIndex(std::size_t index) {
    return index == 0u ? Negative() : Positive();
  }

  /// Convert dir to index [0,1] which allows to store direction dependent
  /// objects in std::array<T, 2u>
  ///
  /// @return either 0 or 1
  constexpr std::size_t index() const {
    if (m_value == Value::Negative) {
      return 0u;
    }
    return 1u;
  }

  /// Turns the direction into a signed value
  ///
  /// @return a signed value
  constexpr int sign() const { return static_cast<int>(m_value); }

  /// Reverse the direction
  ///
  /// @return an opposite direction
  constexpr Direction invert() const {
    return *this == Positive() ? Negative() : Positive();
  }

  std::string toString() const;

  friend constexpr bool operator==(Direction lhs, Direction rhs) {
    return lhs.m_value == rhs.m_value;
  }

 private:
  explicit constexpr Direction(Value value) : m_value(value) {}

  Value m_value = Value::Positive;
};

std::ostream& operator<<(std::ostream& os, Direction dir);

// Direction * T

constexpr int operator*(Direction dir, int value) {
  return dir.sign() * value;
}

constexpr float operator*(Direction dir, float value) {
  return dir.sign() * value;
}

constexpr double operator*(Direction dir, double value) {
  return dir.sign() * value;
}

inline Acts::Vector3 operator*(Direction dir, const Acts::Vector3& value) {
  return dir.sign() * value;
}

// T * Direction

constexpr int operator*(int value, Direction dir) {
  return value * dir.sign();
}

constexpr float operator*(float value, Direction dir) {
  return value * dir.sign();
}

constexpr double operator*(double value, Direction dir) {
  return value * dir.sign();
}

inline Acts::Vector3 operator*(const Acts::Vector3& value, Direction dir) {
  return value * dir.sign();
}

// T *= Direction

constexpr int operator*=(int& value, Direction dir) {
  value *= dir.sign();
  return value;
}

constexpr float operator*=(float& value, Direction dir) {
  value *= dir.sign();
  return value;
}

constexpr double operator*=(double& value, Direction dir) {
  value *= dir.sign();
  return value;
}

inline Acts::Vector3& operator*=(Acts::Vector3& value, Direction dir) {
  value *= dir.sign();
  return value;
}

}  // namespace Acts
