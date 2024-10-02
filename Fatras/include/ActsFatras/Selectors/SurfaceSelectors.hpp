// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
