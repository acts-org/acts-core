// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
