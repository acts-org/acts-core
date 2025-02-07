// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Definitions/Direction.hpp"

#include <cstdlib>

std::string Acts::Direction::toString() const {
  switch (m_value) {
    case Value::Positive:
      return "forward";
    case Value::Negative:
      return "backward";
    default:
      assert(false && "Invalid direction (shouldn't happen)");
      std::abort();
  }
}

std::ostream& Acts::operator<<(std::ostream& os, Direction dir) {
  os << dir.toString();
  return os;
}
