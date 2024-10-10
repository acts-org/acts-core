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

#include <cmath>
#include <optional>
#include <vector>

#include <boost/container/static_vector.hpp>

namespace Acts::Test {

/// Space point representation of a measurement suitable for track seeding.
class TestSpacePoint {
 public:
  TestSpacePoint() = default;

  /// Construct the space point from global position and selected variances.
  ///
  /// @tparam position_t Input position type
  /// @param pos Global position
  /// @param varRho Measurement variance of the global transverse distance
  /// @param varZ Measurement variance of the global longitudinal position
  /// @param measurementIndices Indices of the underlying measurement
  template <typename position_t>
  TestSpacePoint(
      const Eigen::MatrixBase<position_t>& pos, std::optional<float> t,
      float varRho, float varZ, std::optional<float> varT,
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

  constexpr float x() const { return m_x; }
  constexpr float y() const { return m_y; }
  constexpr float z() const { return m_z; }
  constexpr std::optional<float> t() const { return m_z; }
  constexpr float r() const { return m_rho; }
  constexpr float varianceR() const { return m_varianceRho; }
  constexpr float varianceZ() const { return m_varianceZ; }
  constexpr std::optional<float> varianceT() const { return m_varianceT; }

  const boost::container::static_vector<Acts::SourceLink, 2>& sourceLinks()
      const {
    return m_sourceLinks;
  }

 private:
  // Global position
  float m_x = 0;
  float m_y = 0;
  float m_z = 0;
  std::optional<float> m_t;
  float m_rho = 0;
  // Variance in rho/z of the global coordinates
  float m_varianceRho = 0;
  float m_varianceZ = 0;
  std::optional<float> m_varianceT;
  // source links. A Pixel (strip) SP has one (two) sourceLink(s).
  boost::container::static_vector<Acts::SourceLink, 2> m_sourceLinks;
};

inline bool operator==(const TestSpacePoint& lhs, const TestSpacePoint& rhs) {
  return (std::equal(
              lhs.sourceLinks().begin(), lhs.sourceLinks().end(),
              rhs.sourceLinks().begin(),
              [](const auto& lsl, const auto& rsl) {
                return lsl.template get<Acts::detail::Test::TestSourceLink>() ==
                       rsl.template get<Acts::detail::Test::TestSourceLink>();
              }) &&
          lhs.x() == rhs.x()) &&
         (lhs.y() == rhs.y()) && (lhs.z() == rhs.z()) && (lhs.t() == rhs.t()) &&
         (lhs.varianceR() == rhs.varianceR()) &&
         (lhs.varianceZ() == rhs.varianceZ()) &&
         (lhs.varianceT() == rhs.varianceT());
}

/// Container of space points.
using TestSpacePointContainer = std::vector<TestSpacePoint>;

}  // namespace Acts::Test
