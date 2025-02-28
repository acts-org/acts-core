// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/SurfaceMaterialMapper.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/ApproachDescriptor.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Propagator/ActorList.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/SurfaceCollector.hpp"
#include "Acts/Propagator/VolumeCollector.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/BinAdjustment.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/Result.hpp"

#include <cstddef>
#include <ostream>
#include <utility>
#include <vector>

namespace Acts {

SurfaceMaterialMapper::SurfaceMaterialMapper(
    const Config& cfg, StraightLinePropagator propagator,
    std::unique_ptr<const Logger> slogger)
    : m_cfg(cfg),
      m_propagator(std::move(propagator)),
      m_logger(std::move(slogger)) {}

SurfaceMaterialMapper::State SurfaceMaterialMapper::createState(
    const GeometryContext& gctx, const MagneticFieldContext& mctx,
    const TrackingGeometry& tGeometry) const {
  // Parse the geometry and find all surfaces with material proxies
  auto world = tGeometry.highestTrackingVolume();

  // The Surface material mapping state
  State mState(gctx, mctx);
  resolveMaterialSurfaces(mState, *world);
  collectMaterialVolumes(mState, *world);

  ACTS_DEBUG(mState.accumulatedMaterial.size()
             << " Surfaces with PROXIES collected ... ");
  for (auto& smg : mState.accumulatedMaterial) {
    ACTS_VERBOSE(" -> Surface in with id " << smg.first);
  }
  return mState;
}

void SurfaceMaterialMapper::resolveMaterialSurfaces(
    State& mState, const TrackingVolume& tVolume) const {
  ACTS_VERBOSE("Checking volume '" << tVolume.volumeName()
                                   << "' for material surfaces.");

  ACTS_VERBOSE("- boundary surfaces ...");
  // Check the boundary surfaces
  for (auto& bSurface : tVolume.boundarySurfaces()) {
    checkAndInsert(mState, bSurface->surfaceRepresentation());
  }

  ACTS_VERBOSE("- confined layers ...");
  // Check the confined layers
  if (tVolume.confinedLayers() != nullptr) {
    for (auto& cLayer : tVolume.confinedLayers()->arrayObjects()) {
      // Take only layers that are not navigation layers
      if (cLayer->layerType() != navigation) {
        // Check the representing surface
        checkAndInsert(mState, cLayer->surfaceRepresentation());
        // Get the approach surfaces if present
        if (cLayer->approachDescriptor() != nullptr) {
          for (auto& aSurface :
               cLayer->approachDescriptor()->containedSurfaces()) {
            if (aSurface != nullptr) {
              checkAndInsert(mState, *aSurface);
            }
          }
        }
        // Get the sensitive surface is present
        if (cLayer->surfaceArray() != nullptr) {
          // Sensitive surface loop
          for (auto& sSurface : cLayer->surfaceArray()->surfaces()) {
            if (sSurface != nullptr) {
              checkAndInsert(mState, *sSurface);
            }
          }
        }
      }
    }
  }
  // Step down into the sub volume
  if (tVolume.confinedVolumes()) {
    for (auto& sVolume : tVolume.confinedVolumes()->arrayObjects()) {
      // Recursive call
      resolveMaterialSurfaces(mState, *sVolume);
    }
  }
}

void SurfaceMaterialMapper::checkAndInsert(State& mState,
                                           const Surface& surface) const {
  auto surfaceMaterial = surface.surfaceMaterial();
  // Check if the surface has a proxy
  if (surfaceMaterial != nullptr) {
    if (m_cfg.computeVariance) {
      mState.inputSurfaceMaterial[surface.geometryId()] =
          surface.surfaceMaterialSharedPtr();
    }
    auto geoID = surface.geometryId();
    std::size_t volumeID = geoID.volume();
    ACTS_DEBUG("Material surface found with volumeID " << volumeID);
    ACTS_DEBUG("       - surfaceID is " << geoID);

    // We need a dynamic_cast to either a surface material proxy or
    // proper surface material
    auto psm = dynamic_cast<const ProtoSurfaceMaterial*>(surfaceMaterial);

    // Get the bin utility: try proxy material first
    const BinUtility* bu = (psm != nullptr) ? (&psm->binning()) : nullptr;
    if (bu != nullptr) {
      // Screen output for Binned Surface material
      ACTS_DEBUG("       - (proto) binning is " << *bu);
      // Now update
      BinUtility buAdjusted = adjustBinUtility(*bu, surface, mState.geoContext);
      // Screen output for Binned Surface material
      ACTS_DEBUG("       - adjusted binning is " << buAdjusted);
      mState.accumulatedMaterial[geoID] =
          AccumulatedSurfaceMaterial(buAdjusted);
      return;
    }

    // Second attempt: binned material
    auto bmp = dynamic_cast<const BinnedSurfaceMaterial*>(surfaceMaterial);
    bu = (bmp != nullptr) ? (&bmp->binUtility()) : nullptr;
    // Creaete a binned type of material
    if (bu != nullptr) {
      // Screen output for Binned Surface material
      ACTS_DEBUG("       - binning is " << *bu);
      mState.accumulatedMaterial[geoID] = AccumulatedSurfaceMaterial(*bu);
    } else {
      // Create a homogeneous type of material
      ACTS_DEBUG("       - this is homogeneous material.");
      mState.accumulatedMaterial[geoID] = AccumulatedSurfaceMaterial();
    }
  }
}

void SurfaceMaterialMapper::collectMaterialVolumes(
    State& mState, const TrackingVolume& tVolume) const {
  ACTS_VERBOSE("Checking volume '" << tVolume.volumeName()
                                   << "' for material surfaces.");
  ACTS_VERBOSE("- Insert Volume ...");
  if (tVolume.volumeMaterial() != nullptr) {
    mState.volumeMaterial[tVolume.geometryId()] = tVolume.volumeMaterialPtr();
  }

  // Step down into the sub volume
  if (tVolume.confinedVolumes()) {
    ACTS_VERBOSE("- Check children volume ...");
    for (auto& sVolume : tVolume.confinedVolumes()->arrayObjects()) {
      // Recursive call
      collectMaterialVolumes(mState, *sVolume);
    }
  }
  if (!tVolume.denseVolumes().empty()) {
    for (auto& sVolume : tVolume.denseVolumes()) {
      // Recursive call
      collectMaterialVolumes(mState, *sVolume);
    }
  }
}

void SurfaceMaterialMapper::finalizeMaps(State& mState) const {
  // iterate over the map to call the total average
  for (auto& accMaterial : mState.accumulatedMaterial) {
    ACTS_DEBUG("Finalizing map for Surface " << accMaterial.first);
    mState.surfaceMaterial[accMaterial.first] =
        accMaterial.second.totalAverage();
  }
}

void SurfaceMaterialMapper::mapMaterialTrack(
    State& mState, RecordedMaterialTrack& mTrack) const {
  // Retrieve the recorded material from the recorded material track
  auto& rMaterial = mTrack.second.materialInteractions;
  ACTS_VERBOSE("Retrieved " << rMaterial.size()
                            << " recorded material steps to map.");

  // Check if the material interactions are associated with a surface. If yes we
  // simply need to loop over them and accumulate the material
  if (rMaterial.begin()->intersectionID != GeometryIdentifier()) {
    ACTS_VERBOSE(
        "Material surfaces are associated with the material interaction. The "
        "association interaction/surfaces won't be performed again.");
    mapSurfaceInteraction(mState, rMaterial);
    return;
  } else {
    ACTS_VERBOSE(
        "Material interactions need to be associated with surfaces. Collecting "
        "all surfaces on the trajectory.");
    mapInteraction(mState, mTrack);
    return;
  }
}
void SurfaceMaterialMapper::mapInteraction(
    State& mState, RecordedMaterialTrack& mTrack) const {
  // Retrieve the recorded material from the recorded material track
  auto& rMaterial = mTrack.second.materialInteractions;
  std::map<GeometryIdentifier, unsigned int> assignedMaterial;
  using VectorHelpers::makeVector4;
  // Neutral curvilinear parameters
  NeutralCurvilinearTrackParameters start(
      makeVector4(mTrack.first.first, 0), mTrack.first.second,
      1 / mTrack.first.second.norm(), std::nullopt,
      NeutralParticleHypothesis::geantino());

  // Prepare Action list and abort list
  using MaterialSurfaceCollector = SurfaceCollector<MaterialSurface>;
  using MaterialVolumeCollector = VolumeCollector<MaterialVolume>;
  using ActorList = ActorList<MaterialSurfaceCollector, MaterialVolumeCollector,
                              EndOfWorldReached>;

  StraightLinePropagator::Options<ActorList> options(mState.geoContext,
                                                     mState.magFieldContext);

  // Now collect the material layers by using the straight line propagator
  const auto& result = m_propagator.propagate(start, options).value();
  auto mcResult = result.get<MaterialSurfaceCollector::result_type>();
  auto mvcResult = result.get<MaterialVolumeCollector::result_type>();

  auto mappingSurfaces = mcResult.collected;
  auto mappingVolumes = mvcResult.collected;

  // These should be mapped onto the mapping surfaces found
  ACTS_VERBOSE("Found     " << mappingSurfaces.size()
                            << " mapping surfaces for this track.");
  ACTS_VERBOSE("Mapping surfaces are :");
  for (auto& mSurface : mappingSurfaces) {
    ACTS_VERBOSE(" - Surface : " << mSurface.surface->geometryId()
                                 << " at position = (" << mSurface.position.x()
                                 << ", " << mSurface.position.y() << ", "
                                 << mSurface.position.z() << ")");
    assignedMaterial[mSurface.surface->geometryId()] = 0;
  }

  // Run the mapping process, i.e. take the recorded material and map it
  // onto the mapping surfaces:
  // - material steps and surfaces are assumed to be ordered along the
  // mapping ray
  //- do not record the material inside a volume with material
  auto rmIter = rMaterial.begin();
  auto sfIter = mappingSurfaces.begin();
  auto volIter = mappingVolumes.begin();

  // Use those to minimize the lookup
  GeometryIdentifier lastID = GeometryIdentifier();
  GeometryIdentifier currentID = GeometryIdentifier();
  Vector3 currentPos(0., 0., 0);
  float currentPathCorrection = 1.;
  auto currentAccMaterial = mState.accumulatedMaterial.end();

  // To remember the bins of this event
  using MapBin =
      std::pair<AccumulatedSurfaceMaterial*, std::array<std::size_t, 3>>;
  std::map<AccumulatedSurfaceMaterial*, std::array<std::size_t, 3>>
      touchedMapBins;
  std::map<AccumulatedSurfaceMaterial*, std::shared_ptr<const ISurfaceMaterial>>
      touchedMaterialBin;
  if (sfIter != mappingSurfaces.end() &&
      sfIter->surface->surfaceMaterial()->mappingType() ==
          MappingType::PostMapping) {
    ACTS_WARNING(
        "The first mapping surface is a PostMapping one. Some material from "
        "before the PostMapping surface will be mapped onto it ");
  }

  // Assign the recorded ones, break if you hit an end
  while (rmIter != rMaterial.end() && sfIter != mappingSurfaces.end()) {
    // Material not inside current volume
    if (volIter != mappingVolumes.end() &&
        !volIter->volume->inside(rmIter->position)) {
      double distVol = (volIter->position - mTrack.first.first).norm();
      double distMat = (rmIter->position - mTrack.first.first).norm();
      // Material past the entry point to the current volume
      if (distMat - distVol > s_epsilon) {
        // Switch to next material volume
        ++volIter;
        continue;
      }
    }
    /// check if we are inside a material volume
    if (volIter != mappingVolumes.end() &&
        volIter->volume->inside(rmIter->position)) {
      ++rmIter;
      continue;
    }
    // Do we need to switch to next assignment surface ?
    if (sfIter != mappingSurfaces.end() - 1) {
      int mappingType = sfIter->surface->surfaceMaterial()->mappingType();
      int nextMappingType =
          (sfIter + 1)->surface->surfaceMaterial()->mappingType();

      if (mappingType == MappingType::PreMapping ||
          mappingType == MappingType::Sensor) {
        // Change surface if the material after the current surface.
        if ((rmIter->position - mTrack.first.first).norm() >
            (sfIter->position - mTrack.first.first).norm()) {
          if (nextMappingType == MappingType::PostMapping) {
            ACTS_WARNING(
                "PreMapping or Sensor surface followed by PostMapping. Some "
                "material "
                "from before the PostMapping surface will be mapped onto it");
          }
          ++sfIter;
          continue;
        }
      } else if (mappingType == MappingType::Default ||
                 mappingType == MappingType::PostMapping) {
        switch (nextMappingType) {
          case MappingType::PreMapping:
          case MappingType::Default: {
            // Change surface if the material closest to the next surface.
            if ((rmIter->position - sfIter->position).norm() >
                (rmIter->position - (sfIter + 1)->position).norm()) {
              ++sfIter;
              continue;
            }
            break;
          }
          case MappingType::PostMapping: {
            // Change surface if the material after the next surface.
            if ((rmIter->position - sfIter->position).norm() >
                ((sfIter + 1)->position - sfIter->position).norm()) {
              ++sfIter;
              continue;
            }
            break;
          }
          case MappingType::Sensor: {
            // Change surface if the next material after the next surface.
            if ((rmIter == rMaterial.end() - 1) ||
                ((rmIter + 1)->position - sfIter->position).norm() >
                    ((sfIter + 1)->position - sfIter->position).norm()) {
              ++sfIter;
              continue;
            }
            break;
          }
          default: {
            ACTS_ERROR("Incorrect mapping type for the next surface : "
                       << (sfIter + 1)->surface->geometryId());
          }
        }
      } else {
        ACTS_ERROR("Incorrect mapping type for surface : "
                   << sfIter->surface->geometryId());
      }
    }

    // get the current Surface ID
    currentID = sfIter->surface->geometryId();
    // We have work to do: the assignment surface has changed
    if (!(currentID == lastID)) {
      // Let's (re-)assess the information
      lastID = currentID;
      currentPos = (sfIter)->position;
      currentPathCorrection = sfIter->surface->pathCorrection(
          mState.geoContext, currentPos, sfIter->direction);
      currentAccMaterial = mState.accumulatedMaterial.find(currentID);
    }
    // Now assign the material for the accumulation process
    auto tBin = currentAccMaterial->second.accumulate(
        currentPos, rmIter->materialSlab, currentPathCorrection);
    if (!touchedMapBins.contains(&(currentAccMaterial->second))) {
      touchedMapBins.insert(MapBin(&(currentAccMaterial->second), tBin));
    }
    if (m_cfg.computeVariance) {
      touchedMaterialBin[&(currentAccMaterial->second)] =
          mState.inputSurfaceMaterial[currentID];
    }
    ++assignedMaterial[currentID];
    // Update the material interaction with the associated surface and
    // intersection
    rmIter->surface = sfIter->surface;
    rmIter->intersection = sfIter->position;
    rmIter->intersectionID = currentID;
    rmIter->pathCorrection = currentPathCorrection;
    // Switch to next material
    ++rmIter;
  }

  ACTS_VERBOSE("Surfaces have following number of assigned hits :");
  for (auto& [key, value] : assignedMaterial) {
    ACTS_VERBOSE(" + Surface : " << key << " has " << value << " hits.");
  }

  // After mapping this track, average the touched bins
  for (const auto& [key, value] : touchedMapBins) {
    std::vector<std::array<std::size_t, 3>> trackBins = {value};
    if (m_cfg.computeVariance) {
      // This only makes sense for the binned material
      auto binnedMaterial = dynamic_cast<const BinnedSurfaceMaterial*>(
          touchedMaterialBin[key].get());
      if (binnedMaterial != nullptr) {
        key->trackVariance(
            trackBins,
            binnedMaterial->fullMaterial()[trackBins[0][1]][trackBins[0][0]]);
      }
    }
    key->trackAverage(trackBins);
  }

  // After mapping this track, average the untouched but intersected bins
  if (m_cfg.emptyBinCorrection) {
    // Use the assignedMaterial map to account for empty hits, i.e.
    // the material surface has been intersected by the mapping ray
    // but no material step was assigned to this surface
    for (auto& mSurface : mappingSurfaces) {
      auto mgID = mSurface.surface->geometryId();
      // Count an empty hit only if the surface does not appear in the
      // list of assigned surfaces
      if (assignedMaterial[mgID] == 0) {
        auto missedMaterial = mState.accumulatedMaterial.find(mgID);
        if (m_cfg.computeVariance) {
          missedMaterial->second.trackVariance(
              mSurface.position,
              mState.inputSurfaceMaterial[currentID]->materialSlab(
                  mSurface.position),
              true);
        }
        missedMaterial->second.trackAverage(mSurface.position, true);

        // Add an empty material hit for future material mapping iteration
        MaterialInteraction noMaterial;
        noMaterial.surface = mSurface.surface;
        noMaterial.intersection = mSurface.position;
        noMaterial.intersectionID = mgID;
        rMaterial.push_back(noMaterial);
      }
    }
  }
}

void SurfaceMaterialMapper::mapSurfaceInteraction(
    State& mState, std::vector<MaterialInteraction>& rMaterial) const {
  using MapBin =
      std::pair<AccumulatedSurfaceMaterial*, std::array<std::size_t, 3>>;
  std::map<AccumulatedSurfaceMaterial*, std::array<std::size_t, 3>>
      touchedMapBins;
  std::map<AccumulatedSurfaceMaterial*, std::shared_ptr<const ISurfaceMaterial>>
      touchedMaterialBin;

  // Looping over all the material interactions
  auto rmIter = rMaterial.begin();
  while (rmIter != rMaterial.end()) {
    // get the current interaction information
    GeometryIdentifier currentID = rmIter->intersectionID;
    Vector3 currentPos = rmIter->intersection;
    auto currentAccMaterial = mState.accumulatedMaterial.find(currentID);

    // Now assign the material for the accumulation process
    auto tBin = currentAccMaterial->second.accumulate(
        currentPos, rmIter->materialSlab, rmIter->pathCorrection);
    if (!touchedMapBins.contains(&(currentAccMaterial->second))) {
      touchedMapBins.insert(MapBin(&(currentAccMaterial->second), tBin));
    }
    if (m_cfg.computeVariance) {
      touchedMaterialBin[&(currentAccMaterial->second)] =
          mState.inputSurfaceMaterial[currentID];
    }
    ++rmIter;
  }

  // After mapping this track, average the touched bins
  for (const auto& [key, value] : touchedMapBins) {
    std::vector<std::array<std::size_t, 3>> trackBins = {value};
    if (m_cfg.computeVariance) {
      // This only makes sense for the binned material
      auto binnedMaterial = dynamic_cast<const BinnedSurfaceMaterial*>(
          touchedMaterialBin[key].get());
      if (binnedMaterial != nullptr) {
        key->trackVariance(
            trackBins,
            binnedMaterial->fullMaterial()[trackBins[0][1]][trackBins[0][0]],
            true);
      }
    }
    // No need to do an extra pass for untouched surfaces they would have been
    // added to the material interaction in the initial mapping
    key->trackAverage(trackBins, true);
  }
}

}  // namespace Acts
