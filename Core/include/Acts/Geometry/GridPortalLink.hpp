// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/PortalLinkBase.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"

#include <iosfwd>

namespace Acts {

class IGrid;

template <typename... Axes>
  requires(sizeof...(Axes) <= 2)
class GridPortalLinkT;

/// GridPortalLink implements a subdivided surface where the target volume
/// depends on the position on the surface. The link can be in two states:
/// 1. One-dimensional binning with an associated *direction* to determine which
///    local coordinate to use for the lookup
/// 2. Two-dimensional binning
/// @note The grid dimensions and boundaries are **required** (and checked) to be
///       consistent with the surface bounds.
class GridPortalLink : public PortalLinkBase {
 protected:
  /// Constructor from a surface and a direction, for initialization by derived
  /// class
  /// @param surface The surface
  /// @param direction The binning direction
  GridPortalLink(std::shared_ptr<RegularSurface> surface,
                 AxisDirection direction)
      : PortalLinkBase(std::move(surface)), m_direction(direction) {}

 public:
  /// Factory function for a one-dimensional grid portal link, which allows
  /// using template deduction to figure out the right type
  /// @tparam axis_t The axis type
  /// @param surface The surface
  /// @param direction The binning direction
  /// @param axis The axis to use for the binning
  /// @note The axis boundaries are checked against the bounds of @p surface.
  /// @return A unique pointer to the grid portal link
  template <AxisConcept axis_t>
  static std::unique_ptr<GridPortalLinkT<axis_t>> make(
      std::shared_ptr<RegularSurface> surface, AxisDirection direction,
      axis_t&& axis) {
    using enum AxisDirection;
    if (dynamic_cast<const CylinderSurface*>(surface.get()) != nullptr) {
      if (direction != AxisZ && direction != AxisRPhi) {
        throw std::invalid_argument{"Invalid binning direction"};
      }
    } else if (dynamic_cast<const DiscSurface*>(surface.get()) != nullptr &&
               direction != AxisR && direction != AxisPhi) {
      throw std::invalid_argument{"Invalid binning direction"};
    } else if (dynamic_cast<const PlaneSurface*>(surface.get()) != nullptr &&
               direction != AxisX && direction != AxisY) {
      throw std::invalid_argument{"Invalid binning direction"};
    }

    return std::make_unique<GridPortalLinkT<axis_t>>(
        surface, direction, std::forward<axis_t>(axis));
  }

  /// Factory function for a two-dimensional grid portal link, which allows
  /// using template deduction to figure out the right type.
  /// @tparam axis_1_t The first axis type
  /// @tparam axis_2_t The second axis type
  /// @param surface The surface
  /// @param axis1 The first axis to use for the binning
  /// @param axis2 The second axis to use for the binning
  /// @note The axis boundaries are checked against the bounds of @p surface.
  /// @return A unique pointer to the grid portal link
  template <AxisConcept axis_1_t, AxisConcept axis_2_t>
  static std::unique_ptr<GridPortalLinkT<axis_1_t, axis_2_t>> make(
      std::shared_ptr<RegularSurface> surface, axis_1_t axis1, axis_2_t axis2) {
    std::optional<AxisDirection> direction;
    if (dynamic_cast<const CylinderSurface*>(surface.get()) != nullptr) {
      direction = AxisDirection::AxisRPhi;
    } else if (dynamic_cast<const DiscSurface*>(surface.get()) != nullptr) {
      direction = AxisDirection::AxisR;
    } else if (dynamic_cast<const PlaneSurface*>(surface.get()) != nullptr) {
      direction = AxisDirection::AxisX;
    }

    return std::make_unique<GridPortalLinkT<axis_1_t, axis_2_t>>(
        surface, direction.value(), std::move(axis1), std::move(axis2));
  }

