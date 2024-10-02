// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/CompositePortalLink.hpp"
#include "Acts/Geometry/GridPortalLink.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>
#include <stdexcept>

namespace Acts {

namespace {

template <typename... Args>
std::unique_ptr<GridPortalLink> makeGrid(
    const std::shared_ptr<RegularSurface>& surface, BinningValue direction,
    const Logger& logger, std::tuple<Args...> args, const IAxis* otherAxis,
    bool prepend) {
  // @TODO: PlaneSurface support

  ACTS_VERBOSE("Make resulting merged grid");

  // This is to make it possible to construct Axis with the tuple arguments
  auto axisFactory = []<typename... Ts>(Ts&&... axisArgs) {
    return Axis{std::forward<Ts>(axisArgs)...};
  };

  // Avoid copy-pasting identical code twice below
  auto makeGrid = [&](auto boundaryType) -> std::unique_ptr<GridPortalLink> {
    auto axisArgs = std::tuple_cat(std::tuple{boundaryType}, std::move(args));
    auto merged = std::apply(axisFactory, std::move(axisArgs));

    if (otherAxis == nullptr) {
      // No other axis
      ACTS_VERBOSE("    ~> merged axis: " << merged);
      return GridPortalLink::make(surface, direction, std::move(merged));
    } else {
      return otherAxis->visit(
          [&](const auto& axis) -> std::unique_ptr<GridPortalLink> {
            if (prepend) {
              ACTS_VERBOSE("    ~> other axis (prepend): " << axis);
              ACTS_VERBOSE("    ~> merged axis: " << merged);
              return GridPortalLink::make(surface, axis, std::move(merged));
            } else {
              ACTS_VERBOSE("    ~> merged axis: " << merged);
              ACTS_VERBOSE("    ~> other axis (append): " << axis);
              return GridPortalLink::make(surface, std::move(merged), axis);
            }
          });
    }
  };

  // Check if we're in the cylinder or disc case, and the resulting bounds wrap
  // around and should have closed binning
  if (direction == BinningValue::binPhi || direction == BinningValue::binRPhi) {
    if (const auto* cylinder =
            dynamic_cast<const CylinderSurface*>(surface.get());
        cylinder != nullptr) {
      if (cylinder->bounds().coversFullAzimuth()) {
        return makeGrid(AxisClosed);
      }
    } else if (const auto* disc =
                   dynamic_cast<const DiscSurface*>(surface.get());
               disc != nullptr) {
      const auto& radialBounds =
          dynamic_cast<const RadialBounds&>(disc->bounds());
      if (radialBounds.coversFullAzimuth()) {
        return makeGrid(AxisClosed);
      }
    }
  }

  return makeGrid(AxisBound);
}

std::unique_ptr<GridPortalLink> mergeVariable(
    const std::shared_ptr<RegularSurface>& mergedSurface, const IAxis& axisA,
    const IAxis& axisB, ActsScalar /*tolerance*/, BinningValue direction,
    const Logger& logger, const IAxis* otherAxis, bool prepend) {
  ACTS_VERBOSE("Variable merge: direction is " << direction);

  ACTS_VERBOSE("~> axis a: " << axisA);
  ACTS_VERBOSE("~> axis b: " << axisB);

  std::vector<ActsScalar> binEdges;

  binEdges.reserve(axisA.getNBins() + axisB.getNBins() + 1);

  auto edgesA = axisA.getBinEdges();

  if (direction == BinningValue::binR) {
    ACTS_VERBOSE("Performing asymmetric merge");
    std::ranges::copy(edgesA, std::back_inserter(binEdges));

  } else {
    ACTS_VERBOSE("Performing symmetrized merge");
    ActsScalar halfWidth =
        (axisA.getMax() - axisA.getMin() + axisB.getMax() - axisB.getMin()) /
        2.0;
    ACTS_VERBOSE("    ~> half width: " << halfWidth);

    ActsScalar shift = axisA.getMax() - halfWidth;
    ACTS_VERBOSE("    ~> shift: " << shift);

    std::ranges::transform(edgesA, std::back_inserter(binEdges),
                           [&](ActsScalar edge) { return edge + shift; });
  }

  ActsScalar stitchPoint = binEdges.back();
  auto edgesB = axisB.getBinEdges();
  std::transform(
      std::next(edgesB.begin()), edgesB.end(), std::back_inserter(binEdges),
      [&](ActsScalar edge) { return edge - axisB.getMin() + stitchPoint; });

  return makeGrid(mergedSurface, direction, logger,
                  std::tuple{std::move(binEdges)}, otherAxis, prepend);
}

std::unique_ptr<GridPortalLink> mergeEquidistant(
    const std::shared_ptr<RegularSurface>& mergedSurface, const IAxis& axisA,
    const IAxis& axisB, ActsScalar tolerance, BinningValue direction,
    const Logger& logger, const IAxis* otherAxis, bool prepend) {
  ACTS_VERBOSE("===> potentially equidistant merge: checking bin widths");

  ACTS_VERBOSE("~> axis a: " << axisA);
  ACTS_VERBOSE("~> axis b: " << axisB);

  ActsScalar binsWidthA = (axisA.getMax() - axisA.getMin()) /
                          static_cast<ActsScalar>(axisA.getNBins());
  ActsScalar binsWidthB = (axisB.getMax() - axisB.getMin()) /
                          static_cast<ActsScalar>(axisB.getNBins());

  ACTS_VERBOSE("  ~> binWidths: " << binsWidthA << " vs " << binsWidthB);

  if (std::abs(binsWidthA - binsWidthB) < tolerance) {
    ACTS_VERBOSE("==> binWidths same: " << binsWidthA);

    ActsScalar min = std::numeric_limits<ActsScalar>::signaling_NaN();
    ActsScalar max = std::numeric_limits<ActsScalar>::signaling_NaN();

    if (direction == BinningValue::binR) {
      ACTS_VERBOSE("Performing asymmetric merge");
      min = axisA.getMin();
      max = axisB.getMax();
    } else {
      ACTS_VERBOSE("Performing symmetrized merge");

      ActsScalar halfWidth =
          (axisA.getMax() - axisA.getMin() + axisB.getMax() - axisB.getMin()) /
          2.0;

      min = -halfWidth;
      max = halfWidth;
    }

    return makeGrid(mergedSurface, direction, logger,
                    std::tuple{min, max, axisA.getNBins() + axisB.getNBins()},
                    otherAxis, prepend);

  } else {
    ACTS_VERBOSE("==> binWidths differ: " << binsWidthA << " vs " << binsWidthB
                                          << " ~> variable merge");

    std::unique_ptr<GridPortalLink> mergedPortalLink =
        mergeVariable(mergedSurface, axisA, axisB, tolerance, direction, logger,
                      otherAxis, prepend);
    return mergedPortalLink;
  }
}

std::unique_ptr<GridPortalLink> colinearMerge(
    const std::shared_ptr<RegularSurface>& mergedSurface, const IAxis& axisA,
    const IAxis& axisB, ActsScalar tolerance, BinningValue direction,
    const Logger& logger, const IAxis* otherAxis, bool prepend) {
  AxisType aType = axisA.getType();
  AxisType bType = axisB.getType();
  if (axisA.getBoundaryType() != axisB.getBoundaryType()) {
    ACTS_WARNING("AxisBoundaryTypes are different");
    return nullptr;
  }

  if (axisA.getBoundaryType() != AxisBoundaryType::Bound) {
    ACTS_WARNING("AxisBoundaryType is not Bound, cannot do colinear merge");
    return nullptr;
  }

  // convenience only
  auto mergeVariableLocal = [&] {
    return mergeVariable(mergedSurface, axisA, axisB, tolerance, direction,
                         logger, otherAxis, prepend);
  };

  if (aType == AxisType::Equidistant && bType == AxisType::Equidistant) {
    auto mergedPortalLink =
        mergeEquidistant(mergedSurface, axisA, axisB, tolerance, direction,
                         logger, otherAxis, prepend);
    return mergedPortalLink;
  } else if (aType == AxisType::Variable && bType == AxisType::Variable) {
    ACTS_VERBOSE("===> variable merge");
    auto mergedPortalLink = mergeVariableLocal();
    return mergedPortalLink;
  } else if (aType == AxisType::Equidistant && bType == AxisType::Variable) {
    ACTS_VERBOSE("===> mixed merged");
    auto mergedPortalLink = mergeVariableLocal();
    return mergedPortalLink;
  } else {
    ACTS_VERBOSE("===> mixed merged");
    auto mergedPortalLink = mergeVariableLocal();
    return mergedPortalLink;
  }
}

std::unique_ptr<PortalLinkBase> mergeGridPortals(
    const GridPortalLink* a, const GridPortalLink* b,
    const RegularSurface* surfaceA, const RegularSurface* surfaceB,
    BinningValue loc0, BinningValue loc1, BinningValue direction,
    const Logger& logger) {
  assert(surfaceA != nullptr);
  assert(surfaceB != nullptr);
  assert(surfaceA->type() == surfaceB->type());
  assert(a->dim() == 2 || a->dim() == 1);
  assert(a->dim() == b->dim());

  ACTS_VERBOSE(" - a: " << a->surface().bounds());
  ACTS_VERBOSE(" - b: " << b->surface().bounds());

  // This tolerance is used checking bin equivalence. It's not intended to be
  // user configurable, as we don't foresee cases where the tolerance would have
  // to be adjusteed to the input geometry.
  constexpr auto tolerance = s_onSurfaceTolerance;

  std::shared_ptr<RegularSurface> mergedSurface = nullptr;
  bool reversed = false;

  if (const auto* cylinderA = dynamic_cast<const CylinderSurface*>(surfaceA);
      cylinderA != nullptr) {
    std::tie(mergedSurface, reversed) =
        cylinderA->mergedWith(dynamic_cast<const CylinderSurface&>(*surfaceB),
                              direction, true, logger);
  } else if (const auto* discA = dynamic_cast<const DiscSurface*>(surfaceA);
             discA != nullptr) {
    std::tie(mergedSurface, reversed) = discA->mergedWith(
        dynamic_cast<const DiscSurface&>(*surfaceB), direction, true, logger);
  } else if (const auto* planeA = dynamic_cast<const PlaneSurface*>(surfaceA);
             planeA != nullptr) {
    throw std::logic_error{"Plane surfaces not implemented yet"};
  } else {
    throw std::invalid_argument{"Unsupported surface type"};
  }

  ACTS_VERBOSE("Merged surface: " << mergedSurface->bounds());
  ACTS_VERBOSE("~> reversed? " << std::boolalpha << reversed);

  // Normalize ordering of grid portals and surfaces: a is always at lower
  // range than b
  if (reversed) {
    ACTS_VERBOSE("Swapping grid portals and surfaces after merging");
    std::swap(surfaceA, surfaceB);
    std::swap(a, b);
  }

  // We do this here, because this is the only place where we've normalized
  // the ordering of grids just above: a is always "before" b, so we can
  // iterate over the bins in a unified way to fill the merged grid.
  auto fillGrid = [&](std::unique_ptr<GridPortalLink> merged) {
    if (merged != nullptr) {
      ACTS_VERBOSE("Post processing merged grid: " << merged->grid());
      GridPortalLink::fillMergedGrid(*a, *b, *merged, direction, logger);
    }
    return merged;
  };

  if (a->dim() == 1) {
    ACTS_VERBOSE("Merge two 1D GridPortalLinks on " << surfaceA->name()
                                                    << "s in " << direction);
    if (a->direction() != b->direction()) {
      ACTS_VERBOSE("GridPortalLinks have different directions");
      ACTS_VERBOSE("=> cross merge");

      // Find the one which is already binned in the merging direction
      const auto* aligned = a->direction() == direction ? a : b;
      const auto* alignedSurface =
          a->direction() == direction ? surfaceA : surfaceB;
      const auto* other = a->direction() == direction ? b : a;
      const auto* otherSurface =
          a->direction() == direction ? surfaceB : surfaceA;

      // Extend the aligned one by the other one's axis
      auto aligned2D = aligned->extendTo2d(other->grid().axes().front());
      // Edtend the other one with a single bin
      auto other2D = other->extendTo2d(nullptr);

      assert(aligned2D != nullptr);
      assert(other2D != nullptr);

      ACTS_VERBOSE("Expanded grids:");
      ACTS_VERBOSE(" - aligned: " << aligned2D->grid());
      ACTS_VERBOSE(" - other: " << other2D->grid());

      // @Fix ordering (a,b) should already be in good order, to save us one additional roundtrip
      if (a->direction() == direction) {
        return mergeGridPortals(aligned2D.get(), other2D.get(), alignedSurface,
                                otherSurface, loc0, loc1, direction, logger);
      } else {
        return mergeGridPortals(other2D.get(), aligned2D.get(), otherSurface,
                                alignedSurface, loc0, loc1, direction, logger);
      }
    }

    const auto& axisA = *a->grid().axes().front();
    const auto& axisB = *b->grid().axes().front();

    if (direction == loc0) {
      ACTS_VERBOSE("Grids are binned along " << a->direction());
      if (a->direction() == loc0) {
        ACTS_VERBOSE("=> colinear merge");

        return fillGrid(colinearMerge(mergedSurface, axisA, axisB, tolerance,
                                      loc0, logger, nullptr, false));

      } else {
        ACTS_VERBOSE("=> parallel merge");

        auto a2D = a->extendTo2d(nullptr);
        auto b2D = b->extendTo2d(nullptr);
        assert(a2D != nullptr);
        assert(b2D != nullptr);

        ACTS_VERBOSE("Expanded grids:");
        ACTS_VERBOSE(" - a: " << a2D->grid());
        ACTS_VERBOSE(" - b: " << b2D->grid());

        return mergeGridPortals(a2D.get(), b2D.get(), surfaceA, surfaceB, loc0,
                                loc1, direction, logger);
      }
    } else if (direction == loc1) {
      ACTS_VERBOSE("Grids are binned along " << a->direction());
      if (a->direction() == loc1) {
        ACTS_VERBOSE("=> colinear merge");

        return fillGrid(colinearMerge(mergedSurface, axisA, axisB, tolerance,
                                      loc1, logger, nullptr, false));

      } else {
        ACTS_VERBOSE("=> parallel merge");
        auto a2D = a->extendTo2d(nullptr);
        auto b2D = b->extendTo2d(nullptr);
        assert(a2D != nullptr);
        assert(b2D != nullptr);

        ACTS_VERBOSE("Expanded grids:");
        ACTS_VERBOSE(" - a: " << a2D->grid());
        ACTS_VERBOSE(" - b: " << b2D->grid());
        return mergeGridPortals(a2D.get(), b2D.get(), surfaceA, surfaceB, loc0,
                                loc1, direction, logger);
      }

    } else {
      ACTS_ERROR("Invalid binning direction: " << direction);
      throw std::invalid_argument{"Invalid binning direction"};
    }
  } else {
    ACTS_VERBOSE("Merging two 2D GridPortalLinks on "
                 << surfaceA->name() << "s in " << direction << " direction");

    const auto& loc0AxisA = *a->grid().axes().front();
    const auto& loc1AxisA = *a->grid().axes().back();
    const auto& loc0AxisB = *b->grid().axes().front();
    const auto& loc1AxisB = *b->grid().axes().back();

    if (direction == loc0) {
      ACTS_VERBOSE("=> colinear merge along " << loc0);
      ACTS_VERBOSE("--> Checking if " << loc1 << " axes are identical");

      if (loc1AxisA != loc1AxisB) {
        ACTS_WARNING("    ~> "
                     << loc1
                     << " axes are not identical, falling back to composite "
                        "merging");
        return nullptr;
      }
      ACTS_VERBOSE("    ~> they are!");

      ACTS_VERBOSE("    ~> " << loc1 << " axis: " << loc1AxisA);
      return fillGrid(colinearMerge(mergedSurface, loc0AxisA, loc0AxisB,
                                    tolerance, loc0, logger, &loc1AxisA,
                                    false));

    } else if (direction == loc1) {
      ACTS_VERBOSE("=> colinear merge along " << loc1);
      ACTS_VERBOSE("--> Checking if " << loc0 << " axes are identical");

      if (loc0AxisA != loc0AxisB) {
        ACTS_WARNING("    ~> "
                     << loc0
                     << " axes are not identical, falling back to composite "
                        "merging");
        return nullptr;
      }
      ACTS_VERBOSE("    ~> they are!");

      ACTS_VERBOSE("    ~> " << loc0 << " axis: " << loc0AxisA);
      return fillGrid(colinearMerge(mergedSurface, loc1AxisA, loc1AxisB,
                                    tolerance, loc1, logger, &loc0AxisA, true));

    } else {
      ACTS_ERROR("Invalid binning direction: " << a->direction());
      throw std::invalid_argument{"Invalid binning direction"};
    }
  }
}

std::unique_ptr<PortalLinkBase> mergeGridPortals(const GridPortalLink* a,
                                                 const GridPortalLink* b,
                                                 BinningValue direction,
                                                 const Logger& logger) {
  using enum BinningValue;
  assert(a->dim() == 2 || a->dim() == 1);
  assert(b->dim() == 2 || b->dim() == 1);

  if (a->dim() < b->dim()) {
    return mergeGridPortals(b, a, direction, logger);
  }

  ACTS_VERBOSE("Merging GridPortalLinks along " << direction << ":");
  ACTS_VERBOSE(" - a: " << a->grid() << " along: " << a->direction());
  ACTS_VERBOSE(" - b: " << b->grid() << " along: " << b->direction());

  const auto* cylinder = dynamic_cast<const CylinderSurface*>(&a->surface());
  const auto* disc = dynamic_cast<const DiscSurface*>(&a->surface());

  if (a->dim() == b->dim()) {
    ACTS_VERBOSE("Grid both have same dimension: " << a->dim());

    if (cylinder != nullptr) {
      return mergeGridPortals(
          a, b, cylinder, &dynamic_cast<const CylinderSurface&>(b->surface()),
          binRPhi, binZ, direction, logger);
    } else if (disc != nullptr) {
      return mergeGridPortals(a, b, disc,
                              &dynamic_cast<const DiscSurface&>(b->surface()),
                              binR, binPhi, direction, logger);
    } else {
      // @TODO: Support PlaneSurface
      ACTS_VERBOSE("Surface type is not supported here, falling back");
      return nullptr;
    }
  } else {
    ACTS_VERBOSE("Grids have different dimension, extending rhs to 2D");
    const IAxis* otherAxis = nullptr;

    if (b->direction() == direction) {
      ACTS_VERBOSE("1D grid is binned in merging direction " << direction);
      ACTS_VERBOSE("~> Adding complementary axis");

      if (cylinder != nullptr) {
        otherAxis = direction == binRPhi ? a->grid().axes().back()
                                         : a->grid().axes().front();
      } else if (disc != nullptr) {
        otherAxis = direction == binR ? a->grid().axes().back()
                                      : a->grid().axes().front();
      } else {
        ACTS_VERBOSE("Surface type is not supported here, falling back");
        return nullptr;
      }
    } else {
      ACTS_VERBOSE("1D grid is binned in complementary direction");
    }

    auto b2D = b->extendTo2d(otherAxis);
    ACTS_VERBOSE("-> new grid: " << b2D->grid());
    return mergeGridPortals(a, b2D.get(), direction, logger);
  }
}

}  // namespace

void GridPortalLink::fillMergedGrid(const GridPortalLink& a,
                                    const GridPortalLink& b,
                                    GridPortalLink& merged,
                                    BinningValue direction,
                                    const Logger& logger) {
  ACTS_VERBOSE("Filling merged grid");
  assert(a.dim() == b.dim());
  assert(a.direction() == b.direction());
  const auto locBinsA = a.numLocalBins();
  const auto locBinsB = b.numLocalBins();

  ACTS_VERBOSE("a: " << a.grid());
  ACTS_VERBOSE("b: " << b.grid());
  ACTS_VERBOSE("merged: " << merged.grid());

  if (a.dim() == 1) {
    std::size_t nBinsA = locBinsA.at(0);
    std::size_t nBinsB = locBinsB.at(0);

    for (std::size_t i = 1; i <= nBinsA; ++i) {
      merged.atLocalBins({i}) = a.atLocalBins({i});
    }
    for (std::size_t i = 1; i <= nBinsB; ++i) {
      merged.atLocalBins({nBinsA + i}) = b.atLocalBins({i});
    }
  } else {
    if (a.direction() == direction) {
      std::size_t nBinsA = locBinsA.at(0);
      std::size_t nBinsB = locBinsB.at(0);
      std::size_t nBinsCommon = locBinsB.at(1);

      for (std::size_t i = 1; i <= nBinsA; ++i) {
        for (std::size_t j = 1; j <= nBinsCommon; ++j) {
          merged.atLocalBins({i, j}) = a.atLocalBins({i, j});
        }
      }

      for (std::size_t i = 1; i <= nBinsB; ++i) {
        for (std::size_t j = 1; j <= nBinsCommon; ++j) {
          std::size_t ti = i + nBinsA;
          merged.atLocalBins({ti, j}) = b.atLocalBins({i, j});
        }
      }
    } else {
      std::size_t nBinsA = locBinsA.at(1);
      std::size_t nBinsB = locBinsB.at(1);
      std::size_t nBinsCommon = locBinsB.at(0);

      for (std::size_t i = 1; i <= nBinsCommon; ++i) {
        for (std::size_t j = 1; j <= nBinsA; ++j) {
          merged.atLocalBins({i, j}) = a.atLocalBins({i, j});
        }
      }

      for (std::size_t i = 1; i <= nBinsCommon; ++i) {
        for (std::size_t j = 1; j <= nBinsB; ++j) {
          std::size_t tj = j + nBinsA;
          merged.atLocalBins({i, tj}) = b.atLocalBins({i, j});
        }
      }
    }
  }
}

std::unique_ptr<PortalLinkBase> GridPortalLink::merge(const GridPortalLink& a,
                                                      const GridPortalLink& b,
                                                      BinningValue direction,
                                                      const Logger& logger) {
  ACTS_VERBOSE("Merging two GridPortalLinks");

  checkMergePreconditions(a, b, direction);

  auto merged = mergeGridPortals(&a, &b, direction, logger);
  if (merged == nullptr) {
    ACTS_WARNING("Grid merging failed returning nullptr");
    return nullptr;
  }

  return merged;
}

}  // namespace Acts
