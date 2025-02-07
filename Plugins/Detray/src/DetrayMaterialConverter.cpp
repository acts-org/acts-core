// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Detray/DetrayMaterialConverter.hpp"

#include "Acts/Detector/Detector.hpp"
#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Plugins/Detray/DetrayConversionUtils.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <numbers>
#include <stdexcept>

namespace {

struct MaterialSurfaceSelector {
  std::vector<const Acts::Surface*> surfaces = {};

  /// @param surface is the test surface
  void operator()(const Acts::Surface* surface) {
    if (surface->surfaceMaterial() != nullptr &&
        !rangeContainsValue(surfaces, surface)) {
      surfaces.push_back(surface);
    }
  }
};

}  // namespace

detray::io::material_slab_payload
Acts::DetrayMaterialConverter::convertMaterialSlab(
    const MaterialSlab& materialSlab) {
  detray::io::material_slab_payload slab;
  // Fill the material parameters and the thickness
  const auto& material = materialSlab.material();
  slab.thickness = materialSlab.thickness();
  slab.mat = detray::io::material_payload{
      {material.X0(), material.L0(), material.Ar(), material.Z(),
       material.massDensity(), material.molarDensity(), 0.}};
  slab.type = detray::io::material_id::slab;
  return slab;
}

detray::io::detector_homogeneous_material_payload
Acts::DetrayMaterialConverter::convertHomogeneousSurfaceMaterial(
    const DetrayConversionUtils::Cache& cCache,
    const Experimental::Detector& detector, const Logger& logger) {
  detray::io::detector_homogeneous_material_payload materialPayload;

  for (const auto volume : detector.volumes()) {
    auto volumeIndex = cCache.volumeLinks.find(volume->geometryId());
    if (volumeIndex != cCache.volumeLinks.end()) {
      // The volume material payload & its link
      detray::io::material_volume_payload volumePayload;
      detray::io::single_link_payload volumeLink;
      volumeLink.link = volumeIndex->second;
      volumePayload.volume_link = volumeLink;
      // Now run through surfaces and portals to find the material
      MaterialSurfaceSelector selector;
      volume->visitSurfaces(selector);
      ACTS_DEBUG("DetrayMaterialConverter: found "
                 << selector.surfaces.size()
                 << " surfaces/portals with material in volume "
                 << volume->name());
      for (const auto surface : selector.surfaces) {
        const auto* surfaceMaterial = surface->surfaceMaterial();
        auto homogeneousMaterial =
            dynamic_cast<const HomogeneousSurfaceMaterial*>(surfaceMaterial);
        if (homogeneousMaterial != nullptr) {
          // Convert the material slab
          auto materialSlab = homogeneousMaterial->materialSlab();
          detray::io::material_slab_payload slabPayload =
              convertMaterialSlab(materialSlab);
          // Find the surfaces to assign
          auto vIndex = cCache.volumeIndex(volume);
          auto localSurfaceLinks = cCache.localSurfaceLinks.find(vIndex);
          if (localSurfaceLinks != cCache.localSurfaceLinks.end()) {
            // Find the surface link
            auto surfaceIndices =
                localSurfaceLinks->second.equal_range(surface->geometryId());
            // Loop over the equal range and fill one grid each, this is needed
            // as the initial portal could be split into multiple surfaces
            for (auto itr = surfaceIndices.first; itr != surfaceIndices.second;
                 ++itr) {
              // Make an identified link copy for every matching surface
              slabPayload.surface.link = itr->second;
              volumePayload.mat_slabs.push_back(slabPayload);
            }
          } else {
            ACTS_WARNING(
                "DetrayMaterialConverter: no local surface links found");
          }
        }
      }
      materialPayload.volumes.push_back(volumePayload);
    } else {
      ACTS_WARNING("DetrayMaterialConverter: volume " << volume->name()
                                                      << " not found in cache");
    }
  }
  return materialPayload;
}

detray::io::grid_payload<detray::io::material_slab_payload,
                         detray::io::material_id>
