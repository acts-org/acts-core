// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Helpers.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>

namespace Acts {

/// A constrained step class for the steppers.
///
/// This class is symmetrical for forward and backward propagation. The sign of
/// the propagation direction should not enter here but rather be applied the
/// step is actually taken.
///
/// As simple as this class looks it hides a few very important details:
/// - Overstepping handling. The step size sign will flip if we happened to pass
/// our target.
/// - Convergence handling. Smaller and smaller step sizes have to be used in
/// order to converge on a target.
///
/// Because of the points mentioned above, the update function will always
/// prefer negative step sizes. A side effect of this is that we will propagate
/// in the opposite direction if the target is "behind us".
///
/// The hierarchy is:
/// - Overstepping resolution / backpropagation
/// - Convergence
/// - Step into the void with `std::numeric_limits<double>::max()`
class ConstrainedStep {
 public:
  /// the types of constraints
  /// from navigator - this would be a navigation step
  /// from actor     - this would be an actor condition
  /// from user      - this is user given for what reason ever
  enum class Type : int { Navigator = 0, Actor = 1, User = 2 };

  constexpr ConstrainedStep() = default;

  /// constructor
  /// @param v is the user given initial value
  constexpr explicit ConstrainedStep(double v) { setUser(v); }

  /// set accuracy
  ///
  /// this will set only the accuracy, as this is the most
  /// exposed to the Propagator
  ///
  /// @param v is the new accuracy value
  constexpr void setAccuracy(double v) {
    assert(v > 0 && "ConstrainedStep accuracy must be > 0.");
    // set the accuracy value
    m_accuracy = v;
  }

  /// set user
  ///
  /// @param v is the new user value
  constexpr void setUser(double v) {
    // TODO enable assert; see https://github.com/acts-project/acts/issues/2543
    // assert(v != 0 && "ConstrainedStep user must be != 0.");
    // set the user value
    setValue(Type::User, v);
  }

  /// returns the min step size
  constexpr double value() const {
    double min = *std::min_element(m_values.begin(), m_values.end());
    // accuracy is always positive and therefore handled separately
    double result = std::min(std::abs(min), m_accuracy);
    return std::signbit(min) ? -result : result;
  }

  /// Access a specific value
  ///
  /// @param type is the requested parameter type
  constexpr double value(Type type) const {
    return m_values[toUnderlying(type)];
  }

  /// Access the accuracy value
  constexpr double accuracy() const { return m_accuracy; }

  /// release a certain constraint value
  ///
  /// @param type is the constraint type to be released
  constexpr void release(Type type) { setValue(type, kNotSet); }

  /// release accuracy
  constexpr void releaseAccuracy() { m_accuracy = kNotSet; }

  /// Update the step size of a certain type
  ///
  /// Only navigation and target abortion step size
  /// updates may change the sign due to overstepping
  ///
  /// @param v is the new value to be updated
  /// @param type is the constraint type
  constexpr void update(double v, Type type) {
    // check the current value and set it if appropriate
    // this will also allow signed values due to overstepping
    if (std::abs(v) < std::abs(value(type))) {
      // TODO enable assert; see
      // https://github.com/acts-project/acts/issues/2543
      // assert(value != 0 && "ConstrainedStep user must be != 0.");
      setValue(type, v);
    }
  }

  std::ostream& toStream(std::ostream& os) const {
    // Helper method to avoid unreadable screen output
    auto streamValue = [&](double v) {
      os << std::setw(5);
      if (std::abs(v) == kNotSet) {
        os << (v > 0 ? "+∞" : "-∞");
      } else {
        os << v;
      }
    };

    os << "(";
    streamValue(m_accuracy);
    os << ", ";
    streamValue(value(Type::Navigator));
    os << ", ";
    streamValue(value(Type::Actor));
    os << ", ";
    streamValue(value(Type::User));
    os << ")";

    return os;
  }

  std::string toString() const {
    std::stringstream dstream;
    toStream(dstream);
    return dstream.str();
  }

 private:
  static constexpr auto kNotSet = std::numeric_limits<double>::max();

  /// the step size tuple
  std::array<double, 3> m_values = {kNotSet, kNotSet, kNotSet};
  /// the accuracy value - this can vary up and down given a good step estimator
  double m_accuracy = kNotSet;

  constexpr void setValue(Type type, double v) {
    m_values[toUnderlying(type)] = v;
  }
};

inline std::ostream& operator<<(std::ostream& os, const ConstrainedStep& step) {
  return step.toStream(os);
}

}  // namespace Acts
