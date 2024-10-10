// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <iosfwd>

namespace Acts {

/// Reimplementation of GeometryIdentifier du to the fact that the default Root
/// C++ version is too old Identifier for geometry nodes.
///
/// Each identifier can be split info the following components:
///
/// - Volumes                 - uses counting given by TrackingGeometry
/// - (Boundary)  Surfaces    - counts through boundary surfaces
/// - (Layer)     Surfaces    - counts confined layers
/// - (Approach)  Surfaces    - counts approach surfaces
/// - (Sensitive) Surfaces    - counts through sensitive surfaces
///
class GeometryIdentifier {
 public:
  using Value = std::uint64_t;

  /// Construct from an already encoded value.
  constexpr GeometryIdentifier(Value encoded) : m_value(encoded) {}
  /// Construct default GeometryIdentifier with all values set to zero.
  GeometryIdentifier() = default;
  GeometryIdentifier(GeometryIdentifier&&) = default;
  GeometryIdentifier(const GeometryIdentifier&) = default;
  ~GeometryIdentifier() = default;
  GeometryIdentifier& operator=(GeometryIdentifier&&) = default;
  GeometryIdentifier& operator=(const GeometryIdentifier&) = default;

  /// Return the encoded value.
  constexpr Value value() const { return m_value; }

  /// Return the volume identifier.
  constexpr Value volume() const { return getBits(volume_mask); }
  /// Return the boundary identifier.
  constexpr Value boundary() const { return getBits(boundary_mask); }
  /// Return the layer identifier.
  constexpr Value layer() const { return getBits(layer_mask); }
  /// Return the approach identifier.
  constexpr Value approach() const { return getBits(approach_mask); }
  /// Return the sensitive identifier.
  constexpr Value sensitive() const { return getBits(sensitive_mask); }

 private:
  // clang-format off
  static constexpr Value volume_mask    = 0xff00000000000000; // 255 volumes
  static constexpr Value boundary_mask  = 0x00ff000000000000; // 255 boundaries
  static constexpr Value layer_mask     = 0x0000fff000000000; // 4095 layers
  static constexpr Value approach_mask  = 0x0000000ff0000000; // 255 approach surfaces
  static constexpr Value sensitive_mask = 0x000000000fffffff; // (2^28)-1 sensitive surfaces
  // clang-format on

  Value m_value = 0;

  /// Extract the bit shift necessary to access the masked values.
  static constexpr int extractShift(Value mask) {
    // use compiler builtin to extract the number of trailing bits from the
    // mask. the builtin should be available on all supported compilers.
    // need unsigned long long version (...ll) to ensure std::uint64_t
    // compatibility.
    // WARNING undefined behaviour for mask == 0 which we should not have.
    return __builtin_ctzll(mask);
  }
  /// Extract the masked bits from the encoded value.
  constexpr Value getBits(Value mask) const {
    return (m_value & mask) >> extractShift(mask);
  }

  friend constexpr bool operator==(GeometryIdentifier lhs,
                                   GeometryIdentifier rhs) {
    return lhs.m_value == rhs.m_value;
  }
  friend constexpr bool operator<(GeometryIdentifier lhs,
                                  GeometryIdentifier rhs) {
    return lhs.m_value < rhs.m_value;
  }
};

std::ostream& operator<<(std::ostream& os, GeometryIdentifier id);

}  // namespace Acts
