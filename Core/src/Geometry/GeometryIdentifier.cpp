// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
