// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/MaterialMapping/MaterialMapping.hpp"

#include "Acts/Material/AccumulatedMaterialSlab.hpp"
#include "Acts/Material/AccumulatedSurfaceMaterial.hpp"
#include "ActsExamples/MaterialMapping/IMaterialWriter.hpp"

#include <stdexcept>
#include <unordered_map>

namespace ActsExamples {

MaterialMapping::MaterialMapping(const MaterialMapping::Config& cfg,
                                 Acts::Logging::Level level)
    : IAlgorithm("MaterialMapping", level),
      m_cfg(cfg),
      m_mappingState(cfg.geoContext, cfg.magFieldContext),
      m_mappingStateVol(cfg.geoContext, cfg.magFieldContext) {
  if (!m_cfg.materialSurfaceMapper && !m_cfg.materialVolumeMapper) {
    throw std::invalid_argument("Missing material mapper");
  } else if (!m_cfg.trackingGeometry) {
    throw std::invalid_argument("Missing tracking geometry");
  }

  m_inputMaterialTracks.initialize(m_cfg.inputMaterialTracks);
  m_outputMaterialTracks.initialize(m_cfg.mappingMaterialCollection);

  ACTS_INFO("This algorithm requires inter-event information, "
            << "run in single-threaded mode!");

  if (m_cfg.materialSurfaceMapper) {
    // Generate and retrieve the central cache object
    m_mappingState = m_cfg.materialSurfaceMapper->createState(
        m_cfg.geoContext, m_cfg.magFieldContext, *m_cfg.trackingGeometry);
  }
  if (m_cfg.materialVolumeMapper) {
    // Generate and retrieve the central cache object
    m_mappingStateVol = m_cfg.materialVolumeMapper->createState(
        m_cfg.geoContext, m_cfg.magFieldContext, *m_cfg.trackingGeometry);
  }
}

ProcessCode MaterialMapping::finalize() {
  ACTS_INFO("Finalizing material mappig output");
  Acts::DetectorMaterialMaps detectorMaterial;

  if (m_cfg.materialSurfaceMapper && m_cfg.materialVolumeMapper) {
    // Finalize all the maps using the cached state
    m_cfg.materialSurfaceMapper->finalizeMaps(m_mappingState);
    m_cfg.materialVolumeMapper->finalizeMaps(m_mappingStateVol);
    // Loop over the state, and collect the maps for surfaces
    for (auto& [key, value] : m_mappingState.surfaceMaterial) {
      detectorMaterial.first.insert({key, std::move(value)});
    }
    // Loop over the state, and collect the maps for volumes
    for (auto& [key, value] : m_mappingStateVol.volumeMaterial) {
      detectorMaterial.second.insert({key, std::move(value)});
    }
  } else {
    if (m_cfg.materialSurfaceMapper) {
      // Finalize all the maps using the cached state
      m_cfg.materialSurfaceMapper->finalizeMaps(m_mappingState);
      // Loop over the state, and collect the maps for surfaces
      for (auto& [key, value] : m_mappingState.surfaceMaterial) {
        detectorMaterial.first.insert({key, std::move(value)});
      }
      // Loop over the state, and collect the maps for volumes
      for (auto& [key, value] : m_mappingState.volumeMaterial) {
        detectorMaterial.second.insert({key, std::move(value)});
      }
    }
    if (m_cfg.materialVolumeMapper) {
      // Finalize all the maps using the cached state
      m_cfg.materialVolumeMapper->finalizeMaps(m_mappingStateVol);
      // Loop over the state, and collect the maps for surfaces
      for (auto& [key, value] : m_mappingStateVol.surfaceMaterial) {
        detectorMaterial.first.insert({key, std::move(value)});
      }
      // Loop over the state, and collect the maps for volumes
      for (auto& [key, value] : m_mappingStateVol.volumeMaterial) {
        detectorMaterial.second.insert({key, std::move(value)});
      }
    }
  }
  // Loop over the available writers and write the maps
  for (auto& imw : m_cfg.materialWriters) {
    imw->writeMaterial(detectorMaterial);
  }

  return ProcessCode::SUCCESS;
}

ProcessCode MaterialMapping::execute(const AlgorithmContext& context) const {
  // Take the collection from the EventStore
  std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>
      mtrackCollection = m_inputMaterialTracks(context);

  if (m_cfg.materialSurfaceMapper) {
    // To make it work with the framework needs a lock guard
    auto mappingState =
        const_cast<Acts::SurfaceMaterialMapper::State*>(&m_mappingState);
    for (auto& [idTrack, mTrack] : mtrackCollection) {
      // Map this one onto the geometry
      m_cfg.materialSurfaceMapper->mapMaterialTrack(*mappingState, mTrack);
    }
  }
  if (m_cfg.materialVolumeMapper) {
    // To make it work with the framework needs a lock guard
    auto mappingState =
        const_cast<Acts::VolumeMaterialMapper::State*>(&m_mappingStateVol);

    for (auto& [idTrack, mTrack] : mtrackCollection) {
      // Map this one onto the geometry
      m_cfg.materialVolumeMapper->mapMaterialTrack(*mappingState, mTrack);
    }
  }
  // Write take the collection to the EventStore
  m_outputMaterialTracks(context, std::move(mtrackCollection));
  return ProcessCode::SUCCESS;
}

std::vector<std::pair<double, int>> MaterialMapping::scoringParameters(
    std::uint64_t surfaceID) {
  std::vector<std::pair<double, int>> scoringParameters;

  if (m_cfg.materialSurfaceMapper) {
    auto surfaceAccumulatedMaterial = m_mappingState.accumulatedMaterial.find(
        Acts::GeometryIdentifier(surfaceID));

    if (surfaceAccumulatedMaterial !=
        m_mappingState.accumulatedMaterial.end()) {
      auto matrixMaterial =
          surfaceAccumulatedMaterial->second.accumulatedMaterial();
      for (const auto& vectorMaterial : matrixMaterial) {
        for (const auto& AccumulatedMaterial : vectorMaterial) {
          auto totalVariance = AccumulatedMaterial.totalVariance();
          scoringParameters.push_back(
              {totalVariance.first, totalVariance.second});
        }
      }
    }
  }
  return scoringParameters;
}

}  // namespace ActsExamples
