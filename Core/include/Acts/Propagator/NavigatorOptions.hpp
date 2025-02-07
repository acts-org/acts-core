// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
