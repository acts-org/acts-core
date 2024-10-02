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
/// - Step into the void with `std::numeric_limits<Scalar>::max()`
class ConstrainedStep {
 public:
  using Scalar = ActsScalar;

  /// the types of constraints
  /// from actor    - this would be a typical navigation step
  /// from aborter  - this would be a target condition
  /// from user     - this is user given for what reason ever
  enum Type : int { actor = 0, aborter = 1, user = 2 };

  constexpr ConstrainedStep() = default;

  /// constructor from Scalar
  /// @param value is the user given initial value
  constexpr explicit ConstrainedStep(Scalar value) { setUser(value); }

  /// set accuracy by one Scalar
  ///
  /// this will set only the accuracy, as this is the most
  /// exposed to the Propagator
  ///
  /// @param value is the new accuracy value
  constexpr void setAccuracy(Scalar value) {
    assert(value > 0 && "ConstrainedStep accuracy must be > 0.");
    // set the accuracy value
    m_accuracy = value;
  }

  /// set user by one Scalar
  ///
  /// @param value is the new user value
  constexpr void setUser(Scalar value) {
    // TODO enable assert; see https://github.com/acts-project/acts/issues/2543
    // assert(value != 0 && "ConstrainedStep user must be != 0.");
    // set the user value
    m_values[user] = value;
  }

  /// returns the min step size
  constexpr Scalar value() const {
    Scalar min = *std::min_element(m_values.begin(), m_values.end());
    // accuracy is always positive and therefore handled separately
    Scalar result = std::min(std::abs(min), m_accuracy);
    return std::signbit(min) ? -result : result;
  }

  /// Access a specific value
  ///
  /// @param type is the requested parameter type
  constexpr Scalar value(Type type) const { return m_values[type]; }

  /// Access the accuracy value
  constexpr Scalar accuracy() const { return m_accuracy; }

  /// release a certain constraint value
  ///
  /// @param type is the constraint type to be released
  constexpr void release(Type type) { m_values[type] = kNotSet; }

  /// release accuracy
  constexpr void releaseAccuracy() { m_accuracy = kNotSet; }

  /// Update the step size of a certain type
  ///
  /// Only navigation and target abortion step size
  /// updates may change the sign due to overstepping
  ///
  /// @param value is the new value to be updated
  /// @param type is the constraint type
  /// @param releaseStep Allow step size to increase again
  constexpr void update(Scalar value, Type type, bool releaseStep = false) {
    if (releaseStep) {
      release(type);
    }
    // check the current value and set it if appropriate
    // this will also allow signed values due to overstepping
    if (std::abs(value) < std::abs(m_values[type])) {
      // TODO enable assert; see
      // https://github.com/acts-project/acts/issues/2543
      // assert(value != 0 && "ConstrainedStep user must be != 0.");
      m_values[type] = value;
    }
  }

  std::ostream& toStream(std::ostream& os) const {
    // Helper method to avoid unreadable screen output
    auto streamValue = [&](Scalar val) {
      os << std::setw(5);
      if (std::abs(val) == kNotSet) {
        os << (val > 0 ? "+∞" : "-∞");
      } else {
        os << val;
      }
    };

    os << "(";
    streamValue(m_accuracy);
    os << ", ";
    streamValue(value(actor));
    os << ", ";
    streamValue(value(aborter));
    os << ", ";
    streamValue(value(user));
    os << ")";

    return os;
  }

  std::string toString() const {
    std::stringstream dstream;
    toStream(dstream);
    return dstream.str();
  }

 private:
  static constexpr auto kNotSet = std::numeric_limits<Scalar>::max();

  /// the step size tuple
  std::array<Scalar, 3> m_values = {kNotSet, kNotSet, kNotSet};
  /// the accuracy value - this can vary up and down given a good step estimator
  Scalar m_accuracy = kNotSet;
};

inline std::ostream& operator<<(std::ostream& os, const ConstrainedStep& step) {
  return step.toStream(os);
}

}  // namespace Acts