  /// Factory function for an automatically sized one-dimensional grid. This produces a single-bin grid with boundaries taken from the bounds of @p surface along @p direction.
  /// @param surface The surface
  /// @param volume The tracking volume
  /// @param direction The binning direction
  /// @return A unique pointer to the grid portal link
  static std::unique_ptr<GridPortalLink> make(
      const std::shared_ptr<RegularSurface>& surface, TrackingVolume& volume,
      AxisDirection direction);

  /// Merge two grid portal links into a single one. The routine can merge
  /// one-dimenaional, tow-dimensional and mixed links. The merge will try to
  /// preserve equidistant binning in case bin widths match. Otherwise, the
  /// merge falls back to variable binning.
  ///
  /// 1D merge scenarios:
  ///
  /// ```
  /// +----------------------------------------------------------+
  /// |Colinear                                                  |
  /// |                                                          |
  /// | +-------+-------+-------+     +-------+-------+-------+  |
  /// | |       |       |       |     |       |       |       |  |
  /// | |       |       |       |  +  |       |       |       |  |
  /// | |       |       |       |     |       |       |       |  |
  /// | +-------+-------+-------+     +-------+-------+-------+  |
  /// |       +-------+-------+-------+-------+-------+-------+  |
  /// |       |       |       |       |       |       |       |  |
  /// |    =  |       |       |       |       |       |       |  |
  /// |       |       |       |       |       |       |       |  |
  /// |       +-------+-------+-------+-------+-------+-------+  |
  /// |                                                          |
  /// +----------------------------------------------------------+
  /// ```
  ///
  /// Two grid along a shared direction are merged along their shared direction
  ///
  /// ```
  /// +-------------------------------------------------+
  /// |Parallel                                         |
  /// |                                                 |
  /// | +-------+      +-------+     +-------+-------+  |
  /// | |       |      |       |     |       |       |  |
  /// | |       |      |       |     |       |       |  |
  /// | |       |      |       |     |       |       |  |
  /// | +-------+      +-------+     +-------+-------+  |
  /// | |       |      |       |     |       |       |  |
  /// | |       |  +   |       |  =  |       |       |  |
  /// | |       |      |       |     |       |       |  |
  /// | +-------+      +-------+     +-------+-------+  |
  /// | |       |      |       |     |       |       |  |
  /// | |       |      |       |     |       |       |  |
  /// | |       |      |       |     |       |       |  |
  /// | +-------+      +-------+     +-------+-------+  |
  /// |                                                 |
  /// +-------------------------------------------------+
  /// ```
  ///
  /// Two grids along a shared direction a merged in the direction that is
  /// orthogonal to their shared direction.
  ///
  /// ```
  /// +-------------------------------------------+
  /// |Perpendicular                              |
  /// |                                           |
  /// |  +-------+                                |
  /// |  |       |                                |
  /// |  |       |                                |
  /// |  |       |                                |
  /// |  +-------+     +-------+-------+-------+  |
  /// |  |       |     |       |       |       |  |
  /// |  |       |  +  |       |       |       |  |
  /// |  |       |     |       |       |       |  |
  /// |  +-------+     +-------+-------+-------+  |
  /// |  |       |                                |
  /// |  |       |                                |
  /// |  |       |     +-------+-------+-------+  |
  /// |  +-------+     |       |       |       |  |
  /// |                |       |       |       |  |
  /// |                |       |       |       |  |
  /// |                +-------+-------+-------+  |
  /// |                |       |       |       |  |
  /// |             =  |       |       |       |  |
  /// |                |       |       |       |  |
  /// |                +-------+-------+-------+  |
  /// |                |       |       |       |  |
  /// |                |       |       |       |  |
  /// |                |       |       |       |  |
  /// |                +-------+-------+-------+  |
  /// |                                           |
  /// +-------------------------------------------+
  /// ```
  ///
  /// Two grids whose directions are not shared are merged (ordering does not
  /// matter here). The routine will expand one of the grids to match the
  /// other's binning, by subdividing the grid in the as-of-yet unbinned
  /// direction, while filling all bins with the original bin contents.
  /// Afterwards, a conventional mixed-dimension merge is performed.
  ///
  /// Mixed merge scenarios
  /// The order is normalized by always taking the 2D grid as the left hand
  /// side. The 1D grid is expanded to match the binning in the as-of-yet
  /// unbinned direction with the binning taken from the 2D grid.
  ///
  /// ```
  /// +-----------------------------------------+
  /// |2D + 1D                                  |
  /// |                                         |
  /// | +-------+-------+     +-------+-------+ |
  /// | |       |       |     |       |       | |
  /// | |       |       |     |       |       | |
  /// | |       |       |     |       |       | |
  /// | +-------+-------+     +-------+-------+ |
  /// | |       |       |     |       |       | |
  /// | |       |       |     |       |       | |
  /// | |       |       |     |       |       | |
  /// | +-------+-------+  =  +-------+-------+ |
  /// | |       |       |     |       |       | |
  /// | |       |       |     |       |       | |
  /// | |       |       |     |       |       | |
  /// | +-------+-------+     +-------+-------+ |
  /// |         +             |       |       | |
  /// | +-------+-------+     |       |       | |
  /// | |       |       |     |       |       | |
  /// | |       |       |     +-------+-------+ |
  /// | |       |       |                       |
  /// | +-------+-------+                       |
  /// +-----------------------------------------+
  /// ```
  ///
  /// ```
  /// +--------------------------------------------------------------+
  /// |2D + 1D                                                       |
  /// |                                                              |
  /// | +-------+-------+    +-------+     +-------+-------+-------+ |
  /// | |       |       |    |       |     |       |       |       | |
  /// | |       |       |    |       |     |       |       |       | |
  /// | |       |       |    |       |     |       |       |       | |
  /// | +-------+-------+    +-------+     +-------+-------+-------+ |
  /// | |       |       |    |       |     |       |       |       | |
  /// | |       |       |  + |       |  =  |       |       |       | |
  /// | |       |       |    |       |     |       |       |       | |
  /// | +-------+-------+    +-------+     +-------+-------+-------+ |
  /// | |       |       |    |       |     |       |       |       | |
  /// | |       |       |    |       |     |       |       |       | |
  /// | |       |       |    |       |     |       |       |       | |
  /// | +-------+-------+    +-------+     +-------+-------+-------+ |
  /// |                                                              |
  /// +--------------------------------------------------------------+
  /// ```
  ///
  /// 2D merges
  /// The grids need to already share a common axis. If that is not the case,
  /// the merge routine returns a `nullptr`. This can be handled by composite
  /// merging as a fallback if needed.
  /// Ordering and direction does not matter here.
  ///
  /// ```
  /// +-----------------------------------------+
  /// |2D + 2D                                  |
  /// |                                         |
  /// | +-------+-------+     +-------+-------+ |
  /// | |       |       |     |       |       | |
  /// | |       |       |     |       |       | |
  /// | |       |       |     |       |       | |
  /// | +-------+-------+     +-------+-------+ |
  /// | |       |       |     |       |       | |
  /// | |       |       |  +  |       |       | |
  /// | |       |       |     |       |       | |
  /// | +-------+-------+     +-------+-------+ |
  /// | |       |       |     |       |       | |
  /// | |       |       |     |       |       | |
  /// | |       |       |     |       |       | |
  /// | +-------+-------+     +-------+-------+ |
  /// |                                         |
  /// |       +-------+-------+-------+-------+ |
  /// |       |       |       |       |       | |
  /// |       |       |       |       |       | |
  /// |       |       |       |       |       | |
  /// |       +-------+-------+-------+-------+ |
  /// |       |       |       |       |       | |
  /// |   =   |       |       |       |       | |
  /// |       |       |       |       |       | |
  /// |       +-------+-------+-------+-------+ |
  /// |       |       |       |       |       | |
  /// |       |       |       |       |       | |
  /// |       |       |       |       |       | |
  /// |       +-------+-------+-------+-------+ |
  /// |                                         |
  /// +-----------------------------------------+
  /// ```
  ///
  /// @param a The first grid portal link
  /// @param b The second grid portal link
  /// @param direction The merging direction
  /// @param logger The logger to use for messages
  /// @return The merged grid portal link or nullptr if grid merging did not succeed
  /// @note The returned pointer can be nullptr, if the grids
  ///       given are not mergeable, because their binnings are
  ///       not compatible. This is not an error case, and needs
  ///       to be handled by th caller! Invalid input is handled
  ///       via exceptions.
  static std::unique_ptr<PortalLinkBase> merge(
      const GridPortalLink& a, const GridPortalLink& b, AxisDirection direction,
      const Logger& logger = getDummyLogger());

