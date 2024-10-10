// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/LayerStructureBuilder.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/ProtoBinning.hpp"
#include "Acts/Detector/detail/IndexedSurfacesGenerator.hpp"
#include "Acts/Detector/detail/ReferenceGenerators.hpp"
#include "Acts/Detector/detail/SupportSurfacesHelper.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"
#include "Acts/Utilities/Helpers.hpp"

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
void adaptBinningRange(std::vector<Acts::Experimental::ProtoBinning>& pBinning,
                       const Acts::Extent& extent) {
  for (auto& pb : pBinning) {
    // Starting values
    Acts::ActsScalar vmin = pb.edges.front();
    Acts::ActsScalar vmax = pb.edges.back();
    // Get the number of bins
    std::size_t nBins = pb.bins();
    // Check if extent overwrites that
    if (extent.constrains(pb.binValue)) {
      const auto& range = extent.range(pb.binValue);
      // Patch the edges values from the range
      vmin = range.min();
      vmax = range.max();
    }
    // Possibly update the edges
    if (pb.axisType == Acts::AxisType::Equidistant) {
      Acts::ActsScalar binWidth = (vmax - vmin) / nBins;
      // Fill the edges
      pb.edges = {vmin};
      pb.edges.resize(nBins + 1);
      for (std::size_t ib = 0; ib <= nBins; ++ib) {
        pb.edges[ib] = vmin + ib * binWidth;
      }
    } else {
      pb.edges.front() = vmin;
      pb.edges.back() = vmax;
    }
  }
}

/// Helper for 1-dimensional generators
///
/// @tparam aType is the axis boundary type: closed or bound
///
/// @param gctx the geometry context
/// @param lSurfaces the surfaces of the layer
/// @param assignToAll the indices assigned to all
/// @param binning the binning struct
///
/// @return a configured surface candidate updators
template <Acts::AxisBoundaryType aType>
Acts::Experimental::InternalNavigationDelegate createUpdater(
    const Acts::GeometryContext& gctx,
    std::vector<std::shared_ptr<Acts::Surface>> lSurfaces,
    std::vector<std::size_t> assignToAll,
    const Acts::Experimental::ProtoBinning& binning) {
  // The surface candidate updator & a generator for polyhedrons
  Acts::Experimental::InternalNavigationDelegate sfCandidates;
  Acts::Experimental::detail::PolyhedronReferenceGenerator rGenerator;
  // Indexed Surface generator for this case
  Acts::Experimental::detail::IndexedSurfacesGenerator<
      decltype(lSurfaces), Acts::Experimental::IndexedSurfacesNavigation>
      isg{std::move(lSurfaces),
          std::move(assignToAll),
          {binning.binValue},
          {binning.expansion}};
  if (binning.axisType == Acts::AxisType::Equidistant) {
    // Equidistant
    Acts::GridAxisGenerators::Eq<aType> aGenerator{
        {binning.edges.front(), binning.edges.back()}, binning.bins()};
    sfCandidates = isg(gctx, aGenerator, rGenerator);
  } else {
    // Variable
    Acts::GridAxisGenerators::Var<aType> aGenerator{binning.edges};
    sfCandidates = isg(gctx, aGenerator, rGenerator);
  }
  return sfCandidates;
}

