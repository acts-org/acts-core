// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

namespace Acts {
class Surface;
}

namespace ActsFatras {

/// Do not select any surface, ever.
struct NoSurface {
  constexpr bool operator()(const Acts::Surface& /*surface*/) const {
    return false;
  }
};

/// Select every surface.
struct EverySurface {
  constexpr bool operator()(const Acts::Surface& /*surface*/) const {
    return true;
  }
};

}  // namespace ActsFatras
