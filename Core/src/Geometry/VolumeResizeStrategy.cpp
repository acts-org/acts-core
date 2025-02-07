// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/VolumeResizeStrategy.hpp"

namespace Acts {

std::ostream& operator<<(std::ostream& os, VolumeResizeStrategy strategy) {
  switch (strategy) {
    case VolumeResizeStrategy::Expand:
      os << "Expand";
      break;
    case VolumeResizeStrategy::Gap:
      os << "Gap";
      break;
    default:
      os << "Unknown";
      break;
  }
  return os;
}

}  // namespace Acts
