// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/PortalShell.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <memory>
#include <vector>

namespace Acts {

/// @class CuboidPortalShell
/// Base class for cuboid shaped portal shells, e.g. shells for cuboid
/// volumes
class CuboidPortalShell : public PortalShellBase {
 public:
  using Face = CuboidVolumeBounds::Face;

  using enum CuboidVolumeBounds::Face;

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

  virtual const Transform3& transform() const = 0;
};

/// Output stream operator for the CuboidPortalShell::Face enum
/// @param os The output stream
/// @param face The face to output
/// @return The output stream
std::ostream& operator<<(std::ostream& os, CuboidPortalShell::Face face);

/// @class SingleCuboidPortalShell
/// This class describes a cuboid shell containing a single volume.
class SingleCuboidPortalShell : public CuboidPortalShell {
 public:
  /// Construct a single cuboid portal shell for the given volume
  /// @param volume The volume to create the shell for
  explicit SingleCuboidPortalShell(TrackingVolume& volume);

  /// @copydoc PortalShellBase::size
  std::size_t size() const final;

  /// @copydoc CuboidPortalShell::portal
  Portal* portal(Face face) final;

  /// @copydoc CuboidPortalShell::portalPtr
  std::shared_ptr<Portal> portalPtr(Face face) final;

  /// @copydoc CuboidPortalShell::setPortal
  void setPortal(std::shared_ptr<Portal> portal, Face face) final;

  /// @copydoc PortalShellBase::applyToVolume
  void applyToVolume() override;

  /// @copydoc PortalShellBase::isValid
  bool isValid() const override;

  /// @copydoc PortalShellBase::label
  std::string label() const override;

  const Transform3& transform() const override {
    return m_volume->transform();
  };

 private:
  std::array<std::shared_ptr<Portal>, 6> m_portals{};

  TrackingVolume* m_volume;
};

/// @class CuboidStackPortalShell
/// This class describes a cuboid shell containing multiple volumes.
class CuboidStackPortalShell final : public CuboidPortalShell {
 public:
  /// Construct the portal shell stack from the given shells
  /// @param gctx The geometry context
  /// @param shells The shells to stack
  /// @note The shells must be ordered in the given direction
  /// @param direction The stacking direction (along x/y/z axis) in local stack coordinates
  /// @param logger A logging instance for debugging
  CuboidStackPortalShell(const GeometryContext& gctx,
                         std::vector<CuboidPortalShell*> shells,
                         AxisDirection direction,
                         const Logger& logger = getDummyLogger());

  /// @copydoc PortalShellBase::size
  std::size_t size() const override;

  /// @copydoc CuboidPortalShell::portal
  Portal* portal(Face face) override;

  /// @copydoc CuboidPortalShell::portalPtr
  std::shared_ptr<Portal> portalPtr(Face face) override;

  /// @copydoc CuboidPortalShell::setPortal
  void setPortal(std::shared_ptr<Portal> portal, Face face) override;

  void applyToVolume() override {
    // No-op, because it's a composite portal shell
  }

  /// @copydoc PortalShellBase::isValid
  bool isValid() const override;

  /// @copydoc PortalShellBase::label
  std::string label() const override;

  /// Return the stack's group transform
  const Transform3& transform() const override;

 private:
  /// Shell stacking direction in local stack coordinates
  AxisDirection m_direction;

  /// The cuboid face positioned first along the stacking direction
  CuboidVolumeBounds::Face m_frontFace = NegativeXFace;
  /// The cuboid face positioned last along the stacking direction
  CuboidVolumeBounds::Face m_backFace = PositiveXFace;
  /// The cuboid faces parallel to the stacking direction
  std::array<CuboidVolumeBounds::Face, 4> m_sideFaces{
      NegativeZFace, PositiveZFace, NegativeYFace, PositiveYFace};

  std::vector<CuboidPortalShell*> m_shells;
};

}  // namespace Acts
