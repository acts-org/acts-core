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

/// representation of a drift circle measurement used for track finding
class DriftCircle {
  using Scalar = Acts::ActsScalar;

 public:
  /// Construct the drift circle from the drift radius and tube location
  ///
  /// @param tubePos position of the tube in the station frame
  /// @param driftRadius measured drift radius
  /// @param driftRadiusError error on drift radius
  /// @param stationName station name index
  /// @param stationEta station eta index
  /// @param stationPhi station phi index
  /// @param multilayer multilayer index
  /// @param tubeLayer tube layer index
  /// @param tube tube index
  DriftCircle(const Acts::Vector3&& tubePos, float driftRadius,
              float driftRadiusError, int stationName, int stationEta,
              int stationPhi, int multilayer, int tubeLayer, int tube)
      : m_x(tubePos[Acts::ePos0]),
        m_y(tubePos[Acts::ePos1]),
        m_z(tubePos[Acts::ePos2]),
        m_rho(driftRadius),
        m_sigmaRho(driftRadiusError),
        m_stationName(stationName),
        m_stationEta(stationEta),
        m_stationPhi(stationPhi),
        m_multilayer(multilayer),
        m_tubeLayer(tubeLayer),
        m_tube(tube) {}

  constexpr Scalar x() const { return m_x; }
  constexpr Scalar y() const { return m_y; }
  constexpr Scalar z() const { return m_z; }
  constexpr Scalar rDrift() const { return m_rho; }
  constexpr Scalar rDriftError() const { return m_sigmaRho; }
  constexpr int stationName() const { return m_stationName; }
  constexpr int stationEta() const { return m_stationEta; }
  constexpr int stationPhi() const { return m_stationPhi; }
  constexpr int multilayer() const { return m_multilayer; }
  constexpr int tubeLayer() const { return m_tubeLayer; }
  constexpr int tube() const { return m_tube; }

 private:
  // Global position
  Scalar m_x = 0.0f;
  Scalar m_y = 0.0f;
  Scalar m_z = 0.0f;
  Scalar m_rho = 0.0f;
  Scalar m_sigmaRho = 0.0f;
  int m_stationName = 0;
  int m_stationEta = 0;
  int m_stationPhi = 0;
  int m_multilayer = 0;
  int m_tubeLayer = 0;
  int m_tube = 0;
};

inline bool operator==(const DriftCircle& lhs, const DriftCircle& rhs) {
  return (lhs.stationName() == rhs.stationName() &&
          lhs.stationEta() == rhs.stationEta() &&
          lhs.stationPhi() == rhs.stationPhi() &&
          lhs.multilayer() == rhs.multilayer() &&
          lhs.tubeLayer() == rhs.tubeLayer() && lhs.tube() == rhs.tube() &&
          std::abs(rhs.rDrift() - lhs.rDrift()) < 1.e-8 &&
          std::abs(rhs.rDriftError() - lhs.rDriftError()) < 1.e-8);
}

/// Container of space points.
using DriftCircleContainer = std::vector<DriftCircle>;

}  // namespace ActsExamples
