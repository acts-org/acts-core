// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cstddef>
#include <string>

namespace Acts {

class Portal;
class TrackingVolume;
class GeometryContext;

/// @class PortalShellBase
/// The portal shell of a volume is the set of portals that describes
/// connections "outside" of the volume. Which boundary surfaces of a volume are
/// included in the shell depends on the volume. Portal shells are only used
/// during geometry construction, and are not part of the final geometry
/// description.
///
/// This class is the base class for all portal shells
class PortalShellBase {
 public:
  /// Virtusl destructor
  virtual ~PortalShellBase() = default;

  /// Fill the open slots of the shell with a @c TrivialPortalLink
  /// to the given @p volume.
  /// @param volume The volume to connect
  virtual void fill(TrackingVolume& volume) = 0;

  /// Get the number of portals in the shell. This number depends on the volume
  /// type
  /// @return The number of portals in the shell
  virtual std::size_t size() const = 0;

  /// Instruct the shell to register the portals with the volume, handing over
  /// shared ownership in the process.
  /// @note The target volume depends on the shell type, e.g. composite shells
  ///       like the @c CylinerStackPortalShell register portals to the *correct*
  ///       volumes.
  virtual void applyToVolume() = 0;

  /// Check if a portal is *valid*, e.g. if non of the portals has two
  /// unconnected sides.
  /// @return True if the shell is valid, false otherwise
  virtual bool isValid() const = 0;

  /// Get a label for the portal shell for debugging purposes
  /// @return A label for the portal shell
  virtual std::string label() const = 0;
};

}  // namespace Acts
