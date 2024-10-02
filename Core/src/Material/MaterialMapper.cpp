// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/MaterialMapper.hpp"

Acts::MaterialMapper::MaterialMapper(const Config& cfg,
                                     std::unique_ptr<const Logger> mlogger)
    : m_cfg(cfg), m_logger(std::move(mlogger)) {
  if (m_cfg.assignmentFinder == nullptr) {
    throw std::invalid_argument("The assignment finder is not set");
  }
  if (m_cfg.surfaceMaterialAccumulater == nullptr) {
    throw std::invalid_argument("The surface material accumulater is not set");
  }
}

std::unique_ptr<Acts::MaterialMapper::State> Acts::MaterialMapper::createState()
    const {
  // Create the state
  auto state = std::make_unique<State>();
  // Create the surface material accumulater state
  state->surfaceMaterialAccumulaterState =
      m_cfg.surfaceMaterialAccumulater->createState();
  // Return the state object
  return state;
}

std::pair<Acts::RecordedMaterialTrack, Acts::RecordedMaterialTrack>
Acts::MaterialMapper::mapMaterial(State& state, const GeometryContext& gctx,
                                  const MagneticFieldContext& mctx,
                                  const RecordedMaterialTrack& rmTrack,
                                  const Options& options) const {
  // The recorded material track
  const auto& [starDir, recordedMaterial] = rmTrack;
  const auto& [position, direction] = starDir;
  auto [surfaceAssignments, volumeAssignments] =
      m_cfg.assignmentFinder->assignmentCandidates(gctx, mctx, position,
                                                   direction);

  // The mapped and unmapped material
  RecordedMaterialTrack mappedMaterial = {starDir, {}};
  RecordedMaterialTrack unmappedMaterial = {starDir, {}};
  // Assign the surface interactions
  auto [assigned, unassigned, emptyBinSurfaces] =
      MaterialInteractionAssignment::assign(
          gctx, recordedMaterial.materialInteractions, surfaceAssignments,
          options.assignmentOptions);

  // Record the assigned ones - as mapped ones
  mappedMaterial.second.materialInteractions.insert(
      mappedMaterial.second.materialInteractions.end(), assigned.begin(),
      assigned.end());

  // Record the unassigned ones - as unmapped ones
  unmappedMaterial.second.materialInteractions.insert(
      unmappedMaterial.second.materialInteractions.end(), unassigned.begin(),
      unassigned.end());

  // The material interactions
  m_cfg.surfaceMaterialAccumulater->accumulate(
      *state.surfaceMaterialAccumulaterState.get(), assigned, emptyBinSurfaces);

  // The function to calculate the total material before returning
  auto calculateTotalMaterial = [](RecordedMaterialTrack& rTrack) -> void {
    for (const auto& mi : rTrack.second.materialInteractions) {
      rTrack.second.materialInX0 += mi.materialSlab.thicknessInX0();
      rTrack.second.materialInL0 += mi.materialSlab.thicknessInL0();
    }
  };
  // Fill the totals to the material tracks (useful for debugging)
  calculateTotalMaterial(mappedMaterial);
  calculateTotalMaterial(unmappedMaterial);
  // Return the mapped and unmapped material
  return {mappedMaterial, unmappedMaterial};
}

Acts::MaterialMapper::DetectorMaterialMaps Acts::MaterialMapper::finalizeMaps(
    const State& state) const {
  // The final maps
  DetectorMaterialMaps detectorMaterialMaps;
  // The surface maps
  detectorMaterialMaps.first =
      m_cfg.surfaceMaterialAccumulater->finalizeMaterial(
          *state.surfaceMaterialAccumulaterState.get());

  return detectorMaterialMaps;
}
