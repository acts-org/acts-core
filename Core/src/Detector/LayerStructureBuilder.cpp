// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/LayerStructureBuilder.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/detail/IndexedSurfacesGenerator.hpp"
#include "Acts/Detector/detail/ReferenceGenerators.hpp"
#include "Acts/Detector/detail/SupportSurfacesHelper.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/ProtoAxis.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <set>
#include <stdexcept>
#include <utility>

namespace Acts::Experimental {
class DetectorVolume;
}  // namespace Acts::Experimental

namespace {

/// Check autorange for a given binning
///
/// @param pBinning the proto binning
/// @param extent the extent from which the range is taken
///
void adaptBinningRange(
    std::vector<std::tuple<Acts::ProtoAxis, std::size_t>>& pBinning,
    const Acts::Extent& extent) {
  for (auto& [pb, pe] : pBinning) {
    // Starting values
    const auto& axis = pb.getAxis();
    const auto& edges = axis.getBinEdges();
    double vmin = edges.front();
    double vmax = edges.back();
    // Check if extent overwrites that
    if (extent.constrains(pb.getAxisDirection())) {
      const auto& range = extent.range(pb.getAxisDirection());
      // Patch the edges values from the range
      vmin = range.min();
      vmax = range.max();
    }
    pb.setRange(vmin, vmax);
  }
}

}  // namespace

Acts::Experimental::LayerStructureBuilder::LayerStructureBuilder(
    const Acts::Experimental::LayerStructureBuilder::Config& cfg,
    std::unique_ptr<const Acts::Logger> logger)
    : IInternalStructureBuilder(), m_cfg(cfg), m_logger(std::move(logger)) {
  if (m_cfg.surfacesProvider == nullptr) {
    throw std::invalid_argument(
        "LayerStructureBuilder: surfaces provider is nullptr.");
  }
}

