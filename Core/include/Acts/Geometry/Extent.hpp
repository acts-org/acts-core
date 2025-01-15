// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

/// @note This file is foreseen for the `Geometry` module to replace `Extent`

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/RangeXD.hpp"

#include <array>
#include <bitset>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace Acts {

using Envelope = std::array<double, 2>;

constexpr Envelope zeroEnvelope = {0, 0};

/// This struct models a multi-dimensional enveloper along the axis directions
struct ExtentEnvelope {
  /// Access a single envelope configuration
  /// @param aDir the axis definition
  /// @return the envelope
  Envelope& operator[](AxisDirection aDir) {
    return m_values.at(toUnderlying(aDir));
  }

  /// Access a single envelope configuration
  /// @param aDir the axis direction
  /// @return the envelope
  const Envelope& operator[](AxisDirection aDir) const {
    return m_values.at(toUnderlying(aDir));
  }

  /// Constructor from a single envelope that is assigned to all values
  /// @param envelope the envelope to be assigned
  explicit ExtentEnvelope(const Envelope& envelope = zeroEnvelope) {
    for (auto& val : m_values) {
      val = envelope;
    }
  }

  /// Static factory for a zero envelope
  /// @return the zero envelope
  constexpr static ExtentEnvelope Zero() {
    return ExtentEnvelope{{
        zeroEnvelope,
        zeroEnvelope,
        zeroEnvelope,
        zeroEnvelope,
        zeroEnvelope,
        zeroEnvelope,
        zeroEnvelope,
        zeroEnvelope,
        zeroEnvelope,
    }};
  }

  /// Helper struct for designated initializer construction
  struct Arguments {
    Envelope x = zeroEnvelope;
    Envelope y = zeroEnvelope;
    Envelope z = zeroEnvelope;
    Envelope r = zeroEnvelope;
    Envelope phi = zeroEnvelope;
    Envelope rPhi = zeroEnvelope;
    Envelope theta = zeroEnvelope;
    Envelope eta = zeroEnvelope;
    Envelope mag = zeroEnvelope;
  };

  /// Constructor using a helper struct for designated initializaion
  /// @param args the arguments
  constexpr explicit ExtentEnvelope(Arguments&& args) {
    using enum AxisDirection;
    m_values[toUnderlying(AxisX)] = args.x;
    m_values[toUnderlying(AxisY)] = args.y;
    m_values[toUnderlying(AxisZ)] = args.z;
    m_values[toUnderlying(AxisR)] = args.r;
    m_values[toUnderlying(AxisPhi)] = args.phi;
    m_values[toUnderlying(AxisTheta)] = args.theta;
    m_values[toUnderlying(AxisEta)] = args.eta;
    m_values[toUnderlying(AxisMag)] = args.mag;
  }

  /// Comparison operator between envelope sets
  /// @param lhs the left hand side
  /// @param rhs the right hand side
  /// @return true if the envelopes are equal
  friend bool operator==(const ExtentEnvelope& lhs, const ExtentEnvelope& rhs) {
    return lhs.m_values == rhs.m_values;
  }

 private:
  std::array<Envelope, numAxisDirections()> m_values{};
};

/// A class representing the geometric extent of an object in its possible
/// dimensions, these can be all dimensions that are described as AxisDirections
///
/// The extent object can have an optional envelope in all of those values
/// @note that the consistency of the different envelopes is not checked
///
class Extent {
 public:
  /// Constructor with (optional) @param envelope
  explicit Extent(const ExtentEnvelope& envelope = ExtentEnvelope::Zero());

  /// Define a comparison operator
  bool operator==(const Extent& e) const;

  /// Extend with a position vertex
  ///
  /// @param vtx the vertex to be used for extending
  /// @param aDirs the axis directions
  /// @param applyEnv boolean to steer if envelope should be applied
  /// @param fillHistograms is a boolean flag to steer whether the values
  ///        to fill this extent should be stored
  void extend(const Vector3& vtx,
              const std::vector<AxisDirection>& aDirs = allAxisDirections(),
              bool applyEnv = true, bool fillHistograms = false);

  /// Extend with a set of vectors by iterators
  ///
  /// @param start the start iterator of the loop
  /// @param end the end iterator of the loop
  /// @param aDirs the axis directions
  /// @param applyEnv boolean to steer if envelope should be applied
  /// @param fillHistograms is a boolean flag to steer whether the values
  ///        to fill this extent should be stored
  template <typename vector_iterator_t>
  void extend(const vector_iterator_t& start, const vector_iterator_t& end,
              const std::vector<AxisDirection>& aDirs = allAxisDirections(),
              bool applyEnv = true, bool fillHistograms = false) {
    for (vector_iterator_t vIt = start; vIt < end; ++vIt) {
      extend(*vIt, aDirs, applyEnv, fillHistograms);
    }
  }

  /// Extend with another geometric extent, usually pushes the
  /// current range to the boundaries of the rhs extent,
  /// unless the current extent is already bigger.
  ///
  /// @note the extent can also simply set an envelope
  /// which then is applied to the current one
  ///
  /// @param rhs is the other source Extent
  /// @param aDirs the axis directions
  /// @param applyEnv boolean to steer if envelope should be applied
  ///        on the constraint values, if only an envelope is given
  ///        but the value not constraint, then it is always applied
  ///
  /// @note that the histogram values can not be filled in this call
  void extend(const Extent& rhs,
              const std::vector<AxisDirection>& aDirs = allAxisDirections(),
              bool applyEnv = true);

