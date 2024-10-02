// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Utilities/Delegate.hpp"

#include <memory>
#include <vector>

namespace Acts {
class VolumeBounds;

namespace Experimental {

class DetectorVolume;
class Portal;

/// The Portal generator definition
using PortalGenerator = Delegate<std::vector<std::shared_ptr<Portal>>(
    const Transform3&, const VolumeBounds&,
    const std::shared_ptr<DetectorVolume>&)>;

/// @brief Generator function for creation of portal surfaces
///
/// @param dTransform a contextually resolved transform
/// @param dBounds the detecor volume bounds
/// @param dVolume the reference to the detector volume which generates this volume
///
/// @return a vector of newly created portals with registered inside volume
std::vector<std::shared_ptr<Portal>> generatePortals(
    const Transform3& dTransform, const VolumeBounds& dBounds,
    const std::shared_ptr<DetectorVolume>& dVolume) noexcept(false);

/// Create a default portal generator that connects to the
/// static method.
///
PortalGenerator defaultPortalGenerator();

/// @brief Calls the portal generation and adds registration to sub portals
///
/// This code is split off the PortalGenerator code in order to allow
/// unit testing of the portal generation without detector volume construction
///
/// @param dTransform a contextually resolved transform
/// @param dBounds the detecor volume bounds
/// @param dVolume the reference to the detector volume which generates this volume
///
/// @return a vector of newly created portals with registered inside volume
std::vector<std::shared_ptr<Portal>> generatePortalsUpdateInternals(
    const Transform3& dTransform, const VolumeBounds& dBounds,
    const std::shared_ptr<DetectorVolume>& dVolume) noexcept(false);

/// Create a default portal generator that connects to the
/// static method.
///
/// @note parameters are ignored in this case
PortalGenerator defaultPortalAndSubPortalGenerator();

}  // namespace Experimental
}  // namespace Acts
