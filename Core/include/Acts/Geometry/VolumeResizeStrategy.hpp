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

/// The resize strategy defines how the volumes are resized
enum class VolumeResizeStrategy {
  /// Extend the volume connected to the respective edge to fit the new bounds
  Expand,
  /// Create a gap volume at the respective edge to fit the new bounds
  Gap,
};

std::ostream& operator<<(std::ostream& os, VolumeResizeStrategy strategy);

}  // namespace Acts