Acts::DetrayMaterialConverter::convertGridSurfaceMaterial(
    const ISurfaceMaterial& material, const Logger& logger) {
  detray::io::grid_payload<detray::io::material_slab_payload,
                           detray::io::material_id>
      materialGrid;

  // Check the material types
  // (1) homogeneous -> skip
  auto homogeneousMaterial =
      dynamic_cast<const HomogeneousSurfaceMaterial*>(&material);
  if (homogeneousMaterial != nullptr) {
    ACTS_DEBUG(
        "DetrayMaterialConverter: found homogeneous surface material, ignored "
        "as this should be handled by the homogeneous material conversion.");
    return materialGrid;
  }
  // (2) - binned material -> convert into grid structure
  auto binnedMaterial = dynamic_cast<const BinnedSurfaceMaterial*>(&material);
  if (binnedMaterial != nullptr) {
    ACTS_VERBOSE("DetrayMaterialConverter: found binned surface material");

    // BinUtility modifications
    bool swapped = false;
    // Get the bin utility (make a copy as we may modify it)
    // Detray expects 2-dimensional grid, currently supported are
    // x-y, r-phi, phi-z
    BinUtility bUtility = binnedMaterial->binUtility();
    // Turn the bin value into a 2D grid
    if (bUtility.dimensions() == 1u) {
      if (bUtility.binningData()[0u].binvalue == AxisDirection::AxisX) {
        // Turn to X-Y
        bUtility += BinUtility(1u, std::numeric_limits<float>::lowest(),
                               std::numeric_limits<float>::max(),
                               BinningOption::closed, AxisDirection::AxisY);
      } else if (bUtility.binningData()[0u].binvalue == AxisDirection::AxisY) {
        // Turn to X-Y
        BinUtility nbUtility(1u, std::numeric_limits<float>::lowest(),
                             std::numeric_limits<float>::max(),
                             BinningOption::closed, AxisDirection::AxisX);
        nbUtility += bUtility;
        bUtility = std::move(nbUtility);
        swapped = true;
      } else if (bUtility.binningData()[0u].binvalue == AxisDirection::AxisR) {
        // Turn to R-Phi
        bUtility += BinUtility(1u, -std::numbers::pi, std::numbers::pi, closed,
                               AxisDirection::AxisPhi);
      } else if (bUtility.binningData()[0u].binvalue == AxisDirection::AxisZ) {
        // Turn to Phi-Z - swap needed
        BinUtility nbUtility(1u, -std::numbers::pi, std::numbers::pi, closed,
                             AxisDirection::AxisPhi);
        nbUtility += bUtility;
        bUtility = std::move(nbUtility);
        swapped = true;
      } else {
        std::invalid_argument("Unsupported binning for Detray");
      }
    } else if (bUtility.dimensions() == 2u &&
               bUtility.binningData()[0u].binvalue == AxisDirection::AxisZ &&
               bUtility.binningData()[1u].binvalue == AxisDirection::AxisPhi) {
      BinUtility nbUtility(bUtility.binningData()[1u]);
      nbUtility += BinUtility{bUtility.binningData()[0u]};
      bUtility = std::move(nbUtility);
      swapped = true;
    }

    AxisDirection bVal0 = bUtility.binningData()[0u].binvalue;
    AxisDirection bVal1 = bUtility.binningData()[1u].binvalue;

    // Translate into grid index type
    detray::io::material_id gridIndexType = detray::io::material_id::unknown;
    if (bVal0 == AxisDirection::AxisR && bVal1 == AxisDirection::AxisPhi) {
      gridIndexType = detray::io::material_id::ring2_map;
    } else if (bVal0 == AxisDirection::AxisPhi &&
               bVal1 == AxisDirection::AxisZ) {
      gridIndexType = detray::io::material_id::concentric_cylinder2_map;
    } else if (bVal0 == AxisDirection::AxisX && bVal1 == AxisDirection::AxisY) {
      gridIndexType = detray::io::material_id::rectangle2_map;
    } else {
      std::runtime_error(
          "DetrayMaterialConverter: Unsupported binning for Detray");
    }

    detray::io::typed_link_payload<detray::io::material_id> linkPayload{
        gridIndexType, 0u};
    materialGrid.grid_link = linkPayload;

    // Now convert the (modified) bin utility
    for (const auto& bData : bUtility.binningData()) {
      auto axisPayload = DetrayConversionUtils::convertBinningData(bData);
      materialGrid.axes.push_back(axisPayload);
    }

    // Convert the material slabs from the material matrix
    auto materialMatrix = binnedMaterial->fullMaterial();
    for (std::size_t ib1 = 0; ib1 < materialMatrix.size(); ++ib1) {
      for (std::size_t ib0 = 0; ib0 < materialMatrix[0u].size(); ++ib0) {
        // Translate into a local bin
        std::size_t lb0 = swapped ? ib1 : ib0;
        std::size_t lb1 = swapped ? ib0 : ib1;
        detray::io::material_slab_payload slab =
            convertMaterialSlab(materialMatrix[ib1][ib0]);
        detray::io::grid_bin_payload<detray::io::material_slab_payload> slabBin{
            {static_cast<unsigned int>(lb0), static_cast<unsigned int>(lb1)},
            {slab}};
        // Fill into the grid
        materialGrid.bins.push_back(slabBin);
      }
    }
    return materialGrid;
  }

  if (dynamic_cast<const Acts::ProtoSurfaceMaterial*>(&material) != nullptr ||
      dynamic_cast<const Acts::ProtoGridSurfaceMaterial*>(&material) !=
          nullptr) {
    ACTS_WARNING(
        "DetrayMaterialConverter: ProtoSurfaceMaterial and "
        "ProtoGridSurfaceMaterial are not being translated, consider to switch "
        "material conversion off.");
    return materialGrid;
  }

  throw std::invalid_argument(
      "DetrayMaterialConverter: unknown surface material type detected.");
}

