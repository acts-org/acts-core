// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <utility>

namespace Acts {

class Surface;

template <typename T>
concept SurfaceVisitor = requires(T v) {
  { v(std::declval<const Surface*>()) };
};

template <typename T>
concept MutableSurfaceVisitor = requires(T v) {
  { v(std::declval<Surface*>()) };
};

}  // namespace Acts
