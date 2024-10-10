// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"

#include <cmath>
#include <vector>

#include <boost/container/static_vector.hpp>

namespace ActsExamples {

/// Space point representation of a measurement suitable for track seeding.
class SimSpacePoint {
  using Scalar = Acts::ActsScalar;

 public:
  /// Construct the space point from global position and selected variances.
  ///
  /// @tparam position_t Input position type
  /// @param pos Global position
  /// @param t Global time
  /// @param varRho Measurement variance of the global transverse distance
  /// @param varZ Measurement variance of the global longitudinal position
  /// @param varT Measurement variance of the global time
  /// @param sourceLinks sourceLinks of the measurements
  /// @param topHalfStripLength half of the length of the top strip
  /// @param bottomHalfStripLength half of the length of the bottom strip
  /// @param topStripDirection direction of the top strip
  /// @param bottomStripDirection direction of the bottom strip
  /// @param stripCenterDistance distance between the center of the two strips
  /// @param topStripCenterPosition position of the center of the top strip
  /// @param validDoubleMeasurementDetails boolean to check if double measurements are valid
  template <typename position_t>
  SimSpacePoint(
      const Eigen::MatrixBase<position_t>& pos, std::optional<Scalar> t,
      Scalar varRho, Scalar varZ, std::optional<Scalar> varT,
      boost::container::static_vector<Acts::SourceLink, 2> sourceLinks,
      Scalar topHalfStripLength, Scalar bottomHalfStripLength,
      const Acts::Vector3& topStripDirection,
      const Acts::Vector3& bottomStripDirection,
      const Acts::Vector3& stripCenterDistance,
      const Acts::Vector3& topStripCenterPosition)
      : m_x(pos[Acts::ePos0]),
        m_y(pos[Acts::ePos1]),
        m_z(pos[Acts::ePos2]),
        m_t(t),
        m_rho(std::hypot(m_x, m_y)),
        m_varianceRho(varRho),
        m_varianceZ(varZ),
        m_varianceT(varT),
        m_sourceLinks(std::move(sourceLinks)),
        m_topHalfStripLength(topHalfStripLength),
        m_bottomHalfStripLength(bottomHalfStripLength),
        m_topStripDirection(topStripDirection),
        m_bottomStripDirection(bottomStripDirection),
        m_stripCenterDistance(stripCenterDistance),
        m_topStripCenterPosition(topStripCenterPosition),
        m_validDoubleMeasurementDetails(true) {
    EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(position_t, 3);
  }

  /// Construct the space point from global position and selected variances.
  ///
  /// @tparam position_t Input position type
  /// @param pos Global position
  /// @param t Global time
  /// @param varRho Measurement variance of the global transverse distance
  /// @param varZ Measurement variance of the global longitudinal position
  /// @param varT Measurement variance of the global time
  /// @param sourceLinks sourceLinks of the measurements
  template <typename position_t>
  SimSpacePoint(
      const Eigen::MatrixBase<position_t>& pos, std::optional<Scalar> t,
      Scalar varRho, Scalar varZ, std::optional<Scalar> varT,
      boost::container::static_vector<Acts::SourceLink, 2> sourceLinks)
      : m_x(pos[Acts::ePos0]),
        m_y(pos[Acts::ePos1]),
        m_z(pos[Acts::ePos2]),
        m_t(t),
        m_rho(std::hypot(m_x, m_y)),
        m_varianceRho(varRho),
        m_varianceZ(varZ),
        m_varianceT(varT),
        m_sourceLinks(std::move(sourceLinks)) {
    EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(position_t, 3);
  }

  constexpr Scalar x() const { return m_x; }
  constexpr Scalar y() const { return m_y; }
  constexpr Scalar z() const { return m_z; }
  constexpr std::optional<Scalar> t() const { return m_t; }
  constexpr Scalar r() const { return m_rho; }
  constexpr Scalar varianceR() const { return m_varianceRho; }
  constexpr Scalar varianceZ() const { return m_varianceZ; }
  constexpr std::optional<Scalar> varianceT() const { return m_varianceT; }

  const boost::container::static_vector<Acts::SourceLink, 2>& sourceLinks()
      const {
    return m_sourceLinks;
  }

  constexpr float topHalfStripLength() const { return m_topHalfStripLength; }
  constexpr float bottomHalfStripLength() const {
    return m_bottomHalfStripLength;
  }
  Acts::Vector3 topStripDirection() const { return m_topStripDirection; }
  Acts::Vector3 bottomStripDirection() const { return m_bottomStripDirection; }
  Acts::Vector3 stripCenterDistance() const { return m_stripCenterDistance; }
  Acts::Vector3 topStripCenterPosition() const {
    return m_topStripCenterPosition;
  }
  constexpr bool validDoubleMeasurementDetails() const {
    return m_validDoubleMeasurementDetails;
  }

 private:
  // Global position
  Scalar m_x;
  Scalar m_y;
  Scalar m_z;
  std::optional<Scalar> m_t;
  Scalar m_rho;
  // Variance in rho/z of the global coordinates
  Scalar m_varianceRho;
  Scalar m_varianceZ;
  std::optional<Scalar> m_varianceT;
  // SourceLinks of the corresponding measurements. A Pixel (strip) SP has one
  // (two) sourceLink(s).
  boost::container::static_vector<Acts::SourceLink, 2> m_sourceLinks;

  // half of the length of the top strip
  float m_topHalfStripLength = 0;
  // half of the length of the bottom strip
  float m_bottomHalfStripLength = 0;
  // direction of the top strip
  Acts::Vector3 m_topStripDirection = {0, 0, 0};
  // direction of the bottom strip
  Acts::Vector3 m_bottomStripDirection = {0, 0, 0};
  // distance between the center of the two strips
  Acts::Vector3 m_stripCenterDistance = {0, 0, 0};
  // position of the center of the bottom strip
  Acts::Vector3 m_topStripCenterPosition = {0, 0, 0};
  bool m_validDoubleMeasurementDetails = false;
};

inline bool operator==(const SimSpacePoint& lhs, const SimSpacePoint& rhs) {
  // TODO would it be sufficient to check just the index under the assumption
  //   that the same measurement index always produces the same space point?
  // no need to check r since it is fully defined by x/y

  return (std::equal(lhs.sourceLinks().begin(), lhs.sourceLinks().end(),
                     rhs.sourceLinks().begin(),
                     [](const auto& lsl, const auto& rsl) {
                       return lsl.template get<IndexSourceLink>() ==
                              rsl.template get<IndexSourceLink>();
                     }) &&
          (lhs.x() == rhs.x()) && (lhs.y() == rhs.y()) &&
          (lhs.z() == rhs.z()) && (lhs.t() == rhs.t()) &&
          (lhs.varianceR() == rhs.varianceR()) &&
          (lhs.varianceZ() == rhs.varianceZ()) &&
          (lhs.varianceT() == rhs.varianceT()));
}

/// Container of space points.
using SimSpacePointContainer = std::vector<SimSpacePoint>;

}  // namespace ActsExamples