  /// Return the associated grid in a type-erased form
  /// @return The grid
  virtual const IGrid& grid() const = 0;

  /// Return the associated grid in a type-erased form
  /// @return The grid
  virtual IGrid& grid() = 0;

  /// Set the volume on all grid bins
  /// @param volume The volume to set
  virtual void setVolume(TrackingVolume* volume) = 0;

  /// Get the number of dimensions of the grid
  /// @return The number of dimensions
  virtual unsigned int dim() const = 0;

  /// Expand a 1D grid to a 2D one, by using the provided axis along the
  /// *missing* direction.
  /// @param other The axis to use for the missing direction,
  ///              can be null for auto determination
  /// @return A unique pointer to the 2D grid portal link
  virtual std::unique_ptr<GridPortalLink> extendTo2d(
      const IAxis* other) const = 0;

  /// The binning direction of the grid
  /// @note For 2D grids, this will always be the loc0
  ///       direction, depending on the surface type.
  /// @return The binning direction
  AxisDirection direction() const { return m_direction; }

  /// Helper function to fill the bin contents after merging.
  /// This called by the merging routine, and requires access to the internal
  /// grid state.
  /// @param a The first grid portal link
  /// @param b The second grid portal link
  /// @param merged The merged grid portal link
  /// @param direction The merging direction
  /// @param logger The logger to use for messages
  static void fillMergedGrid(const GridPortalLink& a, const GridPortalLink& b,
                             GridPortalLink& merged, AxisDirection direction,
                             const Logger& logger);

