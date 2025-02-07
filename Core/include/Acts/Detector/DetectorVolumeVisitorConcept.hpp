// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <utility>

namespace Acts {

namespace Experimental {
class DetectorVolume;
}

template <typename T>
concept DetectorVolumeVisitor = requires(T v) {
  { v(std::declval<const Experimental::DetectorVolume*>()) };
};

template <typename T>
concept MutableDetectorVolumeVisitor = requires(T v) {
  { v(std::declval<Experimental::DetectorVolume*>()) };
};

}  // namespace Acts
