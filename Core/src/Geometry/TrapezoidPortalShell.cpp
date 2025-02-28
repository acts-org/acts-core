// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/TrapezoidPortalShell.hpp"

#include "Acts/Geometry/BoundarySurfaceFace.hpp"
#include "Acts/Geometry/Portal.hpp"
#include "Acts/Geometry/PortalLinkBase.hpp"
#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"

namespace Acts {

void TrapezoidPortalShell::fill(TrackingVolume& volume) {
  for (Face face : {NegativeZFaceXY, PositiveZFaceXY, TrapezoidFaceAlpha,
                    TrapezoidFaceBeta, NegativeYFaceZX, PositiveYFaceZX}) {
    const auto& portalAtFace = portalPtr(face);
    if (portalAtFace != nullptr) {
      portalAtFace->fill(volume);
      volume.addPortal(portalAtFace);
    }
  }
}

SingleTrapezoidPortalShell::SingleTrapezoidPortalShell(TrackingVolume& volume)
    : m_volume{&volume} {
  if (m_volume->volumeBounds().type() != VolumeBounds::BoundsType::eTrapezoid) {
    throw std::invalid_argument("Invalid volume bounds - not trapezoid");
  }

  const auto& bounds =
      dynamic_cast<const TrapezoidVolumeBounds&>(m_volume->volumeBounds());

  std::vector<OrientedSurface> orientedSurfaces =
      bounds.orientedSurfaces(m_volume->transform());

  for (Face face : {NegativeZFaceXY, PositiveZFaceXY, TrapezoidFaceAlpha,
                    TrapezoidFaceBeta, NegativeYFaceZX, PositiveYFaceZX}) {
    const auto& portalSurface = orientedSurfaces.at(toUnderlying(face));

    m_portals.at(toUnderlying(face)) = std::make_shared<Portal>(
        portalSurface.direction, portalSurface.surface, *m_volume);
  }
}

Portal* SingleTrapezoidPortalShell::portal(Face face) {
  return portalPtr(face).get();
}

std::shared_ptr<Portal> SingleTrapezoidPortalShell::portalPtr(Face face) {
  return m_portals.at(toUnderlying(face));
}

void SingleTrapezoidPortalShell::setPortal(std::shared_ptr<Portal> portal,
                                           Face face) {
  assert(portal != nullptr);
  assert(portal->isValid());
  m_portals.at(toUnderlying(face)) = std::move(portal);
}

std::size_t SingleTrapezoidPortalShell::size() const {
  std::size_t count = 0;
  std::ranges::for_each(
      m_portals, [&count](const auto& portal) { count += portal ? 1 : 0; });
  return count;
}

void SingleTrapezoidPortalShell::applyToVolume() {
  for (std::size_t i = 0; i < m_portals.size(); i++) {
    const auto& portal = m_portals.at(i);
    if (portal != nullptr) {
      if (!portal->isValid()) {
        std::stringstream ss;
        ss << static_cast<Face>(i);
        throw std::runtime_error{"Invalid portal found in shell at " +
                                 ss.str()};
      }
      m_volume->addPortal(portal);
    }
  }
}

bool SingleTrapezoidPortalShell::isValid() const {
  return std::ranges::all_of(m_portals, [](const auto& portal) {
    return portal == nullptr || portal->isValid();
  });
}

std::string SingleTrapezoidPortalShell::label() const {
  std::stringstream ss;
  ss << "TrapezoidShell(vol=" << m_volume->volumeName() << ")";
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, TrapezoidPortalShell::Face face) {
  switch (face) {
    case TrapezoidPortalShell::NegativeZFaceXY:
      return os << "NegativeZFaceXY";
    case TrapezoidPortalShell::PositiveZFaceXY:
      return os << "PositiveZFaceXY";
    case TrapezoidPortalShell::TrapezoidFaceAlpha:
      return os << "TrapezoidFaceAlpha";
    case TrapezoidPortalShell::TrapezoidFaceBeta:
      return os << "TrapezoidFaceBeta";
    case TrapezoidPortalShell::NegativeYFaceZX:
      return os << "NegativeYFaceZX";
    case TrapezoidPortalShell::PositiveYFaceZX:
      return os << "PositiveYFaceZX";
    default:
      return os << "Invalid face";
  }
}

}  // namespace Acts