  /// Helper function that prints a textual representation of the grid with the
  /// volume names.
  /// @param os The output stream
  void printContents(std::ostream& os) const;

 protected:
  /// Helper function to check consistency for grid on a cylinder surface
  /// @param cyl The cylinder surface
  void checkConsistency(const CylinderSurface& cyl) const;

  /// Helper function to check consistency for grid on a disc surface
  /// @param disc The disc surface
  void checkConsistency(const DiscSurface& disc) const;

  /// Helper function to check consistency for grid on a plane surface
  /// @param plane The plane surface
  void checkConsistency(const PlaneSurface& plane) const;

  /// Expand a 1D grid to a 2D one for a cylinder surface
  /// @param surface The cylinder surface
  /// @param other The axis to use for the missing direction,
  ///              can be null for auto determination
  /// @return A unique pointer to the 2D grid portal link
  std::unique_ptr<GridPortalLink> extendTo2dImpl(
      const std::shared_ptr<CylinderSurface>& surface,
      const IAxis* other) const;

  /// Expand a 1D grid to a 2D one for a disc surface
  /// @param surface The disc surface
  /// @param other The axis to use for the missing direction,
  ///              can be null for auto determination
  /// @return A unique pointer to the 2D grid portal link
  std::unique_ptr<GridPortalLink> extendTo2dImpl(
      const std::shared_ptr<DiscSurface>& surface, const IAxis* other) const;

  /// Expand a 1D grid to a 2D one for a plane surface
  /// @param surface The plane surface
  /// @param other The axis to use for the missing direction,
  ///              can be null for auto determination
  /// @return A unique pointer to the 2D grid portal link
  std::unique_ptr<GridPortalLink> extendTo2dImpl(
      const std::shared_ptr<PlaneSurface>& surface, const IAxis* other) const;

