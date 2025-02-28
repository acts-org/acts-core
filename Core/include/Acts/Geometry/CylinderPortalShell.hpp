// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/PortalShell.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <memory>

namespace Acts {

/// @class CylinderPortalShell
/// Base class for cylinder shaped portal shells, e.g. shells for cylinder
/// volumes
class CylinderPortalShell : public PortalShellBase {
 public:
  using Face = CylinderVolumeBounds::Face;

  using enum CylinderVolumeBounds::Face;

  /// Retrieve the portal associated to the given face. Can be nullptr if unset.
  /// @param face The face to retrieve the portal for
  /// @return The portal associated to the face
  virtual Portal* portal(Face face) = 0;

  /// Retrieve a shared_ptr for the portal associated to the given face. Can be
  /// nullptr if unset.
  /// @param face The face to retrieve the portal for
  /// @return The portal associated to the face
  virtual std::shared_ptr<Portal> portalPtr(Face face) = 0;

  /// Set the portal associated to the given face.
  /// @param portal The portal to set
  /// @param face The face to set the portal
  virtual void setPortal(std::shared_ptr<Portal> portal, Face face) = 0;

  /// @copydoc PortalShellBase::fill
  void fill(TrackingVolume& volume) override;
};

/// Output stream operator for the CylinderPortalShell::Face enum
/// @param os The output stream
/// @param face The face to output
/// @return The output stream
std::ostream& operator<<(std::ostream& os, CylinderPortalShell::Face face);

/// @class SingleCylinderPortalShell
/// This class describes a cylinder shell containing a single volume. The
/// available faces depend on the configuration of the cylinder volume bounds.
/// If a phi sector is configured, the shell will have corresponding portal
/// slots. If the inner radius is non-zero, the shell will have an inner
/// cylinder portal slot.
class SingleCylinderPortalShell : public CylinderPortalShell {
 public:
  using Base = CylinderPortalShell;

  /// Construct a single cylinder portal shell for the given volume
  /// @param volume The volume to create the shell for
  explicit SingleCylinderPortalShell(TrackingVolume& volume);

  /// @copydoc PortalShellBase::size
  std::size_t size() const final;

  /// @copydoc CylinderPortalShell::portal
  Portal* portal(Face face) final;

  /// @copydoc CylinderPortalShell::portalPtr
  std::shared_ptr<Portal> portalPtr(Face face) final;

  /// @copydoc CylinderPortalShell::setPortal
  void setPortal(std::shared_ptr<Portal> portal, Face face) final;

  /// @copydoc PortalShellBase::applyToVolume
  void applyToVolume() override;

  /// @copydoc PortalShellBase::isValid
  bool isValid() const override;

  /// @copydoc PortalShellBase::label
  std::string label() const override;

 private:
  std::array<std::shared_ptr<Portal>, 6> m_portals{};

  TrackingVolume* m_volume;
};

/// @class CylinderStackPortalShell
/// This class describes a cylinder shell containing multiple volumes. The
/// available faces depend on the configuration of the cylinder volume bounds.
/// @note The stack shell currently does not support phi sectors
/// The stack can be oriented along the (local) z or r direction, which drives
/// the stacking. Depending on the direction, portals on the shells of children
/// are merged or fused. Subsequently, portal access respects shared portals
/// between shells. Below is an illustration of a stack in the r direction:
///
///  Fused         +-----------------+
/// portals ----+  |                 |
///   |         |  v           OuterCylinder
///   |  +------+------+
///   |  |      |      |
///   |  |      |      |<--+
///   +--+---+  v      |   |
///      +---+---------+   |
///      |   |         |   |      Shared portal
///      |   |         |<--+---      (grid)
///      |   v         |   |      PositiveDisc
///      +-------------+   |
/// r ^  |             |   |
///   |  |             |<--+
///   |  |             |
///   |  +-------------+       InnerCylinder
///   +----->      ^            (if rMin>0)
///          z     |                 |
///                +-----------------+
///
/// @note The shells must be ordered in the given direction
/// Depending on the stack direction, the portal lookup will return different
/// portals. In the illustration above, the `PositiveDisc` portal is shared
/// among all shells, while the `OuterCylinder` and `InnerCylinder` portals are
/// looked up from the innermost and outermost shell in the r direction.
class CylinderStackPortalShell : public CylinderPortalShell {
 public:
  using SingleShell = SingleCylinderPortalShell;

  /// Construct the portal shell stack from the given shells
  /// @param gctx The geometry context
  /// @param shells The shells to stack
  /// @note The shells must be ordered in the given direction
  /// @param direction The stacking direction
  /// @param logger A logging instance for debugging
  CylinderStackPortalShell(const GeometryContext& gctx,
                           std::vector<CylinderPortalShell*> shells,
                           AxisDirection direction,
                           const Logger& logger = getDummyLogger());

  /// @copydoc PortalShellBase::size
  std::size_t size() const final;

  /// @copydoc CylinderPortalShell::portal
  Portal* portal(Face face) final;

  /// @copydoc CylinderPortalShell::portalPtr
  std::shared_ptr<Portal> portalPtr(Face face) final;

  /// @copydoc CylinderPortalShell::setPortal
  void setPortal(std::shared_ptr<Portal> portal, Face face) final;

  void applyToVolume() override {
    // No-op, because it's a composite portal shell
  }

  /// @copydoc PortalShellBase::isValid
  bool isValid() const override;

  /// @copydoc PortalShellBase::label
  std::string label() const override;

 private:
  AxisDirection m_direction;
  std::vector<CylinderPortalShell*> m_shells;
  bool m_hasInnerCylinder{true};
};

}  // namespace Acts
