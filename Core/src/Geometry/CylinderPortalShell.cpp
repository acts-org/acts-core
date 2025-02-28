// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/CylinderPortalShell.hpp"

#include "Acts/Geometry/BoundarySurfaceFace.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/Portal.hpp"
#include "Acts/Geometry/PortalLinkBase.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>

#include <boost/algorithm/string/join.hpp>

namespace Acts {

void CylinderPortalShell::fill(TrackingVolume& volume) {
  for (Face face : {PositiveDisc, NegativeDisc, OuterCylinder, InnerCylinder,
                    NegativePhiPlane, PositivePhiPlane}) {
    const auto& portalAtFace = portalPtr(face);
    if (portalAtFace != nullptr) {
      portalAtFace->fill(volume);
      volume.addPortal(portalAtFace);
    }
  }
}

SingleCylinderPortalShell::SingleCylinderPortalShell(TrackingVolume& volume)
    : m_volume{&volume} {
  if (m_volume->volumeBounds().type() != VolumeBounds::BoundsType::eCylinder) {
    throw std::invalid_argument("Invalid volume bounds type");
  }

  const auto& bounds =
      dynamic_cast<const CylinderVolumeBounds&>(m_volume->volumeBounds());

  std::vector<OrientedSurface> orientedSurfaces =
      bounds.orientedSurfaces(m_volume->transform());

  auto handle = [&](Face face, std::size_t from) {
    const auto& source = orientedSurfaces.at(from);
    m_portals.at(toUnderlying(face)) =
        std::make_shared<Portal>(source.direction, source.surface, *m_volume);
  };

  if (orientedSurfaces.size() == 6) {
    // Fully equipped cylinder
    handle(PositiveDisc, positiveFaceXY);
    handle(NegativeDisc, negativeFaceXY);
    handle(OuterCylinder, tubeOuterCover);
    handle(InnerCylinder, tubeInnerCover);
    handle(NegativePhiPlane, tubeSectorNegativePhi);
    handle(PositivePhiPlane, tubeSectorPositivePhi);
  } else if (orientedSurfaces.size() == 5) {
    // Phi sector but no inner cylinder (rMin == 0)
    handle(PositiveDisc, positiveFaceXY);
    handle(NegativeDisc, negativeFaceXY);
    handle(OuterCylinder, tubeOuterCover);
    // Skip inner tube cover, requires offsetting
    handle(NegativePhiPlane, tubeSectorNegativePhi - 1);
    handle(PositivePhiPlane, tubeSectorPositivePhi - 1);
  } else if (orientedSurfaces.size() == 4) {
    // No phi sector but rMin > 0
    handle(PositiveDisc, positiveFaceXY);
    handle(NegativeDisc, negativeFaceXY);
    handle(OuterCylinder, tubeOuterCover);
    handle(InnerCylinder, tubeInnerCover);
  } else if (orientedSurfaces.size() == 3) {
    // No phi sector and rMin == 0
    handle(PositiveDisc, positiveFaceXY);
    handle(NegativeDisc, negativeFaceXY);
    handle(OuterCylinder, tubeOuterCover);
  } else {
    throw std::invalid_argument("Invalid number of oriented surfaces");
  }
}

Portal* SingleCylinderPortalShell::portal(Face face) {
  return portalPtr(face).get();
}

std::shared_ptr<Portal> SingleCylinderPortalShell::portalPtr(Face face) {
  return m_portals.at(toUnderlying(face));
}

void SingleCylinderPortalShell::setPortal(std::shared_ptr<Portal> portal,
                                          Face face) {
  assert(portal != nullptr);
  assert(portal->isValid());
  m_portals.at(toUnderlying(face)) = std::move(portal);
}

std::size_t SingleCylinderPortalShell::size() const {
  std::size_t count = 0;
  std::ranges::for_each(
      m_portals, [&count](const auto& portal) { count += portal ? 1 : 0; });
  return count;
}

void SingleCylinderPortalShell::applyToVolume() {
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

bool SingleCylinderPortalShell::isValid() const {
  return std::ranges::all_of(m_portals, [](const auto& portal) {
    return portal == nullptr || portal->isValid();
  });
};

std::string SingleCylinderPortalShell::label() const {
  std::stringstream ss;
  ss << "CylinderShell(vol=" << m_volume->volumeName() << ")";
  return ss.str();
}

CylinderStackPortalShell::CylinderStackPortalShell(
    const GeometryContext& gctx, std::vector<CylinderPortalShell*> shells,
    AxisDirection direction, const Logger& logger)
    : m_direction{direction}, m_shells{std::move(shells)} {
  ACTS_VERBOSE("Making cylinder stack shell in " << m_direction
                                                 << " direction");
  if (std::ranges::any_of(m_shells,
                          [](const auto* shell) { return shell == nullptr; })) {
    ACTS_ERROR("Invalid shell pointer");
    throw std::invalid_argument("Invalid shell pointer");
  }

  ACTS_VERBOSE(" ~> " << label());

  if (!std::ranges::all_of(
          m_shells, [](const auto* shell) { return shell->isValid(); })) {
    ACTS_ERROR("Invalid shell");
    throw std::invalid_argument("Invalid shell");
  }

  auto merge = [&](Face face) {
    std::vector<std::shared_ptr<Portal>> portals;
    std::ranges::transform(
        m_shells, std::back_inserter(portals),
        [face](auto* shell) { return shell->portalPtr(face); });

    auto merged = std::accumulate(
        std::next(portals.begin()), portals.end(), portals.front(),
        [&](const auto& aPortal,
            const auto& bPortal) -> std::shared_ptr<Portal> {
          assert(aPortal != nullptr);
          assert(bPortal != nullptr);

          return std::make_shared<Portal>(
              Portal::merge(gctx, *aPortal, *bPortal, direction, logger));
        });

    assert(merged != nullptr);
    assert(merged->isValid());

    // reset merged portal on all shells
    for (auto& shell : m_shells) {
      shell->setPortal(merged, face);
    }
  };

  auto fuse = [&](Face faceA, Face faceB) {
    for (std::size_t i = 1; i < m_shells.size(); i++) {
      auto& shellA = m_shells.at(i - 1);
      auto& shellB = m_shells.at(i);
      ACTS_VERBOSE("Fusing " << shellA->label() << " and " << shellB->label());

      auto fused = std::make_shared<Portal>(Portal::fuse(
          gctx, *shellA->portalPtr(faceA), *shellB->portalPtr(faceB), logger));

      assert(fused != nullptr && "Invalid fused portal");
      assert(fused->isValid() && "Fused portal is invalid");

      shellA->setPortal(fused, faceA);
      shellB->setPortal(fused, faceB);

      assert(shellA->isValid() && "Shell A is not valid after fusing");
      assert(shellB->isValid() && "Shell B is not valid after fusing");
    }
  };

  if (direction == AxisDirection::AxisR) {
    ACTS_VERBOSE("Merging portals at positive and negative discs");
    merge(PositiveDisc);
    merge(NegativeDisc);

    ACTS_VERBOSE("Fusing portals at outer and inner cylinders");
    fuse(OuterCylinder, InnerCylinder);

  } else if (direction == AxisDirection::AxisZ) {
    bool allHaveInnerCylinders = std::ranges::all_of(
        m_shells, [](const auto* shell) { return shell->size() == 4; });

    bool noneHaveInnerCylinders = std::ranges::all_of(
        m_shells, [](const auto* shell) { return shell->size() == 3; });

    if (!allHaveInnerCylinders && !noneHaveInnerCylinders) {
      ACTS_ERROR("Invalid inner cylinder configuration");
      throw std::invalid_argument("Invalid inner cylinder configuration");
    }

    m_hasInnerCylinder = allHaveInnerCylinders;

    ACTS_VERBOSE("Merging portals at outer cylinders");
    merge(OuterCylinder);
    assert(isValid() && "Shell is not valid after outer merging");

    if (m_hasInnerCylinder) {
      ACTS_VERBOSE("Merging portals at inner cylinders");
      merge(InnerCylinder);
      assert(isValid() && "Shell is not valid after inner merging");
    }

    ACTS_VERBOSE("Fusing portals at positive and negative discs");
    fuse(PositiveDisc, NegativeDisc);
    assert(isValid() && "Shell is not valid after disc fusing");

  } else {
    throw std::invalid_argument("Invalid direction");
  }

  assert(isValid() && "Shell is not valid after construction");
}

std::size_t CylinderStackPortalShell::size() const {
  return m_hasInnerCylinder ? 4 : 3;
}

Portal* CylinderStackPortalShell::portal(Face face) {
  return portalPtr(face).get();
}

std::shared_ptr<Portal> CylinderStackPortalShell::portalPtr(Face face) {
  if (m_direction == AxisDirection::AxisR) {
    switch (face) {
      case NegativeDisc:
        return m_shells.front()->portalPtr(NegativeDisc);
      case PositiveDisc:
        return m_shells.front()->portalPtr(PositiveDisc);
      case OuterCylinder:
        return m_shells.back()->portalPtr(OuterCylinder);
      case InnerCylinder:
        return m_shells.front()->portalPtr(InnerCylinder);
      case NegativePhiPlane:
        [[fallthrough]];
      case PositivePhiPlane:
        return nullptr;
      default:
        std::stringstream ss;
        ss << "Invalid face: " << face;
        throw std::invalid_argument(ss.str());
    }

  } else {
    switch (face) {
      case NegativeDisc:
        return m_shells.front()->portalPtr(NegativeDisc);
      case PositiveDisc:
        return m_shells.back()->portalPtr(PositiveDisc);
      case OuterCylinder:
        [[fallthrough]];
      case InnerCylinder:
        return m_shells.front()->portalPtr(face);
      case NegativePhiPlane:
        [[fallthrough]];
      case PositivePhiPlane:
        return nullptr;
      default:
        std::stringstream ss;
        ss << "Invalid face: " << face;
        throw std::invalid_argument(ss.str());
    }
  }
}

void CylinderStackPortalShell::setPortal(std::shared_ptr<Portal> portal,
                                         Face face) {
  assert(portal != nullptr);

  if (m_direction == AxisDirection::AxisR) {
    switch (face) {
      case NegativeDisc:
        [[fallthrough]];
      case PositiveDisc:
        for (auto* shell : m_shells) {
          shell->setPortal(portal, face);
        }
        break;
      case OuterCylinder:
        m_shells.back()->setPortal(std::move(portal), OuterCylinder);
        break;
      case InnerCylinder:
        if (!m_hasInnerCylinder) {
          throw std::invalid_argument("Inner cylinder not available");
        }
        m_shells.front()->setPortal(std::move(portal), InnerCylinder);
        break;
      default:
        std::stringstream ss;
        ss << "Invalid face: " << face;
        throw std::invalid_argument(ss.str());
    }

  } else {
    switch (face) {
      case NegativeDisc:
        m_shells.front()->setPortal(std::move(portal), NegativeDisc);
        break;
      case PositiveDisc:
        m_shells.back()->setPortal(std::move(portal), PositiveDisc);
        break;
      case InnerCylinder:
        if (!m_hasInnerCylinder) {
          throw std::invalid_argument("Inner cylinder not available");
        }
        [[fallthrough]];
      case OuterCylinder:
        for (auto* shell : m_shells) {
          shell->setPortal(portal, face);
        }
        break;
      default:
        std::stringstream ss;
        ss << "Invalid face: " << face;
        throw std::invalid_argument(ss.str());
    }
  }
}

bool CylinderStackPortalShell::isValid() const {
  return std::ranges::all_of(m_shells, [](const auto* shell) {
    assert(shell != nullptr);
    return shell->isValid();
  });
}

std::string CylinderStackPortalShell::label() const {
  std::stringstream ss;
  ss << "CylinderStackShell(dir=" << m_direction << ", children=";

  std::vector<std::string> labels;
  std::ranges::transform(m_shells, std::back_inserter(labels),
                         [](const auto* shell) { return shell->label(); });
  ss << boost::algorithm::join(labels, "|");
  ss << ")";
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, CylinderPortalShell::Face face) {
  switch (face) {
    using enum CylinderVolumeBounds::Face;
    case PositiveDisc:
      return os << "PositiveDisc";
    case NegativeDisc:
      return os << "NegativeDisc";
    case OuterCylinder:
      return os << "OuterCylinder";
    case InnerCylinder:
      return os << "InnerCylinder";
    case NegativePhiPlane:
      return os << "NegativePhiPlane";
    case PositivePhiPlane:
      return os << "PositivePhiPlane";
    default:
      return os << "Invalid face";
  }
}

}  // namespace Acts