  /// Helper enum to declare which local direction to fill
  enum class FillDirection {
    loc0,
    loc1,
  };

  /// Helper function to fill a 2D grid from a 1D grid, by extending all
  /// bins along the different direction.
  /// @param dir The direction to fill
  /// @param grid1d The 1D grid
  /// @param grid2d The 2D grid
  static void fillGrid1dTo2d(FillDirection dir, const GridPortalLink& grid1d,
                             GridPortalLink& grid2d);

 private:
  AxisDirection m_direction;
};

/// Concrete class deriving from @c GridPortalLink that boxes a concrete grid for lookup.
/// @tparam Axes The axis types of the grid
template <typename... Axes>
  requires(sizeof...(Axes) <= 2)
class GridPortalLinkT : public GridPortalLink {
 public:
  /// The internal grid type
  using GridType = Grid<const TrackingVolume*, Axes...>;

  /// The dimension of the grid
  static constexpr std::size_t DIM = sizeof...(Axes);

  /// Constructor from a surface, axes and direction
  /// @param surface The surface
  /// @param direction The binning direction
  /// @param axes The axes for the grid
  /// @note The axes are checked for consistency with the bounds of @p surface.
  GridPortalLinkT(std::shared_ptr<RegularSurface> surface,
                  AxisDirection direction, Axes&&... axes)
      : GridPortalLink(std::move(surface), direction),
        m_grid(std::tuple{std::move(axes)...}) {
    using enum AxisDirection;

    if (const auto* cylinder =
            dynamic_cast<const CylinderSurface*>(m_surface.get())) {
      checkConsistency(*cylinder);

      if (direction == AxisRPhi) {
        m_projection = &projection<CylinderSurface, AxisRPhi>;
      } else if (direction == AxisZ) {
        m_projection = &projection<CylinderSurface, AxisZ>;
      } else {
        throw std::invalid_argument{"Invalid binning direction"};
      }

    } else if (const auto* disc =
                   dynamic_cast<const DiscSurface*>(m_surface.get())) {
      checkConsistency(*disc);

      if (direction == AxisR) {
        m_projection = &projection<DiscSurface, AxisR>;
      } else if (direction == AxisDirection::AxisPhi) {
        m_projection = &projection<DiscSurface, AxisPhi>;
      } else {
        throw std::invalid_argument{"Invalid binning direction"};
      }
    } else if (const auto* plane =
                   dynamic_cast<const PlaneSurface*>(m_surface.get())) {
      checkConsistency(*plane);

      if (direction == AxisX) {
        m_projection = &projection<PlaneSurface, AxisX>;
      } else if (direction == AxisDirection::AxisY) {
        m_projection = &projection<PlaneSurface, AxisY>;
      } else {
        throw std::invalid_argument{"Invalid binning direction"};
      }

    } else {
      throw std::logic_error{"Surface type is not supported"};
    }
  }

  /// Get the grid
  /// @return The grid
  const GridType& grid() const override { return m_grid; }

  /// Get the grid
  /// @return The grid
  GridType& grid() override { return m_grid; }

  /// Get the number of dimensions of the grid
  /// @return The number of dimensions
  unsigned int dim() const override { return DIM; }

  /// Prints an identification to the output stream
  /// @param os The output stream
  void toStream(std::ostream& os) const override {
    os << "GridPortalLink<dim=" << dim() << ">";
  }