  /// Constrain an extent by another one, this is
  /// - values that are already constrained are not touched
  /// - values not constrained by @param rhs are not touched
  /// - values that are constrained by the external one, but not
  /// by the current one, are touched
  ///
  /// @param envelope an envelope applied to the constrained value
  void addConstrain(const Extent& rhs,
                    const ExtentEnvelope& envelope = ExtentEnvelope::Zero());

  /// Set a range for a dedicated binning value
  ///
  /// @param aDir the axis direction
  /// @param min the minimum parameter
  /// @param max the maximum parameter
  void set(AxisDirection aDir, double min, double max);

  /// Set a min value for a dedicated binning value
  ///
  /// @param aDir the axis direction
  /// @param min the minimum parameter
  void setMin(AxisDirection aDir, double min);

  /// Set a max value for a dedicated binning value
  ///
  /// @param aDir the axis direction
  /// @param max the maximum parameter
  void setMax(AxisDirection aDir, double max);

  /// (re-)Set the envelope
  ///
  /// @param envelope new envelope to be set
  void setEnvelope(const ExtentEnvelope& envelope = ExtentEnvelope::Zero());

  /// Return the individual 1-dimensional range
  ///
  /// @param aDir is the axis direction to be returned
  ///
  /// @return a one dimensional arrange
  auto range(AxisDirection aDir) { return m_range[toUnderlying(aDir)]; }

  /// Return the individual 1-dimensional range
  ///
  /// @param aDir is the axis direction to be returned
  ///
  /// @return a one dimensional arrange
  Range1D<double> range(AxisDirection aDir) const;

  /// Return the N-dimension range
  const RangeXD<numAxisDirections(), double>& range() const;

  /// Return an D-dimensional sub range according to the
  /// the given binvalues
  /// @tparam kSUBDIM the number of sub dimensions
  /// @param axisDirections the axis directions
  /// @return the sub range
  template <unsigned int kSUBDIM>
  RangeXD<kSUBDIM, double> range(
      const std::array<AxisDirection, kSUBDIM>& axisDirections) const {
    RangeXD<kSUBDIM, double> rRange;
    for (auto [i, v] : enumerate(axisDirections)) {
      rRange[i] = range(v);
    }
    return rRange;
  }

  /// Return the envelope - non-const access
  ExtentEnvelope& envelope();

  /// Return the envelope - const access
  const ExtentEnvelope& envelope() const;

  /// Return the histogram store
  ///
  /// The histogram store can be used for automated binning detection
  const std::array<std::vector<double>, numAxisDirections()>& valueHistograms()
      const;

  /// Access the minimum parameter
  ///
  /// @param aDir the axis direction
  double min(AxisDirection aDir) const {
    return m_range[toUnderlying(aDir)].min();
  }

  /// Access the maximum parameter
  ///
  /// @param aDir the axis direction
  double max(AxisDirection aDir) const {
    return m_range[toUnderlying(aDir)].max();
  }

  /// Access the midpoint
  ///
  /// @param aDir the axis direction
  double medium(AxisDirection aDir) const {
    return 0.5 * (m_range[toUnderlying(aDir)].min() +
                  m_range[toUnderlying(aDir)].max());
  }

  /// Access the parameter interval (i.e. the range span)
  ///
  /// @param aDir the axis direction
  double interval(AxisDirection aDir) const {
    return m_range[toUnderlying(aDir)].size();
  }

  /// Contains check
  ///
  /// @param rhs the extent that is check if it is contained
  /// @param aDir is the axis direction, if set to nullopt
  ///               the check on all is done
  ///
  /// @return true if the rhs is contained
  bool contains(const Extent& rhs,
                std::optional<AxisDirection> aDir = std::nullopt) const;

  /// Contains check for a single point
  ///
  /// @param vtx the point that is check if it is contained
  ///
  /// @return true if the rhs is contained
  bool contains(const Vector3& vtx) const;

  /// Intersection checks
  ///
  /// @param rhs the extent that is check for intersection
  /// @param aDir is the axis direction, if set to nulloptr
  ///               the check on all is done
  ///
  /// @return true if the rhs intersects
  bool intersects(const Extent& rhs,
                  std::optional<AxisDirection> aDir = std::nullopt) const;

  /// Check if this object constrains a given direction
  ///
  /// @param aDir is the axis direction
  bool constrains(AxisDirection aDir) const;

  /// Check if this object constrains any direction
  bool constrains() const;

  /// Convert to output stream for screen output
  ///
  /// @param indent indentation for the screen display
  std::string toString(const std::string& indent = "") const;

 private:
  /// A bitset that remembers the constraint values
  std::bitset<numAxisDirections()> m_constrains{0};
  /// The actual range store
  RangeXD<numAxisDirections(), double> m_range;
  /// A potential envelope
  ExtentEnvelope m_envelope = ExtentEnvelope::Zero();
  /// (Optional) Value histograms for bin detection
  std::array<std::vector<double>, numAxisDirections()> m_valueHistograms;
};

inline Range1D<double> Acts::Extent::range(AxisDirection aDir) const {
  return m_range[toUnderlying(aDir)];
}

inline const RangeXD<numAxisDirections(), double>& Extent::range() const {
  return m_range;
}

inline ExtentEnvelope& Extent::envelope() {
  return m_envelope;
}

inline const ExtentEnvelope& Extent::envelope() const {
  return m_envelope;
}

inline const std::array<std::vector<double>, numAxisDirections()>&
Extent::valueHistograms() const {
  return m_valueHistograms;
}

/// Overload of << operator for std::ostream for debug output
std::ostream& operator<<(std::ostream& sl, const Extent& rhs);

}  // namespace Acts
