// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/PortalLinkBase.hpp"

#include "Acts/Geometry/CompositePortalLink.hpp"
#include "Acts/Geometry/GridPortalLink.hpp"
#include "Acts/Geometry/TrivialPortalLink.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"

namespace Acts {

void PortalLinkBase::checkMergePreconditions(const PortalLinkBase& a,
                                             const PortalLinkBase& b,
                                             AxisDirection direction) {
  const auto& surfaceA = a.surface();
  const auto& surfaceB = b.surface();

  throw_assert(&surfaceA != &surfaceB,
               "Cannot merge portals to the same surface");

  throw_assert(surfaceA.type() == surfaceB.type(),
               "Cannot merge portals of different surface types");

  throw_assert(surfaceA.bounds().type() == surfaceB.bounds().type(),
               "Cannot merge portals of different surface bounds");

  if (const auto* cylA = dynamic_cast<const CylinderSurface*>(&surfaceA);
      cylA != nullptr) {
    const auto* cylB = dynamic_cast<const CylinderSurface*>(&surfaceB);
    throw_assert(cylB != nullptr,
                 "Cannot merge CylinderSurface with "
                 "non-CylinderSurface");
    throw_assert(direction == AxisDirection::AxisZ ||
                     direction == AxisDirection::AxisRPhi,
                 "Invalid binning direction: " + axisDirectionName(direction));
  } else if (const auto* discA = dynamic_cast<const DiscSurface*>(&surfaceA);
             discA != nullptr) {
    const auto* discB = dynamic_cast<const DiscSurface*>(&surfaceB);
    throw_assert(discB != nullptr,
                 "Cannot merge DiscSurface with non-DiscSurface");
    throw_assert(direction == AxisDirection::AxisR ||
                     direction == AxisDirection::AxisPhi,
                 "Invalid binning direction: " + axisDirectionName(direction));

    throw_assert(dynamic_cast<const RadialBounds*>(&discA->bounds()) &&
                     dynamic_cast<const RadialBounds*>(&discB->bounds()),
                 "DiscSurface bounds must be RadialBounds");

  } else {
    throw std::logic_error{"Surface type is not supported"};
  }
}

std::unique_ptr<PortalLinkBase> PortalLinkBase::merge(
    std::unique_ptr<PortalLinkBase> a, std::unique_ptr<PortalLinkBase> b,
    AxisDirection direction, const Logger& logger) {
  ACTS_VERBOSE("Merging two arbitrary portals");

  ACTS_VERBOSE(" - a: " << *a);
  ACTS_VERBOSE(" - b: " << *b);

  checkMergePreconditions(*a, *b, direction);

  // Three options:
  // 1. Grid
  // 2. Trivial
  // 3. Composite

  // Grid Grid
  // Grid Trivial
  // Grid Composite
  // Trivial Grid
  // Trivial Trivial
  // Trivial Composite
  // Composite Grid
  // Composite Trivial
  // Composite Composite

  auto gridMerge =
      [&](const GridPortalLink& aGrid,
          const GridPortalLink& bGrid) -> std::unique_ptr<PortalLinkBase> {
    assert(a != nullptr);
    assert(b != nullptr);
    auto merged = GridPortalLink::merge(aGrid, bGrid, direction, logger);
    if (merged == nullptr) {
      return std::make_unique<CompositePortalLink>(std::move(a), std::move(b),
                                                   direction);
    }
    return merged;
  };

  if (const auto* aGrid = dynamic_cast<const GridPortalLink*>(a.get());
      aGrid != nullptr) {
    if (const auto* bGrid = dynamic_cast<const GridPortalLink*>(b.get());
        bGrid != nullptr) {
      ACTS_VERBOSE("Merging two grid portals");
      return gridMerge(*aGrid, *bGrid);

    } else if (const auto* bTrivial =
                   dynamic_cast<const TrivialPortalLink*>(b.get());
               bTrivial != nullptr) {
      ACTS_WARNING(
          "Merging a grid portal with a trivial portal (via composite)");
      return std::make_unique<CompositePortalLink>(std::move(a), std::move(b),
                                                   direction);

    } else if (dynamic_cast<const CompositePortalLink*>(b.get()) != nullptr) {
      ACTS_WARNING("Merging a grid portal with a composite portal");
      return std::make_unique<CompositePortalLink>(std::move(a), std::move(b),
                                                   direction);

    } else {
      throw std::logic_error{"Portal link type is not supported"};
    }

  } else if (const auto* aTrivial =
                 dynamic_cast<const TrivialPortalLink*>(a.get());
             aTrivial != nullptr) {
    if (const auto* bGrid = dynamic_cast<const GridPortalLink*>(b.get());
        bGrid) {
      ACTS_WARNING(
          "Merging a trivial portal with a grid portal (via composite)");
      return std::make_unique<CompositePortalLink>(std::move(a), std::move(b),
                                                   direction);

    } else if (const auto* bTrivial =
                   dynamic_cast<const TrivialPortalLink*>(b.get());
               bTrivial != nullptr) {
      ACTS_VERBOSE("Merging two trivial portals (via composite");
      return std::make_unique<CompositePortalLink>(std::move(a), std::move(b),
                                                   direction);

    } else if (dynamic_cast<const CompositePortalLink*>(b.get()) != nullptr) {
      ACTS_VERBOSE("Merging a trivial portal with a composite portal");
      return std::make_unique<CompositePortalLink>(std::move(a), std::move(b),
                                                   direction);

    } else {
      throw std::logic_error{"Portal link type is not supported"};
    }

  } else if (dynamic_cast<const CompositePortalLink*>(a.get()) != nullptr) {
    if (dynamic_cast<const GridPortalLink*>(b.get()) != nullptr) {
      ACTS_WARNING("Merging a composite portal with a grid portal");
      return std::make_unique<CompositePortalLink>(std::move(a), std::move(b),
                                                   direction);

    } else if (dynamic_cast<const TrivialPortalLink*>(b.get()) != nullptr) {
      ACTS_VERBOSE("Merging a composite portal with a trivial portal");
      return std::make_unique<CompositePortalLink>(std::move(a), std::move(b),
                                                   direction);

    } else if (dynamic_cast<CompositePortalLink*>(b.get()) != nullptr) {
      ACTS_VERBOSE("Merging two composite portals");
      return std::make_unique<CompositePortalLink>(std::move(a), std::move(b),
                                                   direction);

    } else {
      throw std::logic_error{"Portal link type is not supported"};
    }

  } else {
    throw std::logic_error{"Portal link type is not supported"};
  }
}

}  // namespace Acts
