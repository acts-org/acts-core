// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/PortalShell.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Logger.hpp"

namespace Acts {
//@class for Trapezoid portal shells, e.g shells for trapezoid volumes

class TrapezoidPortalShell : public PortalShellBase {
 public:
  using Face = TrapezoidVolumeBounds::Face;

  using enum TrapezoidVolumeBounds::Face;

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
std::ostream& operator<<(std::ostream& os, TrapezoidPortalShell::Face face);

/// @class SingleTrapezoidPortalShell
/// This class describes a trapezoid shell containing a single volume.
class SingleTrapezoidPortalShell : public TrapezoidPortalShell {
 public:
  // constructs a single trapezoid shell for the given tracking volume
  // @param volume The volume to create the shell for
  explicit SingleTrapezoidPortalShell(TrackingVolume& volume);

  /// @copydoc PortalShellBase::size
  std::size_t size() const override;

  /// @copydoc TrapezoidPortalShell::portal
  Portal* portal(Face face) override;

  /// @copydoc TrapezoidPortalShell::portalPtr
  std::shared_ptr<Portal> portalPtr(Face face) override;

  /// @copydoc TrapezoidPortalShell::setPortal
  void setPortal(std::shared_ptr<Portal> portal, Face face) override;

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

}  // namespace Acts
