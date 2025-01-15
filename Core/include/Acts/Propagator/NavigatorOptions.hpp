// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <functional>

namespace Acts {

class GeometryContext;
class Surface;

struct NavigatorPlainOptions {
  /// NavigatorPlainOptions with context
  explicit NavigatorPlainOptions(const GeometryContext &gctx)
      : geoContext(gctx) {}

  /// Context object for the geometry
  std::reference_wrapper<const GeometryContext> geoContext;

  const Surface *startSurface{};
  const Surface *targetSurface{};
};

}  // namespace Acts
