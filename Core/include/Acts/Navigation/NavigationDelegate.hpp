// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Navigation/NavigationStream.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Utilities/Delegate.hpp"

namespace Acts {

class NavigationStream;
class Logger;

/// Struct that serves as the argument to the navigation delegate.
/// It is not supposed to be used as an lvalue.
struct NavigationArguments {
  Vector3 position;
  Vector3 direction;

  BoundaryTolerance tolerance = BoundaryTolerance::None();
};

/// Central alias for the navigation delegate. This type is owning to support
/// (type-erased) navigation delegate chains (i.e. multiple policies).
using NavigationDelegate = OwningDelegate<void(
    const NavigationArguments&, AppendOnlyNavigationStream&, const Logger&)>;

}  // namespace Acts