/// Helper for 2-dimensional generators
///
/// @tparam aType is the axis boundary type, axis a: closed or bound
/// @tparam bType is the axis boundary type, axis b: closed or bound
///
/// @param gctx the geometry context
/// @param lSurfaces the surfaces of the layer
/// @param assignToAll the indices assigned to all
/// @param aBinning the binning struct of axis a
/// @param bBinning the binning struct of axis b
///
/// @return a configured surface candidate updators
template <Acts::AxisBoundaryType aType, Acts::AxisBoundaryType bType>
Acts::Experimental::InternalNavigationDelegate createUpdater(
    const Acts::GeometryContext& gctx,
    const std::vector<std::shared_ptr<Acts::Surface>>& lSurfaces,
    const std::vector<std::size_t>& assignToAll,
    const Acts::Experimental::ProtoBinning& aBinning,
    const Acts::Experimental::ProtoBinning& bBinning) {
  // The surface candidate updator & a generator for polyhedrons
  Acts::Experimental::InternalNavigationDelegate sfCandidates;
  Acts::Experimental::detail::PolyhedronReferenceGenerator rGenerator;
  // Indexed Surface generator for this case
  Acts::Experimental::detail::IndexedSurfacesGenerator<
      decltype(lSurfaces), Acts::Experimental::IndexedSurfacesNavigation>
      isg{lSurfaces,
          assignToAll,
          {aBinning.binValue, bBinning.binValue},
          {aBinning.expansion, bBinning.expansion}};
  // Run through the cases
  if (aBinning.axisType == Acts::AxisType::Equidistant &&
      bBinning.axisType == Acts::AxisType::Equidistant) {
    // Equidistant-Equidistant
    Acts::GridAxisGenerators::EqEq<aType, bType> aGenerator{
        {aBinning.edges.front(), aBinning.edges.back()},
        aBinning.bins(),
        {bBinning.edges.front(), bBinning.edges.back()},
        bBinning.bins()};
    sfCandidates = isg(gctx, aGenerator, rGenerator);
  } else if (bBinning.axisType == Acts::AxisType::Equidistant) {
    // Variable-Equidistant
    Acts::GridAxisGenerators::VarEq<aType, bType> aGenerator{
        aBinning.edges,
        {bBinning.edges.front(), bBinning.edges.back()},
        bBinning.bins()};
    sfCandidates = isg(gctx, aGenerator, rGenerator);
  } else if (aBinning.axisType == Acts::AxisType::Equidistant) {
    // Equidistant-Variable
    Acts::GridAxisGenerators::EqVar<aType, bType> aGenerator{
        {aBinning.edges.front(), aBinning.edges.back()},
        aBinning.bins(),
        bBinning.edges};
    sfCandidates = isg(gctx, aGenerator, rGenerator);
  } else {
    // Variable-Variable
    Acts::GridAxisGenerators::VarVar<aType, bType> aGenerator{aBinning.edges,
                                                              bBinning.edges};
    sfCandidates = isg(gctx, aGenerator, rGenerator);
  }
  // Return the candidates
  return sfCandidates;
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
      for (const auto& bv : allBinningValues()) {
        if (support.volumeExtent.constrains(bv) &&
            !rangeContainsValue(support.internalConstraints, bv)) {
          ACTS_VERBOSE("  Support surface is constrained by volume extent in "
                       << binningValueName(bv));
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
            support.offset, support.volumeClearance[BinningValue::binZ],
            support.volumeClearance[BinningValue::binPhi]};
        detail::SupportSurfacesHelper::addSupport(internalSurfaces, assignToAll,
                                                  supportExtent, cSupport,
                                                  support.splits);
      } else if (support.type == Surface::SurfaceType::Disc) {
        // Add disc support
        detail::SupportSurfacesHelper::DiscSupport dSupport{
            support.offset, support.volumeClearance[BinningValue::binR],
            support.volumeClearance[BinningValue::binPhi]};
        detail::SupportSurfacesHelper::addSupport(internalSurfaces, assignToAll,
                                                  supportExtent, dSupport,
                                                  support.splits);
      } else if (support.type == Surface::SurfaceType::Plane) {
        // Set the local coordinates - cyclic permutation
        std::array<BinningValue, 2> locals = {BinningValue::binX,
                                              BinningValue::binY};
        if (support.pPlacement == BinningValue::binX) {
          locals = {BinningValue::binY, BinningValue::binZ};
        } else if (support.pPlacement == BinningValue::binY) {
          locals = {BinningValue::binZ, BinningValue::binX};
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
    } else if (binnings.size() == 1u) {
      // Check if autorange for binning applies
      if (m_cfg.extent.has_value()) {
        ACTS_DEBUG("- adapting the proto binning range to the surface extent.");
        adaptBinningRange(binnings, m_cfg.extent.value());
      }
      ACTS_DEBUG("- 1-dimensional surface binning detected.");
      // Capture the binning
      auto binning = binnings[0u];
      if (binning.boundaryType == Acts::AxisBoundaryType::Closed) {
        ACTS_VERBOSE("-- closed binning option.");
        internalCandidatesUpdater =
            createUpdater<Acts::AxisBoundaryType::Closed>(
                gctx, internalSurfaces, assignToAll, binning);
      } else {
        ACTS_VERBOSE("-- bound binning option.");
        internalCandidatesUpdater =
            createUpdater<Acts::AxisBoundaryType::Bound>(gctx, internalSurfaces,
                                                         assignToAll, binning);
      }
    } else if (binnings.size() == 2u) {
      // Check if autorange for binning applies
      if (m_cfg.extent.has_value()) {
        ACTS_DEBUG(
            "- adapting the proto binning range(s) to the surface extent.");
        adaptBinningRange(binnings, m_cfg.extent.value());
      }
      // Sort the binning for conventions
      std::ranges::sort(binnings, {}, [](const auto& b) { return b.binValue; });

      ACTS_DEBUG("- 2-dimensional surface binning detected.");
      // Capture the binnings
      const auto& binning0 = binnings[0u];
      const auto& binning1 = binnings[1u];

      if (binning0.boundaryType == Acts::AxisBoundaryType::Closed) {
        ACTS_VERBOSE("-- closed/bound binning option.");
        internalCandidatesUpdater =
            createUpdater<Acts::AxisBoundaryType::Closed,
                          Acts::AxisBoundaryType::Bound>(
                gctx, internalSurfaces, assignToAll, binning0, binning1);
      } else if (binning1.boundaryType == Acts::AxisBoundaryType::Closed) {
        ACTS_VERBOSE("-- bound/closed binning option.");
        internalCandidatesUpdater =
            createUpdater<Acts::AxisBoundaryType::Bound,
                          Acts::AxisBoundaryType::Closed>(
                gctx, internalSurfaces, assignToAll, binning0, binning1);
      } else {
        ACTS_VERBOSE("-- bound/bound binning option.");
        internalCandidatesUpdater =
            createUpdater<Acts::AxisBoundaryType::Bound,
                          Acts::AxisBoundaryType::Bound>(
                gctx, internalSurfaces, assignToAll, binning0, binning1);
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
