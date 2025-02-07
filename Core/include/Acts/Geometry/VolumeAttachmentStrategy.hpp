// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <ostream>

namespace Acts {

/// The attachment strategy defines how the volumes are attached
/// Attachment always happens pair-wise
enum class VolumeAttachmentStrategy {
  /// Given two volumes, the *left* one, i.e. the one with the lower **local**
  /// x, y, or z value is extended
  First,
  /// Given two volumes, the *right* one, i.e. the one with the higher
  /// **local** x, y, or z value is extended
  Second,
  /// Given two volumes, the *midpoint* between the two volumes is found
  Midpoint,
  /// A gap volume is created to fit between the two volumes
  Gap,
};

std::ostream& operator<<(std::ostream& os, VolumeAttachmentStrategy strategy);

}  // namespace Acts
