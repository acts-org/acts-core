// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Utilities/Delegate.hpp"

namespace Acts {

class Surface;

namespace Experimental {

/// Base class for navigation delegates that handle internal
/// volume navigation updates
class IInternalNavigation {
 public:
  virtual ~IInternalNavigation() = default;
};

/// Declare an updator for the local navigation, i.e. the
/// navigation inside a detector volume. This can be called
/// either directly after a volume switch or in order to update
/// within a volume after some progression
///
/// This delegate dispatches the local navigation action
/// to a dedicated struct or function that is optimised for
/// the given environment.
///
/// @param gctx is the current geometry context
/// @param nState [in,out] is the navigation state to be updated
///
/// @note it relies on the detector volume to be set to the state
/// Memory  managed navigation state updator
using InternalNavigationDelegate =
    OwningDelegate<void(const GeometryContext& gctx, NavigationState& nState),
                   IInternalNavigation>;

/// Base class for external navigation delegates that handle external
/// volume navigation updates
class IExternalNavigation {
 public:
  virtual ~IExternalNavigation() = default;
};

/// Declare a Detctor Volume finding or switching delegate
///
/// @param gctx is the current geometry context
/// @param nState [in, out] is the navigation state to be updated
///
/// @return the new DetectorVolume into which one changes at this switch
using ExternalNavigationDelegate =
    OwningDelegate<void(const GeometryContext& gctx, NavigationState& nState),
                   IExternalNavigation>;

}  // namespace Experimental
}  // namespace Acts
