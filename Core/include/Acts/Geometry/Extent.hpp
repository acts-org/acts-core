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
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/RangeXD.hpp"

#include <array>
#include <bitset>
#include <ostream>
#include <string>
#include <vector>

namespace Acts {

using Envelope = std::array<ActsScalar, 2>;

constexpr Envelope zeroEnvelope = {0, 0};

/// This struct models a multi-dimensional enveloper along the binning values
struct ExtentEnvelope {
  /// Access a single envelope configuration
  /// @param bValue the binning value
  /// @return the envelope
  Envelope& operator[](BinningValue bValue) {
    return m_values.at(toUnderlying(bValue));
  }

  /// Access a single envelope configuration
  /// @param bValue the binning value
  /// @return the envelope
  const Envelope& operator[](BinningValue bValue) const {
    return m_values.at(toUnderlying(bValue));
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
    Envelope h = zeroEnvelope;
    Envelope eta = zeroEnvelope;
    Envelope mag = zeroEnvelope;
  };

  /// Constructor using a helper struct for designated initializaion
  /// @param args the arguments
  constexpr explicit ExtentEnvelope(Arguments&& args) {
    using enum BinningValue;
    m_values[toUnderlying(binX)] = args.x;
    m_values[toUnderlying(binY)] = args.y;
    m_values[toUnderlying(binZ)] = args.z;
    m_values[toUnderlying(binR)] = args.r;
    m_values[toUnderlying(binPhi)] = args.phi;
    m_values[toUnderlying(binH)] = args.h;
    m_values[toUnderlying(binEta)] = args.eta;
    m_values[toUnderlying(binMag)] = args.mag;
  }

  /// Comparison operator between envelope sets
  /// @param lhs the left hand side
  /// @param rhs the right hand side
  /// @return true if the envelopes are equal
  friend bool operator==(const ExtentEnvelope& lhs, const ExtentEnvelope& rhs) {
    return lhs.m_values == rhs.m_values;
  }

 private:
  std::array<Envelope, numBinningValues()> m_values{};
};

/// A class representing the geometric extent of an object in its possible
/// dimensions, these can be all dimensions that are described as BinningValues
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
  /// @param bValues the binning values
  /// @param applyEnv boolean to steer if envelope should be applied
  /// @param fillHistograms is a boolean flag to steer whether the values
  ///        to fill this extent should be stored
  void extend(const Vector3& vtx,
              const std::vector<BinningValue>& bValues = allBinningValues(),
              bool applyEnv = true, bool fillHistograms = false);

