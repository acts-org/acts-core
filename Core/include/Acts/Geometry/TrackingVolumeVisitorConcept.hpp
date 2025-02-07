// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <utility>

namespace Acts {

class TrackingVolume;

template <typename T>
concept TrackingVolumeVisitor = requires(T v) {
  { v(std::declval<const TrackingVolume*>()) };
};

}  // namespace Acts