  /// Makes a 2D grid from a 1D grid by extending it using an optional axis
  /// @param other The axis to use for the missing direction,
  ///              can be null for auto determination
  /// @return A unique pointer to the 2D grid portal link
  std::unique_ptr<GridPortalLink> extendTo2d(
      const IAxis* other) const override {
    if constexpr (DIM == 2) {
      return std::make_unique<GridPortalLinkT<Axes...>>(*this);
    } else {
      if (auto cylinder =
              std::dynamic_pointer_cast<CylinderSurface>(m_surface)) {
        return extendTo2dImpl(cylinder, other);
      } else if (auto disc =
                     std::dynamic_pointer_cast<DiscSurface>(m_surface)) {
        return extendTo2dImpl(disc, other);
      } else if (auto plane =
                     std::dynamic_pointer_cast<PlaneSurface>(m_surface)) {
        return extendTo2dImpl(plane, other);
      } else {
        throw std::logic_error{
            "Surface type is not supported (this should not happen)"};
      }
    }
  }

  /// Set the volume on all grid bins
  /// @param volume The volume to set
  void setVolume(TrackingVolume* volume) override {
    auto loc = m_grid.numLocalBins();
    if constexpr (GridType::DIM == 1) {
      for (std::size_t i = 1; i <= loc[0]; i++) {
        m_grid.atLocalBins({i}) = volume;
      }
    } else {
      for (std::size_t i = 1; i <= loc[0]; i++) {
        for (std::size_t j = 1; j <= loc[1]; j++) {
          m_grid.atLocalBins({i, j}) = volume;
        }
      }
    }
  }

  /// Lookup the tracking volume at a position on the surface
  /// @param gctx The geometry context
  /// @param position The local position
  /// @param tolerance The tolerance for the lookup
  /// @note The position is required to be on the associated surface
  /// @return The tracking volume (can be null)
  Result<const TrackingVolume*> resolveVolume(
      const GeometryContext& gctx, const Vector3& position,
      double tolerance = s_onSurfaceTolerance) const override {
    auto res = m_surface->globalToLocal(gctx, position, tolerance);
    if (!res.ok()) {
      return res.error();
    }

    const Vector2& local = *res;
    return resolveVolume(gctx, local, tolerance);
  }

  /// Lookup the tracking volume at a position on the surface
  /// @param position The local position
  /// @note The position is required to be on the associated surface
  /// @return The tracking volume (can be null)
  Result<const TrackingVolume*> resolveVolume(
      const GeometryContext& /*gctx*/, const Vector2& position,
      double /*tolerance*/ = s_onSurfaceTolerance) const override {
    throw_assert(surface().insideBounds(position, BoundaryTolerance::None()),
                 "Checking volume outside of bounds");
    return m_grid.atPosition(m_projection(position));
  }

 private:
  /// Helper function that's assigned to project from the 2D local position to a
  /// possible 1D grid.
  template <class surface_t, AxisDirection direction>
  static ActsVector<DIM> projection(const Vector2& position) {
    using enum AxisDirection;
    if constexpr (DIM == 2) {
      return position;
    } else {
      if constexpr (std::is_same_v<surface_t, CylinderSurface>) {
        static_assert(direction == AxisRPhi || direction == AxisZ,
                      "Invalid binning direction");

        if constexpr (direction == AxisRPhi) {
          return ActsVector<1>{position[0]};
        } else if constexpr (direction == AxisZ) {
          return ActsVector<1>{position[1]};
        }
      } else if constexpr (std::is_same_v<surface_t, DiscSurface>) {
        static_assert(direction == AxisR || direction == AxisPhi,
                      "Invalid binning direction");

        if constexpr (direction == AxisR) {
          return ActsVector<1>{position[0]};
        } else if constexpr (direction == AxisPhi) {
          return ActsVector<1>{position[1]};
        }
      } else if constexpr (std::is_same_v<surface_t, PlaneSurface>) {
        static_assert(direction == AxisX || direction == AxisY,
                      "Invalid binning direction");

        if constexpr (direction == AxisX) {
          return ActsVector<1>{position[0]};
        } else if constexpr (direction == AxisY) {
          return ActsVector<1>{position[1]};
        }
      }
    }
  }

  GridType m_grid;

  /// Stores a function pointer that can project from 2D to 1D if needed
  ActsVector<DIM> (*m_projection)(const Vector2& position);
};

}  // namespace Acts
