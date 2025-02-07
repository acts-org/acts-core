// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Navigation/NavigationStream.hpp"

#include "Acts/Detector/Portal.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <algorithm>

namespace Acts {

bool NavigationStream::initialize(const GeometryContext& gctx,
                                  const QueryPoint& queryPoint,
                                  const BoundaryTolerance& cTolerance,
                                  double onSurfaceTolerance) {
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
  for (auto& [sIntersection, gen2Portal, portal, bTolerance] : m_candidates) {
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
              Candidate{.intersection = rsIntersection,
                        .gen2Portal = gen2Portal,
                        .portal = portal,
                        .bTolerance = bTolerance});
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
        const auto& [aIntersection, aGen2Portal, aPortal, aTolerance] = a;
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

bool NavigationStream::update(const GeometryContext& gctx,
                              const QueryPoint& queryPoint,
                              double onSurfaceTolerance) {
  // Loop over the (currently valid) candidates and update
  for (; m_currentIndex < m_candidates.size(); ++m_currentIndex) {
    // Get the candidate, and resolve the tuple
    Candidate& candidate = currentCandidate();
    // Get the surface from the object intersection
    const Surface* surface = candidate.intersection.object();
    // (re-)Intersect the surface
    auto multiIntersection =
        surface->intersect(gctx, queryPoint.position, queryPoint.direction,
                           candidate.bTolerance, onSurfaceTolerance);
    // Split them into valid intersections
    for (const auto& rsIntersection : multiIntersection.split()) {
      // Skip wrong index solution
      if (rsIntersection.index() != candidate.intersection.index()) {
        continue;
      }
      // Valid solution is either on surface or updates the distance
      if (rsIntersection.isValid()) {
        candidate.intersection = rsIntersection;
        return true;
      }
    }
  }
  // No candidate was reachable
  return false;
}

void NavigationStream::addSurfaceCandidate(
    const Surface& surface, const BoundaryTolerance& bTolerance) {
  m_candidates.emplace_back(
      Candidate{.intersection = ObjectIntersection<Surface>::invalid(&surface),
                .bTolerance = bTolerance});
}

void NavigationStream::addSurfaceCandidates(
    std::span<const Surface*> surfaces, const BoundaryTolerance& bTolerance) {
  std::ranges::for_each(surfaces, [&](const auto* surface) {
    m_candidates.emplace_back(
        Candidate{.intersection = ObjectIntersection<Surface>::invalid(surface),
                  .bTolerance = bTolerance});
  });
}

void NavigationStream::addPortalCandidate(const Experimental::Portal& portal) {
  m_candidates.emplace_back(Candidate{
      .intersection = ObjectIntersection<Surface>::invalid(&portal.surface()),
      .gen2Portal = &portal,
      .bTolerance = BoundaryTolerance::None()});
}

void NavigationStream::addPortalCandidate(const Portal& portal) {
  m_candidates.emplace_back(Candidate{
      .intersection = ObjectIntersection<Surface>::invalid(&portal.surface()),
      .portal = &portal,
      .bTolerance = BoundaryTolerance::None()});
}

void NavigationStream::addPortalCandidates(
    std::span<const Experimental::Portal*> portals) {
  std::ranges::for_each(portals, [&](const auto& portal) {
    m_candidates.emplace_back(Candidate{
        .intersection =
            ObjectIntersection<Surface>::invalid(&(portal->surface())),
        .gen2Portal = portal,
        .bTolerance = BoundaryTolerance::None()});
  });
}

AppendOnlyNavigationStream::AppendOnlyNavigationStream(NavigationStream& stream)
    : m_stream{&stream} {}

void AppendOnlyNavigationStream::addPortalCandidate(const Portal& portal) {
  m_stream->addPortalCandidate(portal);
}

void AppendOnlyNavigationStream::addSurfaceCandidate(
    const Surface& surface, const BoundaryTolerance& bTolerance) {
  m_stream->addSurfaceCandidate(surface, bTolerance);
}

}  // namespace Acts
