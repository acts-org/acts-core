// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/GeometryIdentifier.hpp"

#include <array>
#include <ostream>

std::ostream& Acts::operator<<(std::ostream& os, Acts::GeometryIdentifier id) {
  // zero represents an invalid/undefined identifier
  if (id.value() == 0u) {
    return (os << "undefined");
  }

  static const std::array<const char*, 6> names = {
      "vol=", "bnd=", "lay=", "apr=", "sen=", "ext=",
  };

  const std::array<GeometryIdentifier::Value, 6> levels = {
      id.volume(),   id.boundary(),  id.layer(),
      id.approach(), id.sensitive(), id.extra()};

  bool writeSeparator = false;
  for (auto i = 0u; i < levels.size(); ++i) {
    if (levels[i] != 0u) {
      if (writeSeparator) {
        os << '|';
      }
      os << names[i] << levels[i];
      writeSeparator = true;
    }
  }
  return os;
}

Acts::GeometryIdentifier Acts::GeometryIdentifierHook::decorateIdentifier(
    Acts::GeometryIdentifier identifier,
    const Acts::Surface& /*surface*/) const {
  return identifier;
}
