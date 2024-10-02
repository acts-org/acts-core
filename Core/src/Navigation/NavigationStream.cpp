// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Navigation/NavigationStream.hpp"

#include "Acts/Detector/Portal.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <algorithm>

bool Acts::NavigationStream::initialize(const GeometryContext& gctx,
                                        const QueryPoint& queryPoint,
                                        const BoundaryTolerance& cTolerance,
                                        ActsScalar onSurfaceTolerance) {
  // Position and direction from the query point
  const Vector3& position = queryPoint.position;
  const Vector3& direction = queryPoint.direction;

  // De-duplicate first (necessary to deal correctly with multiple
  // intersections) - sort them by surface pointer
  std::ranges::sort(m_candidates, [](const Candidate& a, const Candidate& b) {
    return (&a.surface()) < (&b.surface());
  });
  // Remove duplicates on basis of the surface pointer
  m_candidates.erase(std::unique(m_candidates.begin(), m_candidates.end(),
                                 [](const Candidate& a, const Candidate& b) {
                                   return (&a.surface()) == (&b.surface());
                                 }),
                     m_candidates.end());

  // A container collecting additional candidates from multiple
  // valid interseciton
  std::vector<Candidate> additionalCandidates = {};
  for (auto& [sIntersection, portal, bTolerance] : m_candidates) {
    // Get the surface from the object intersection
    const Surface* surface = sIntersection.object();
    // Intersect the surface
    auto multiIntersection = surface->intersect(gctx, position, direction,
                                                cTolerance, onSurfaceTolerance);

    // Split them into valid intersections, keep track of potentially
    // additional candidates
    bool originalCandidateUpdated = false;
    for (const auto& rsIntersection : multiIntersection.split()) {
      // Skip negative solutions, respecting the on surface tolerance
      if (rsIntersection.pathLength() < -onSurfaceTolerance) {
        continue;
      }
      // Valid solution is either on surface or updates the distance
      if (rsIntersection.isValid()) {
        if (!originalCandidateUpdated) {
          sIntersection = rsIntersection;
          originalCandidateUpdated = true;
        } else {
          additionalCandidates.emplace_back(
              Candidate{rsIntersection, portal, bTolerance});
        }
      }
    }
  }

  // Append the multi intersection candidates
  m_candidates.insert(m_candidates.end(), additionalCandidates.begin(),
                      additionalCandidates.end());

  // Sort the candidates by path length
  std::ranges::sort(m_candidates, Candidate::pathLengthOrder);

  // The we find the first invalid candidate
  auto firstInvalid =
      std::ranges::find_if(m_candidates, [](const Candidate& a) {
        const auto& [aIntersection, aPortal, aTolerance] = a;
        return !aIntersection.isValid();
      });

  // Set the range and initialize
  m_candidates.resize(std::distance(m_candidates.begin(), firstInvalid));

  m_currentIndex = 0;
  if (m_candidates.empty()) {
    return false;
  }
  return true;
}

bool Acts::NavigationStream::update(const GeometryContext& gctx,
                                    const QueryPoint& queryPoint,
                                    ActsScalar onSurfaceTolerance) {
  // Position and direction from the query point
  const Vector3& position = queryPoint.position;
  const Vector3& direction = queryPoint.direction;

  // Loop over the (currently valid) candidates and update
  for (; m_currentIndex < m_candidates.size(); ++m_currentIndex) {
    // Get the candidate, and resolve the tuple
    Candidate& candidate = currentCandidate();
    auto& [sIntersection, portal, bTolerance] = candidate;
    // Get the surface from the object intersection
    const Surface* surface = sIntersection.object();
    // (re-)Intersect the surface
    auto multiIntersection = surface->intersect(gctx, position, direction,
                                                bTolerance, onSurfaceTolerance);
    // Split them into valid intersections
    for (const auto& rsIntersection : multiIntersection.split()) {
      // Skip wrong index solution
      if (rsIntersection.index() != sIntersection.index()) {
        continue;
      }
      // Valid solution is either on surface or updates the distance
      if (rsIntersection.isValid()) {
        sIntersection = rsIntersection;
        return true;
      }
    }
  }
  // No candidate was reachable
  return false;
}

void Acts::NavigationStream::addSurfaceCandidate(
    const Surface* surface, const BoundaryTolerance& bTolerance) {
  m_candidates.emplace_back(Candidate{
      ObjectIntersection<Surface>::invalid(surface), nullptr, bTolerance});
}

void Acts::NavigationStream::addSurfaceCandidates(
    const std::vector<const Surface*>& surfaces,
    const BoundaryTolerance& bTolerance) {
  std::ranges::for_each(surfaces, [&](const auto* surface) {
    m_candidates.emplace_back(Candidate{
        ObjectIntersection<Surface>::invalid(surface), nullptr, bTolerance});
  });
}

void Acts::NavigationStream::addPortalCandidate(const Portal* portal) {
  m_candidates.emplace_back(
      Candidate{ObjectIntersection<Surface>::invalid(&(portal->surface())),
                portal, BoundaryTolerance::None()});
}

void Acts::NavigationStream::addPortalCandidates(
    const std::vector<const Portal*>& portals) {
  std::ranges::for_each(portals, [&](const auto& portal) {
    m_candidates.emplace_back(
        Candidate{ObjectIntersection<Surface>::invalid(&(portal->surface())),
                  portal, BoundaryTolerance::None()});
  });
}
