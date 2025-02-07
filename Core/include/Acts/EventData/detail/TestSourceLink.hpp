// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"

#include <array>
#include <cstddef>
#include <iosfwd>
#include <stdexcept>

namespace Acts::detail::Test {

struct TestSourceLinkSurfaceAccessor;

/// A minimal source link implementation for testing.
///
/// Instead of storing a reference to a measurement or raw data, the measurement
/// data is stored inline directly in the source link. Only 1d or 2d
/// measurements are supported to limit the overhead. Additionally, a source
/// identifier is stored that can be used to store additional information. How
/// this is interpreted depends on the specific tests.
struct TestSourceLink final {
  using SurfaceAccessor = TestSourceLinkSurfaceAccessor;

  GeometryIdentifier m_geometryId{};
  std::size_t sourceId = 0u;
  // use eBoundSize to indicate unused indices
  std::array<BoundIndices, 2> indices = {eBoundSize, eBoundSize};
  Acts::Vector2 parameters;
  Acts::ActsSquareMatrix<2> covariance;

  /// Construct a source link for a 1d measurement.
  TestSourceLink(BoundIndices idx, double val, double var,
                 GeometryIdentifier gid = GeometryIdentifier(),
                 std::size_t sid = 0u)
      : m_geometryId(gid),
        sourceId(sid),
        indices{idx, eBoundSize},
        parameters(val, 0),
        covariance(Acts::Vector2(var, 0).asDiagonal()) {}
  /// Construct a source link for a 2d measurement.
  TestSourceLink(BoundIndices idx0, BoundIndices idx1,
                 const Acts::Vector2& params,
                 const Acts::ActsSquareMatrix<2>& cov,
                 GeometryIdentifier gid = GeometryIdentifier(),
                 std::size_t sid = 0u)
      : m_geometryId(gid),
        sourceId(sid),
        indices{idx0, idx1},
        parameters(params),
        covariance(cov) {}
  /// Default-construct an invalid source link to satisfy SourceLinkConcept.
  TestSourceLink() = default;
  TestSourceLink(const TestSourceLink&) = default;
  TestSourceLink(TestSourceLink&&) = default;
  TestSourceLink& operator=(const TestSourceLink&) = default;
  TestSourceLink& operator=(TestSourceLink&&) = default;

  bool operator==(const TestSourceLink& rhs) const {
    return (m_geometryId == rhs.m_geometryId) && (sourceId == rhs.sourceId) &&
           (indices == rhs.indices) && (parameters == rhs.parameters) &&
           (covariance == rhs.covariance);
  }

  std::ostream& print(std::ostream& os) const {
    os << "TestsSourceLink(geometryId=" << m_geometryId
       << ",sourceId=" << sourceId;
    if (indices[0] != eBoundSize) {
      os << ",index0=" << indices[0];
    }
    if (indices[1] != eBoundSize) {
      os << ",index1=" << indices[1];
    }
    os << ")";
    return os;
  }
  constexpr std::size_t index() const { return sourceId; }
};

struct TestSourceLinkSurfaceAccessor {
  const TrackingGeometry& geometry;

  const Acts::Surface* operator()(const Acts::SourceLink& sourceLink) const {
    const auto& testSourceLink = sourceLink.get<TestSourceLink>();
    return geometry.findSurface(testSourceLink.m_geometryId);
  }
};

namespace Experimental {

struct TestSourceLinkSurfaceAccessor {
  const Acts::Experimental::Detector& geometry;

  const Acts::Surface* operator()(const Acts::SourceLink& sourceLink) const {
    const auto& testSourceLink = sourceLink.get<TestSourceLink>();
    return geometry.findSurface(testSourceLink.m_geometryId);
  }
};

}  // namespace Experimental

inline std::ostream& operator<<(std::ostream& os,
                                const TestSourceLink& sourceLink) {
  return sourceLink.print(os);
}

/// Extract the measurement from a TestSourceLink.
///
/// @param gctx Unused
/// @param trackState TrackState to calibrated
template <typename trajectory_t>
void testSourceLinkCalibrator(
    const GeometryContext& /*gctx*/, const CalibrationContext& /*cctx*/,
    const SourceLink& sourceLink,
    typename trajectory_t::TrackStateProxy trackState) {
  TestSourceLink sl = sourceLink.template get<TestSourceLink>();

  trackState.setUncalibratedSourceLink(SourceLink{sourceLink});

  if ((sl.indices[0] != Acts::eBoundSize) &&
      (sl.indices[1] != Acts::eBoundSize)) {
    trackState.allocateCalibrated(2);
    trackState.template calibrated<2>() = sl.parameters;
    trackState.template calibratedCovariance<2>() = sl.covariance;
    trackState.setProjectorSubspaceIndices(
        std::array{sl.indices[0], sl.indices[1]});
  } else if (sl.indices[0] != Acts::eBoundSize) {
    trackState.allocateCalibrated(1);
    trackState.template calibrated<1>() = sl.parameters.head<1>();
    trackState.template calibratedCovariance<1>() =
        sl.covariance.topLeftCorner<1, 1>();
    trackState.setProjectorSubspaceIndices(std::array{sl.indices[0]});
  } else {
    throw std::runtime_error(
        "Tried to extract measurement from invalid TestSourceLink");
  }
}

}  // namespace Acts::detail::Test