  /// Extend with a set of vectors by iterators
  ///
  /// @param start the start iterator of the loop
  /// @param end the end iterator of the loop
  /// @param bValues the binning values
  /// @param applyEnv boolean to steer if envelope should be applied
  /// @param fillHistograms is a boolean flag to steer whether the values
  ///        to fill this extent should be stored
  template <typename vector_iterator_t>
  void extend(const vector_iterator_t& start, const vector_iterator_t& end,
              const std::vector<BinningValue>& bValues = allBinningValues(),
              bool applyEnv = true, bool fillHistograms = false) {
    for (vector_iterator_t vIt = start; vIt < end; ++vIt) {
      extend(*vIt, bValues, applyEnv, fillHistograms);
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
  /// @param bValues the binning values
  /// @param applyEnv boolean to steer if envelope should be applied
  ///        on the constraint values, if only an envelope is given
  ///        but the value not constraint, then it is always applied
  ///
  /// @note that the histogram values can not be filled in this call
  void extend(const Extent& rhs,
              const std::vector<BinningValue>& bValues = allBinningValues(),
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
  /// @param bValue the binning identification
  /// @param min the minimum parameter
  /// @param max the maximum parameter
  void set(BinningValue bValue, ActsScalar min, ActsScalar max);

  /// Set a min value for a dedicated binning value
  ///
  /// @param bValue the binning identification
  /// @param min the minimum parameter
  void setMin(BinningValue bValue, ActsScalar min);

  /// Set a max value for a dedicated binning value
  ///
  /// @param bValue the binning identification
  /// @param max the maximum parameter
  void setMax(BinningValue bValue, ActsScalar max);

  /// (re-)Set the envelope
  ///
  /// @param envelope new envelope to be set
  void setEnvelope(const ExtentEnvelope& envelope = ExtentEnvelope::Zero());

  /// Return the individual 1-dimensional range
  ///
  /// @param bValue is the binning value to be returned
  ///
  /// @return a one dimensional arrange
  auto range(BinningValue bValue) { return m_range[toUnderlying(bValue)]; }

  /// Return the individual 1-dimensional range
  ///
  /// @param bValue is the binning value to be returned
  ///
  /// @return a one dimensional arrange
  Range1D<ActsScalar> range(BinningValue bValue) const;

  /// Return the N-dimension range
  const RangeXD<numBinningValues(), ActsScalar>& range() const;

  /// Return an D-dimensional sub range according to the
  /// the given binvalues
  /// @tparam kSUBDIM the number of sub dimensions
  /// @param binValues the binning values
  /// @return the sub range
  template <unsigned int kSUBDIM>
  RangeXD<kSUBDIM, ActsScalar> range(
      const std::array<BinningValue, kSUBDIM>& binValues) const {
    RangeXD<kSUBDIM, ActsScalar> rRange;
    for (auto [i, v] : enumerate(binValues)) {
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
  const std::array<std::vector<ActsScalar>, numBinningValues()>&
  valueHistograms() const;

  /// Access the minimum parameter
  ///
  /// @param bValue the binning identification
  ActsScalar min(BinningValue bValue) const {
    return m_range[toUnderlying(bValue)].min();
  }

  /// Access the maximum parameter
  ///
  /// @param bValue the binning identification
  ActsScalar max(BinningValue bValue) const {
    return m_range[toUnderlying(bValue)].max();
  }

  /// Access the midpoint
  ///
  /// @param bValue the binning identification
  ActsScalar medium(BinningValue bValue) const {
    return 0.5 * (m_range[toUnderlying(bValue)].min() +
                  m_range[toUnderlying(bValue)].max());
  }

  /// Access the parameter interval (i.e. the range span)
  ///
  /// @param bValue the binning identification
  ActsScalar interval(BinningValue bValue) const {
    return m_range[toUnderlying(bValue)].size();
  }

  /// Contains check
  ///
  /// @param rhs the extent that is check if it is contained
  /// @param bValue is the binning value, if set to nullopt
  ///               the check on all is done
  ///
  /// @return true if the rhs is contained
  bool contains(const Extent& rhs,
                std::optional<BinningValue> bValue = std::nullopt) const;

  /// Contains check for a single point
  ///
  /// @param vtx the point that is check if it is contained
  ///
  /// @return true if the rhs is contained
  bool contains(const Vector3& vtx) const;

  /// Intersection checks
  ///
  /// @param rhs the extent that is check for intersection
  /// @param bValue is the binning value, if set to nulloptr
  ///               the check on all is done
  ///
  /// @return true if the rhs intersects
  bool intersects(const Extent& rhs,
                  std::optional<BinningValue> bValue = std::nullopt) const;

  /// Check if this object constrains a given direction
  ///
  /// @param bValue is the binning value
  bool constrains(BinningValue bValue) const;

  /// Check if this object constrains any direction
  bool constrains() const;

  /// Convert to output stream for screen output
  ///
  /// @param indent indentation for the screen display
  std::string toString(const std::string& indent = "") const;

 private:
  /// A bitset that remembers the constraint values
  std::bitset<numBinningValues()> m_constrains{0};
  /// The actual range store
  RangeXD<numBinningValues(), ActsScalar> m_range;
  /// A potential envelope
  ExtentEnvelope m_envelope = ExtentEnvelope::Zero();
  /// (Optional) Value histograms for bin detection
  std::array<std::vector<ActsScalar>, numBinningValues()> m_valueHistograms;
};

inline Range1D<ActsScalar> Acts::Extent::range(BinningValue bValue) const {
  return m_range[toUnderlying(bValue)];
}

inline const RangeXD<numBinningValues(), ActsScalar>& Extent::range() const {
  return m_range;
}

inline ExtentEnvelope& Extent::envelope() {
  return m_envelope;
}

inline const ExtentEnvelope& Extent::envelope() const {
  return m_envelope;
}

inline const std::array<std::vector<ActsScalar>, numBinningValues()>&
Extent::valueHistograms() const {
  return m_valueHistograms;
}

/// Overload of << operator for std::ostream for debug output
std::ostream& operator<<(std::ostream& sl, const Extent& rhs);

}  // namespace Acts
