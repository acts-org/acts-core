// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/BinnedSurfaceMaterialAccumulater.hpp"

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinAdjustment.hpp"
#include "Acts/Utilities/BinUtility.hpp"

Acts::BinnedSurfaceMaterialAccumulater::BinnedSurfaceMaterialAccumulater(
    const Config& cfg, std::unique_ptr<const Logger> mlogger)
    : m_cfg(cfg), m_logger(std::move(mlogger)) {}

std::unique_ptr<Acts::ISurfaceMaterialAccumulater::State>
Acts::BinnedSurfaceMaterialAccumulater::createState() const {
  auto state = std::make_unique<State>();

  /// Create the surface accumulation
  for (const auto& surface : m_cfg.materialSurfaces) {
    GeometryIdentifier geoID = surface->geometryId();

    // Get the Surface Material
    const ISurfaceMaterial* surfaceMaterial = surface->surfaceMaterial();
    if (surfaceMaterial == nullptr) {
      throw std::invalid_argument(
          "Surface material is not set, inconsistent configuration.");
    }

    // First attempt from ProtoSurfaceMaterial
    auto psm = dynamic_cast<const ProtoSurfaceMaterial*>(surfaceMaterial);
    if (psm != nullptr) {
      auto binUtility = psm->binning();
      // Screen output for Binned Surface material
      ACTS_DEBUG("       - (proto) binning from ProtoSurfaceMateria is "
                 << binUtility);
      // Now adjust to surface type
      binUtility = adjustBinUtility(binUtility, *surface, m_cfg.geoContext);
      // Screen output for Binned Surface material
      ACTS_DEBUG("       - adjusted binning is " << binUtility);
      state->accumulatedMaterial[geoID] =
          AccumulatedSurfaceMaterial(binUtility);
      // Material accumulation  is created for this
      continue;
    }
    // Second attempt from ProtoGridSurfaceMaterial
    auto psgm = dynamic_cast<const ProtoGridSurfaceMaterial*>(surfaceMaterial);
    if (psgm != nullptr) {
      BinUtility binUtility(psgm->binning());
      // Screen output for Binned Surface material
      ACTS_DEBUG("       - (proto) binning from ProtoGridSurfaceMaterial is "
                 << binUtility);
      // Now adjust to surface type
      binUtility = adjustBinUtility(binUtility, *surface, m_cfg.geoContext);
      // Screen output for Binned Surface material
      ACTS_DEBUG("       - adjusted binning is " << binUtility);
      state->accumulatedMaterial[geoID] =
          AccumulatedSurfaceMaterial(binUtility);
      // Material accumulation  is created for this
      continue;
    }
    // Third attempt: binned material
    auto bmp = dynamic_cast<const BinnedSurfaceMaterial*>(surfaceMaterial);
    if (bmp != nullptr) {
      // Screen output for Binned Surface material
      ACTS_DEBUG("       - binning from BinnedSurfaceMaterial is "
                 << bmp->binUtility());
      state->accumulatedMaterial[geoID] =
          AccumulatedSurfaceMaterial(bmp->binUtility());
      // Material accumulation  is created for this
      continue;
    }
    // Create a homogeneous type of material
    ACTS_DEBUG("       - this is homogeneous material.");
    state->accumulatedMaterial[geoID] = AccumulatedSurfaceMaterial();
  }
  return state;
}

void Acts::BinnedSurfaceMaterialAccumulater::accumulate(
    ISurfaceMaterialAccumulater::State& state,
    const std::vector<MaterialInteraction>& interactions,
    const std::vector<IAssignmentFinder::SurfaceAssignment>&
        surfacesWithoutAssignment) const {
  // Cast into the right state object (guaranteed by upstream algorithm)
  State* cState = static_cast<State*>(&state);
  if (cState == nullptr) {
    throw std::invalid_argument(
        "Invalid state object provided, something is seriously wrong.");
  }

  using MapBin =
      std::pair<AccumulatedSurfaceMaterial*, std::array<std::size_t, 3>>;
  std::map<AccumulatedSurfaceMaterial*, std::array<std::size_t, 3>>
      touchedMapBins;

  // Assign the hits
  for (const auto& mi : interactions) {
    // Get the surface
    const Surface* surface = mi.surface;
    GeometryIdentifier geoID = surface->geometryId();
    // Get the accumulated material
    auto accMaterial = cState->accumulatedMaterial.find(geoID);
    if (accMaterial == cState->accumulatedMaterial.end()) {
      throw std::invalid_argument(
          "Surface material is not found, inconsistent configuration.");
    }
    // Accumulate the material - remember the touched bin
    auto tBin = accMaterial->second.accumulate(mi.intersection, mi.materialSlab,
                                               mi.pathCorrection);
    touchedMapBins.insert(MapBin(&(accMaterial->second), tBin));
  }

  // After mapping this track, average the touched bins
  for (const auto& [key, value] : touchedMapBins) {
    std::vector<std::array<std::size_t, 3>> trackBins = {value};
    key->trackAverage(trackBins, true);
  }

  // Empty bin correction
  if (m_cfg.emptyBinCorrection) {
    for (const auto& [surface, position, direction] :
         surfacesWithoutAssignment) {
      // Get the accumulated material
      auto missedMaterial =
          cState->accumulatedMaterial.find(surface->geometryId());
      if (missedMaterial == cState->accumulatedMaterial.end()) {
        throw std::invalid_argument(
            "Surface material is not found, inconsistent configuration.");
      }
      // Apply empty hit correction
      missedMaterial->second.trackAverage(position, true);
    }
  }
}

std::map<Acts::GeometryIdentifier,
         std::shared_ptr<const Acts::ISurfaceMaterial>>
Acts::BinnedSurfaceMaterialAccumulater::finalizeMaterial(
    ISurfaceMaterialAccumulater::State& state) const {
  std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>
      sMaterials;

  // Cast into the right state object (guaranteed by upstream algorithm)
  State* cState = static_cast<State*>(&state);
  if (cState == nullptr) {
    throw std::invalid_argument(
        "Invalid state object provided, something is seriously wrong.");
  }

  // iterate over the map to call the total average
  for (auto& accMaterial : cState->accumulatedMaterial) {
    ACTS_DEBUG("Finalizing map for Surface " << accMaterial.first);
    auto sMaterial = accMaterial.second.totalAverage();
    sMaterials[accMaterial.first] = std::move(sMaterial);
  }

  return sMaterials;
}
