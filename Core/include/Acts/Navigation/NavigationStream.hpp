// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/Portal.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Utilities/Intersection.hpp"

#include <span>
#include <vector>

namespace Acts {

// To be removed when the namespace Experimental is omitted
namespace Experimental {
class Portal;
}

class Surface;

/// The NavigationStream is a container for the navigation candidates that
/// are currentlu processed in a given context. The context could be local to a
/// volume, or global to an entire track following.
///
/// The current candidates are stored in a vector of candidates, where an index
/// is used to indicate the current active candidate.
class NavigationStream {
 public:
  /// The query point for the navigation stream
  ///
  /// This holds the position and direction from which the navigation stream
  /// should either be initialized or updated.
  struct QueryPoint {
    /// The position of the query point
    Vector3 position = Vector3::Zero();
    /// The direction of the query point
    Vector3 direction = Vector3::Zero();
  };

  /// This is a candidate object of the navigation stream, it holds:
  ///
  /// - a Surface intersection
  /// - a Portal : set if the surface represents a portal
  /// - a BoundaryTolerance : the boundary tolerance used for the intersection
  struct Candidate {
    /// The intersection
    ObjectIntersection<Surface> intersection =
        ObjectIntersection<Surface>::invalid();
    /// The portal
    const Acts::Experimental::Portal* gen2Portal = nullptr;
    const Portal* portal = nullptr;
    /// The boundary tolerance
    BoundaryTolerance bTolerance = BoundaryTolerance::None();
    /// Convenience access to surface
    const Surface& surface() const { return *intersection.object(); }
    /// Cinvencience access to the path length
    double pathLength() const { return intersection.pathLength(); }

    /// Order along the path length
    ///
    /// @param aCandidate is the first candidate
    /// @param bCandidate is the second candidate
    ///
    /// @return true if aCandidate is closer to the origin
    constexpr static bool pathLengthOrder(const Candidate& aCandidate,
                                          const Candidate& bCandidate) {
      return ObjectIntersection<Surface>::pathLengthOrder(
          aCandidate.intersection, bCandidate.intersection);
    }
  };

  /// Switch to next next candidate
  ///
  /// @return true if a next candidate is available
  bool switchToNextCandidate() {
    if (m_currentIndex < m_candidates.size()) {
      ++m_currentIndex;
      return true;
    }
    return false;
  }

  /// Const access the current candidate
  const Candidate& currentCandidate() const {
    return m_candidates.at(m_currentIndex);
  }

  /// Current Index
  std::size_t currentIndex() const { return m_currentIndex; }

  /// Non-cost access the candidate vector
  std::vector<Candidate>& candidates() { return m_candidates; }

  /// Const access the candidate vector
  const std::vector<Candidate>& candidates() const { return m_candidates; }

  /// Non-cost access the current candidate
  ///
  /// This will throw and out of bounds exception if the stream is not
  /// valid anymore.
  Candidate& currentCandidate() { return m_candidates.at(m_currentIndex); }

  /// The number of active candidates
  std::size_t remainingCandidates() const {
    return (m_candidates.size() - m_currentIndex);
  }

  /// Fill one surface into the candidate vector
  ///
  /// @param surface the surface to be filled
  /// @param bTolerance the boundary tolerance used for the intersection
  void addSurfaceCandidate(const Surface& surface,
                           const BoundaryTolerance& bTolerance);

  /// Fill n surfaces into the candidate vector
  ///
  /// @param surfaces the surfaces that are filled in
  /// @param bTolerance the boundary tolerance used for the intersection
  void addSurfaceCandidates(std::span<const Surface*> surfaces,
                            const BoundaryTolerance& bTolerance);

  /// Fill one portal into the candidate vector
  ///
  void addPortalCandidate(const Experimental::Portal& portal);
  /// @param portal the portals that are filled in

  void addPortalCandidate(const Portal& portal);

  /// Fill n portals into the candidate vector
  ///
  /// @param portals the portals that are filled in
  void addPortalCandidates(std::span<const Experimental::Portal*> portals);

  /// Initialize the stream from a query point
  ///
  /// @param gctx is the geometry context
  /// @param queryPoint holds current position, direction, etc.
  /// @param cTolerance is the candidate search tolerance
  /// @param onSurfaceTolerance is the tolerance for on-surface intersections
  ///
  /// This method will first de-duplicate the candidates on basis of the surface
  /// pointer to make sure that the multi-intersections are handled correctly.
  /// This will allow intializeStream() to be called even as a re-initialization
  /// and still work correctly with at one time valid candidates.
  ///
  /// @return true if the stream is active, false indicates that there are no valid candidates
  bool initialize(const GeometryContext& gctx,
                  const NavigationStream::QueryPoint& queryPoint,
                  const BoundaryTolerance& cTolerance,
                  double onSurfaceTolerance = s_onSurfaceTolerance);

  /// Convenience method to update a stream from a new query point,
  /// this could be called from navigation delegates that do not require
  /// a local state or from the navigator on the target stream
  ///
  /// @param gctx is the geometry context
  /// @param queryPoint holds current position, direction, etc.
  /// @param onSurfaceTolerance is the tolerance for on-surface intersections
  ///
  /// @return true if the stream is active, false indicate no valid candidates left
  bool update(const GeometryContext& gctx,
              const NavigationStream::QueryPoint& queryPoint,
              double onSurfaceTolerance = s_onSurfaceTolerance);

 private:
  /// The candidates of this navigation stream
  std::vector<Candidate> m_candidates;

  /// The currently active candidate
  std::size_t m_currentIndex = 0u;
};

struct AppendOnlyNavigationStream {
  explicit AppendOnlyNavigationStream(NavigationStream& stream);
  void addSurfaceCandidate(const Surface& surface,
                           const BoundaryTolerance& bTolerance);
  void addPortalCandidate(const Portal& portal);

 private:
  NavigationStream* m_stream;
};

}  // namespace Acts