detray::io::detector_grids_payload<detray::io::material_slab_payload,
                                   detray::io::material_id>
Acts::DetrayMaterialConverter::convertGridSurfaceMaterial(
    const DetrayConversionUtils::Cache& cCache,
    const Experimental::Detector& detector, const Logger& logger) {
  // The material grid payload
  detray::io::detector_grids_payload<detray::io::material_slab_payload,
                                     detray::io::material_id>
      materialGrids;

  using DetrayMaterialGrid =
      detray::io::grid_payload<detray::io::material_slab_payload,
                               detray::io::material_id>;

  // Loop over the volumes in order to assign the right volume links
  for (const auto& volume : detector.volumes()) {
    // Per volume surface selector
    MaterialSurfaceSelector selector;
    volume->visitSurfaces(selector);
    ACTS_VERBOSE("DetrayMaterialConverter: found "
                 << selector.surfaces.size()
                 << " surfaces/portals with material in volume "
                 << volume->name());
    // Find the voluem index first
    auto volumeIndex = cCache.volumeLinks.find(volume->geometryId());
    if (volumeIndex != cCache.volumeLinks.end()) {
      std::vector<DetrayMaterialGrid> volumeMaterialGrids = {};
      // Now convert the surfaces
      for (const auto& surface : selector.surfaces) {
        // Find the surfaces to assign
        auto vIndex = cCache.volumeIndex(volume);
        auto localSurfaceLinks = cCache.localSurfaceLinks.find(vIndex);
        if (localSurfaceLinks != cCache.localSurfaceLinks.end()) {
          // Find the surface link
          auto surfaceIndices =
              localSurfaceLinks->second.equal_range(surface->geometryId());

          ACTS_VERBOSE(
              "DetrayMaterialConverter: assigning to "
              << std::distance(surfaceIndices.first, surfaceIndices.second)
              << " surfaces with material in volume " << volume->name());
          DetrayMaterialGrid materialGrid =
              convertGridSurfaceMaterial(*surface->surfaceMaterial(), logger);
          // Ignore if an empty payload is returned
          if (materialGrid.axes.empty() || materialGrid.bins.empty()) {
            continue;
          }

          // Loop over the equal range and fill one grid each, this is needed
          // as the initial portal could be split into multiple surfaces
          for (auto itr = surfaceIndices.first; itr != surfaceIndices.second;
               ++itr) {
            // Fill the surface index
            materialGrid.owner_link =
                detray::io::single_link_payload{itr->second};
            // Fill the grid
            volumeMaterialGrids.push_back(materialGrid);
          }
        }
      }
      // Register the grids of this volume
      materialGrids.grids.insert({volumeIndex->second, volumeMaterialGrids});
    } else {
      ACTS_WARNING(
          "DetrayMaterialConverter: volume not found in cache, should not "
          "happen.");
    }
  }
  // Return the material grids payload
  return materialGrids;
}
