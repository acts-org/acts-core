// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <ostream>

namespace Acts {

/// The resize strategy defines how the volumes are resized
enum class VolumeResizeStrategy {
  /// Extend the volume connected to the respective edge to fit the new bounds
  Expand,
  /// Create a gap volume at the respective edge to fit the new bounds
  Gap,
};

std::ostream& operator<<(std::ostream& os, VolumeResizeStrategy strategy);

}  // namespace Acts
