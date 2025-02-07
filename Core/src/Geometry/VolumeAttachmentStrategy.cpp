// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/VolumeAttachmentStrategy.hpp"

namespace Acts {
std::ostream& operator<<(std::ostream& os, VolumeAttachmentStrategy strategy) {
  switch (strategy) {
    case VolumeAttachmentStrategy::First:
      os << "First";
      break;
    case VolumeAttachmentStrategy::Second:
      os << "Second";
      break;
    case VolumeAttachmentStrategy::Midpoint:
      os << "Midpoint";
      break;
    case VolumeAttachmentStrategy::Gap:
      os << "Gap";
      break;
    default:
      os << "Unknown";
      break;
  }
  return os;
}

}  // namespace Acts