Acts::Experimental::InternalStructure
Acts::Experimental::LayerStructureBuilder::construct(
    const Acts::GeometryContext& gctx) const {
  // Trivialities first: internal volumes
  std::vector<std::shared_ptr<DetectorVolume>> internalVolumes = {};
  ExternalNavigationDelegate internalVolumeUpdater = tryNoVolumes();

  // Print the auxiliary information
  if (!m_cfg.auxiliary.empty()) {
    ACTS_DEBUG(m_cfg.auxiliary);
  }

  // Retrieve the layer surfaces
  InternalNavigationDelegate internalCandidatesUpdater =
      tryAllPortalsAndSurfaces();
  auto internalSurfaces = m_cfg.surfacesProvider->surfaces(gctx);
  ACTS_DEBUG("Building internal layer structure from "
             << internalSurfaces.size() << " provided surfaces.");

  // Check whether support structure is scheduled to be built, and if so
  // collect those that should be assigned to all bins
  std::vector<std::size_t> assignToAll = {};
  if (!m_cfg.supports.empty()) {
    ACTS_DEBUG("Adding " << m_cfg.supports.size() << " support structures.");
    // The surface candidate updator
    for (const auto& support : m_cfg.supports) {
      // Check if the supportsurface has already been built
      if (support.surface != nullptr) {
        ACTS_VERBOSE("- Use provided support surface directly.");
        if (support.assignToAll) {
          assignToAll.push_back(internalSurfaces.size());
          ACTS_VERBOSE("  Support surface is assigned to all bins.");
        }
        internalSurfaces.push_back(support.surface);
        continue;
      }

      // Throw an exception is misconfigured
      if (support.type == Surface::SurfaceType::Other) {
        throw std::invalid_argument(
            "LayerStructureBuilder: support surface type not specified.");
      }
      ACTS_VERBOSE("- Build support of type '"
                   << Acts::Surface::s_surfaceTypeNames[support.type] << "'.");
      if (support.splits > 1u) {
        ACTS_VERBOSE("  Support surface is modelled with " << support.splits
                                                           << " planes.");
      }

      // The support extent
      Extent supportExtent;
      // Let us start with an eventually existing volume extent, but only pick
      // the binning value that are not constrained by the internal surfaces
      for (const auto& bv : allAxisDirections()) {
        if (support.volumeExtent.constrains(bv) &&
            !rangeContainsValue(support.internalConstraints, bv)) {
          ACTS_VERBOSE("  Support surface is constrained by volume extent in "
                       << axisDirectionName(bv));
          supportExtent.set(bv, support.volumeExtent.min(bv),
                            support.volumeExtent.max(bv));
        }
      }

      // Now add the internal constraints
      if (!support.internalConstraints.empty()) {
        // Estimate the extent from the surfaces
        for (const auto& s : internalSurfaces) {
          auto sPolyhedron =
              s->polyhedronRepresentation(gctx, m_cfg.quarterSegments);
          supportExtent.extend(sPolyhedron.extent(),
                               support.internalConstraints);
        }
      }

      // Add cylindrical support
      if (support.type == Surface::SurfaceType::Cylinder) {
        detail::SupportSurfacesHelper::CylindricalSupport cSupport{
            support.offset, support.volumeClearance[AxisDirection::AxisZ],
            support.volumeClearance[AxisDirection::AxisPhi]};
        detail::SupportSurfacesHelper::addSupport(internalSurfaces, assignToAll,
                                                  supportExtent, cSupport,
                                                  support.splits);
      } else if (support.type == Surface::SurfaceType::Disc) {
        // Add disc support
        detail::SupportSurfacesHelper::DiscSupport dSupport{
            support.offset, support.volumeClearance[AxisDirection::AxisR],
            support.volumeClearance[AxisDirection::AxisPhi]};
        detail::SupportSurfacesHelper::addSupport(internalSurfaces, assignToAll,
                                                  supportExtent, dSupport,
                                                  support.splits);
      } else if (support.type == Surface::SurfaceType::Plane) {
        // Set the local coordinates - cyclic permutation
        std::array<AxisDirection, 2> locals = {AxisDirection::AxisX,
                                               AxisDirection::AxisY};
        if (support.pPlacement == AxisDirection::AxisX) {
          locals = {AxisDirection::AxisY, AxisDirection::AxisZ};
        } else if (support.pPlacement == AxisDirection::AxisY) {
          locals = {AxisDirection::AxisZ, AxisDirection::AxisX};
        }
        // Add rectangular support
        detail::SupportSurfacesHelper::RectangularSupport rSupport{
            support.pPlacement, support.offset,
            support.volumeClearance[locals[0u]],
            support.volumeClearance[locals[1u]]};
        detail::SupportSurfacesHelper::addSupport(internalSurfaces, assignToAll,
                                                  supportExtent, rSupport);
      }

      else {
        throw std::invalid_argument(
            "LayerStructureBuilder: support surface type not supported.");
      }
    }
  }

  if (internalSurfaces.size() >= m_cfg.nMinimalSurfaces) {
    // Copy as we might patch it with the surface extent
    auto binnings = m_cfg.binnings;

    if (binnings.empty()) {
      ACTS_DEBUG(
          "No surface binning provided, navigation will be 'tryAll' "
          "(potentially slow).");
    } else {
      // Sort the binning for conventions
      std::ranges::sort(binnings, {}, [](const auto& b) {
        return std::get<ProtoAxis>(b).getAxisDirection();
      });
      // Check if autorange for binning applies
      if (m_cfg.extent.has_value()) {
        ACTS_DEBUG("- adapting the proto binning range to the surface extent.");
        adaptBinningRange(binnings, m_cfg.extent.value());
      }

      // Provide a reference generator
      Acts::Experimental::detail::PolyhedronReferenceGenerator rGenerator;

      // 1D surface binning
      if (binnings.size() == 1) {
        ACTS_DEBUG("- creating a 1D internal binning and portal navigation");
        auto [protoAxis, fillExpansion] = binnings.at(0);
        internalCandidatesUpdater =
            Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
                Experimental::IndexedSurfacesNavigation>(
                gctx, internalSurfaces, rGenerator, protoAxis, fillExpansion,
                assignToAll);
      } else if (binnings.size() == 2u) {
        ACTS_DEBUG("- creating a 2D internal binning and portal navigation");
        auto [protoAxisA, fillExpansionA] = binnings.at(0);
        auto [protoAxisB, fillExpansionB] = binnings.at(1);
        internalCandidatesUpdater =
            Acts::detail::IndexedSurfacesGenerator::createInternalNavigation<
                Experimental::IndexedSurfacesNavigation>(
                gctx, internalSurfaces, rGenerator, protoAxisA, fillExpansionA,
                protoAxisB, fillExpansionB, assignToAll);
      } else {
        throw std::runtime_error(
            "LayerStructureBuilder: only 1D or 2D surface binning "
            "supported.");
      }
    }
  } else {
    ACTS_DEBUG("Only " << internalSurfaces.size() << " surfaces provided, "
                       << "navigation will be 'tryAll'");
    ACTS_DEBUG("Per configuration " << m_cfg.nMinimalSurfaces
                                    << " surfaces are "
                                    << "required to use the surface binning.");
  }

  // Check if everything went ok
  if (!internalCandidatesUpdater.connected()) {
    throw std::runtime_error(
        "LayerStructureBuilder: could not connect surface candidate updator.");
  }

  // Return the internal structure
  return InternalStructure{internalSurfaces, internalVolumes,
                           std::move(internalCandidatesUpdater),
                           std::move(internalVolumeUpdater)};
}
