// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
