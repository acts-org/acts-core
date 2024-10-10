// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/IntersectionMaterialAssigner.hpp"

#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/StringHelpers.hpp"

#include <algorithm>

namespace {

std::vector<Acts::SurfaceIntersection> forwardOrderedIntersections(
    const Acts::GeometryContext& gctx, const Acts::Vector3& position,
    const Acts::Vector3& direction,
    const std::vector<const Acts::Surface*>& surfaces) {
  // First deal with the surface intersections
  std::vector<Acts::SurfaceIntersection> sIntersections;
  // Intersect the surfaces
  for (auto& surface : surfaces) {
    // Get the intersection
    auto sMultiIntersection = surface->intersect(
        gctx, position, direction, Acts::BoundaryTolerance::None());

    // Take the closest
    auto closestForward = sMultiIntersection.closestForward();
    if (closestForward.status() >= Acts::IntersectionStatus::reachable &&
        closestForward.pathLength() > 0.0) {
      sIntersections.push_back(closestForward);
      continue;
    }
  }
  // Sort the intersection along the pathlength
  std::ranges::sort(sIntersections,
                    &Acts::SurfaceIntersection::pathLengthOrder);
  return sIntersections;
}

}  // namespace

std::pair<std::vector<Acts::IAssignmentFinder::SurfaceAssignment>,
          std::vector<Acts::IAssignmentFinder::VolumeAssignment>>
Acts::IntersectionMaterialAssigner::assignmentCandidates(
    const GeometryContext& gctx, const MagneticFieldContext& /*mctx*/,
    const Vector3& position, const Vector3& direction) const {
  // The resulting candidates
  std::pair<std::vector<Acts::IAssignmentFinder::SurfaceAssignment>,
            std::vector<Acts::IAssignmentFinder::VolumeAssignment>>
      candidates;

  ACTS_DEBUG("Finding material assignment from position "
             << toString(position) << " and direction " << toString(direction));

  // Try the surfaces first
  auto sIntersections =
      forwardOrderedIntersections(gctx, position, direction, m_cfg.surfaces);
  candidates.first.reserve(sIntersections.size());
  for (auto& sIntersection : sIntersections) {
    candidates.first.push_back(IAssignmentFinder::SurfaceAssignment{
        sIntersection.object(), sIntersection.position(), direction});
  }

  // Now deal with the volume intersections : tracking volume first
  if (!m_cfg.trackingVolumes.empty()) {
    for (auto& trackingVolume : m_cfg.trackingVolumes) {
      // Collect the boundary surfaces
      auto boundarySurfaces = trackingVolume->boundarySurfaces();
      std::vector<const Surface*> tSurfaces;
      for (auto& boundarySurface : boundarySurfaces) {
        tSurfaces.push_back(&(boundarySurface->surfaceRepresentation()));
      }
      // Get the intersections
      auto tIntersections =
          forwardOrderedIntersections(gctx, position, direction, tSurfaces);
      // Entry/exit exists in forward direction
      if (tIntersections.size() == 2u) {
        candidates.second.push_back(IAssignmentFinder::VolumeAssignment{
            InteractionVolume(trackingVolume), tIntersections[0u].position(),
            tIntersections[1u].position()});
      }
    }
  }

  // Now deal with the volume intersections : detector volume
  if (!m_cfg.detectorVolumes.empty()) {
    for (auto& detectorVolume : m_cfg.detectorVolumes) {
      // Collect the portals
      auto portals = detectorVolume->portals();
      std::vector<const Surface*> dSurfaces;
      for (auto& portal : portals) {
        dSurfaces.push_back(&(portal->surface()));
      }
      // Get the intersections
      auto dIntersections =
          forwardOrderedIntersections(gctx, position, direction, dSurfaces);
      // Entry/exit exists in forward direction
      if (dIntersections.size() == 2u) {
        candidates.second.push_back(IAssignmentFinder::VolumeAssignment{
            InteractionVolume(detectorVolume), dIntersections[0u].position(),
            dIntersections[1u].position()});
      }
    }
  }

  ACTS_DEBUG("Found " << candidates.first.size() << " surface candidates and "
                      << candidates.second.size() << " volume candidates");

  // Return the result
  return candidates;
}
