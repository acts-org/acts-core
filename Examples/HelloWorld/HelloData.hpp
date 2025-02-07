// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <ostream>
#include <vector>

namespace ActsExamples {

/// An example data object to be shared via the event store.
struct HelloData {
  double x;
  double a;
  double b;
  double t;
};

inline std::ostream& operator<<(std::ostream& os, const HelloData& data) {
  os << data.x << ", " << data.a << ", " << data.b << ", " << data.t;
  return os;
}

using HelloDataCollection = std::vector<HelloData>;

}  // namespace ActsExamples
