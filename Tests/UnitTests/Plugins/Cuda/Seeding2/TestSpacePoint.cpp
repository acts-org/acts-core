// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

// Local include(s).
#include "TestSpacePoint.hpp"

// System include(s).
#include <cmath>
#include <iostream>
#include <limits>

/// Difference allowed on floating point numbers to still be treated equal
static constexpr float allowedDiff = std::numeric_limits<float>::epsilon() * 4;

bool operator==(const TestSpacePoint& a, const TestSpacePoint& b) {
  return ((std::abs(a.m_x - b.m_x) < allowedDiff) &&
          (std::abs(a.m_y - b.m_y) < allowedDiff) &&
          (std::abs(a.m_z - b.m_z) < allowedDiff));
}

std::ostream& operator<<(std::ostream& out, const TestSpacePoint& sp) {
  out << "[surface: " << sp.m_surface << ", x: " << sp.m_x << ", y: " << sp.m_y
      << ", z: " << sp.m_z << "]";
  return out;
}
