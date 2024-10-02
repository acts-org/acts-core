// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/MaterialValidater.hpp"

#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/interface/IAssignmentFinder.hpp"
#include "Acts/Utilities/StringHelpers.hpp"

Acts::MaterialValidater::MaterialValidater(
    const Acts::MaterialValidater::Config& cfg,
    std::unique_ptr<const Acts::Logger> mlogger)
    : m_cfg(cfg), m_logger(std::move(mlogger)) {
  if (m_cfg.materialAssigner == nullptr) {
    throw std::invalid_argument("Missing material assigner");
  }
}

Acts::RecordedMaterialTrack Acts::MaterialValidater::recordMaterial(
    const GeometryContext& gctx, const MagneticFieldContext& mctx,
    const Vector3& position, const Vector3& direction) const {
  ACTS_DEBUG("MaterialValidater::recordMaterial with position "
             << toString(position) << " and direction " << toString(direction));

  // Prepare the material track
  Acts::RecordedMaterialTrack mTrack{{position, direction}, {}};

  auto [surfaceAssignments, volumeAssignments] =
      m_cfg.materialAssigner->assignmentCandidates(gctx, mctx, position,
                                                   direction);

  for (const auto& [surface, sposition, sdirection] : surfaceAssignments) {
    // The slab and the path correction
    auto materialSlab = surface->surfaceMaterial()->materialSlab(sposition);
    auto pathCorrection = surface->pathCorrection(gctx, sposition, sdirection);
    // Get the material information
    Acts::MaterialInteraction mInteraction;
    mInteraction.surface = surface;
    mInteraction.position = sposition;
    mInteraction.direction = sdirection;
    mInteraction.materialSlab = MaterialSlab(
        materialSlab.material(), materialSlab.thickness() * pathCorrection);
    mInteraction.pathCorrection = pathCorrection;
    mInteraction.intersection = sposition;
    mInteraction.intersectionID = surface->geometryId();
    // Assemble the recorded material track
    mTrack.second.materialInX0 += mInteraction.materialSlab.thicknessInX0();
    mTrack.second.materialInL0 += mInteraction.materialSlab.thicknessInL0();
    mTrack.second.materialInteractions.push_back(mInteraction);
  }

  ACTS_VERBOSE("Recorded material track with "
               << mTrack.second.materialInteractions.size()
               << " material interactions.");

  return mTrack;
}
