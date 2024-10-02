// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/MaterialInteractionAssignment.hpp"

Acts::MaterialInteractionAssignment::Result
Acts::MaterialInteractionAssignment::assign(
    const GeometryContext& gctx,
    const std::vector<MaterialInteraction>& materialInteractions,
    const std::vector<IAssignmentFinder::SurfaceAssignment>&
        intersectedSurfaces,
    const Options& options) {
  // Return container: Assume a high assignment rate
  std::vector<MaterialInteraction> assignedMaterialInteractions;
  assignedMaterialInteractions.reserve(materialInteractions.size());
  // Return container: The unassigned materials
  std::vector<MaterialInteraction> unassignedMaterialInteractions;

  // Check for empty intersection
  if (intersectedSurfaces.empty()) {
    unassignedMaterialInteractions = materialInteractions;
    return {assignedMaterialInteractions, unassignedMaterialInteractions, {}};
  }

  /// Simple matching of material interactions to surfaces - no pre/post
  /// matching
  // -----------------------------------------------------------------------------
  // Double-Loop over the sorted material interactions
  std::size_t is = 0u;
  for (const auto& materialInteraction : materialInteractions) {
    // First check if there is a global veto
    bool veto = false;
    for (const auto& gVeto : options.globalVetos) {
      if (gVeto(materialInteraction)) {
        unassignedMaterialInteractions.push_back(materialInteraction);
        veto = true;
        break;
      }
    }
    // Now veto this assignment
    if (veto) {
      continue;
    }

    // Walk along the sorted intersections
    auto [cSurface, cPosition, cDirection] = intersectedSurfaces[is];
    ActsScalar cDistance = (cPosition - materialInteraction.position).norm();

    // Peak forward to check if you have a closer intersection
    while (
        is + 1u < intersectedSurfaces.size() &&
        (((intersectedSurfaces[is + 1]).position - materialInteraction.position)
             .norm() < cDistance)) {
      // Recalculate the new distance
      ActsScalar nDistance = ((intersectedSurfaces[is + 1]).position -
                              materialInteraction.position)
                                 .norm();
      ++is;
      cDistance = nDistance;
    }

    // Settled on the right intersection
    auto [surface, position, direction] = intersectedSurfaces[is];

    // Calculate the path correction
    ActsScalar pathCorrection =
        surface->pathCorrection(gctx, position, direction);

    // A local veta veto kicked in
    GeometryIdentifier intersectionID = surface->geometryId();
    if (options.localVetos.contains(intersectionID)) {
      const auto& localVeto = *options.localVetos.find(intersectionID);
      if (localVeto(materialInteraction, intersectedSurfaces[is])) {
        unassignedMaterialInteractions.push_back(materialInteraction);
        continue;
      }
    }

    // Assign the material interaction - position stays, but intersection is
    // updated
    MaterialInteraction assignedMaterialInteraction = materialInteraction;
    assignedMaterialInteraction.pathCorrection = pathCorrection;
    assignedMaterialInteraction.surface = surface;
    assignedMaterialInteraction.direction = direction;
    assignedMaterialInteraction.intersection = position;
    assignedMaterialInteraction.intersectionID = intersectionID;
    // Check for possible reassignment
    if (is + 1u < intersectedSurfaces.size() &&
        options.reAssignments.contains(intersectionID)) {
      auto reAssignment = (*options.reAssignments.find(intersectionID));
      reAssignment(assignedMaterialInteraction, intersectedSurfaces[is],
                   intersectedSurfaces[is + 1]);
    }
    assignedMaterialInteractions.push_back(assignedMaterialInteraction);
  }

  // Check which candidate surfaces had an assignment
  std::set<const Surface*> assignedSurfaces;
  for (const auto& assignedMaterialInteraction : assignedMaterialInteractions) {
    assignedSurfaces.insert(assignedMaterialInteraction.surface);
  }

  // Return container: Surfaces without assignments
  // (empty bin correction can use this information)
  std::vector<IAssignmentFinder::SurfaceAssignment> surfacesWithoutAssignments;
  for (const auto& intersectedSurface : intersectedSurfaces) {
    if (!assignedSurfaces.contains(intersectedSurface.surface)) {
      surfacesWithoutAssignments.push_back(intersectedSurface);
    }
  }

  // return the pair of assigned and unassigned material interactions
  return {assignedMaterialInteractions, unassignedMaterialInteractions,
          surfacesWithoutAssignments};
}
