// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/CompositePortalLink.hpp"

#include "Acts/Geometry/PortalError.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"

#include <iostream>
#include <stdexcept>

namespace Acts {

CompositePortalLink::CompositePortalLink(std::unique_ptr<PortalLinkBase> a,
                                         std::unique_ptr<PortalLinkBase> b,
                                         BinningValue direction, bool flatten)
    : PortalLinkBase(mergedSurface(a.get(), b.get(), direction)) {
  if (!flatten) {
    m_children.push_back(std::move(a));
    m_children.push_back(std::move(b));

  } else {
    auto handle = [&](std::unique_ptr<PortalLinkBase> link) {
      if (auto* composite = dynamic_cast<CompositePortalLink*>(link.get());
          composite != nullptr) {
        m_children.insert(
            m_children.end(),
            std::make_move_iterator(composite->m_children.begin()),
            std::make_move_iterator(composite->m_children.end()));
      } else {
        m_children.push_back(std::move(link));
      }
    };

    handle(std::move(a));
    handle(std::move(b));
  }
}

Result<const TrackingVolume*> CompositePortalLink::resolveVolume(
    const GeometryContext& gctx, const Vector2& position,
    double tolerance) const {
  // In this overload, we have to go back to global, because the children have
  // their own local coordinate systems
  Vector3 global = m_surface->localToGlobal(gctx, position);
  auto res = resolveVolume(gctx, global, tolerance);
  if (!res.ok()) {
    return res.error();
  }
  return *res;
}

Result<const TrackingVolume*> CompositePortalLink::resolveVolume(
    const GeometryContext& gctx, const Vector3& position,
    double tolerance) const {
  assert(m_surface->isOnSurface(gctx, position, BoundaryTolerance::None(),
                                tolerance));

  for (const auto& child : m_children) {
    if (child->surface().isOnSurface(gctx, position, BoundaryTolerance::None(),
                                     tolerance)) {
      return child->resolveVolume(gctx, position);
    }
  }

  return PortalError::PositionNotOnAnyChildPortalLink;
}

std::shared_ptr<RegularSurface> CompositePortalLink::mergedSurface(
    const PortalLinkBase* a, const PortalLinkBase* b, BinningValue direction) {
  assert(a != nullptr);
  assert(b != nullptr);
  if (a->surface().type() != b->surface().type()) {
    throw std::invalid_argument{"Cannot merge surfaces of different types"};
  }

  if (const auto* cylinderA =
          dynamic_cast<const CylinderSurface*>(&a->surface());
      cylinderA != nullptr) {
    const auto& cylinderB = dynamic_cast<const CylinderSurface&>(b->surface());

    auto [merged, reversed] = cylinderA->mergedWith(cylinderB, direction, true);
    return merged;
  } else if (const auto* discA =
                 dynamic_cast<const DiscSurface*>(&a->surface());
             discA != nullptr) {
    const auto& discB = dynamic_cast<const DiscSurface&>(b->surface());
    auto [merged, reversed] = discA->mergedWith(discB, direction, true);
    return merged;
  } else if (const auto* planeA =
                 dynamic_cast<const PlaneSurface*>(&a->surface());
             planeA != nullptr) {
    throw std::logic_error{"Plane surfaces not implemented yet"};
  } else {
    throw std::invalid_argument{"Unsupported surface type"};
  }
}

std::size_t CompositePortalLink::depth() const {
  std::size_t result = 1;

  for (const auto& child : m_children) {
    if (const auto* composite =
            dynamic_cast<const CompositePortalLink*>(child.get())) {
      result = std::max(result, composite->depth() + 1);
    }
  }

  return result;
}

std::size_t CompositePortalLink::size() const {
  return m_children.size();
}

void CompositePortalLink::toStream(std::ostream& os) const {
  os << "CompositePortalLink";
}

}  // namespace Acts
