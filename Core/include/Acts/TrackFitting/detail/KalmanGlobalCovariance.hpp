// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <unordered_map>

namespace Acts::detail {

/// Calculate the global track parameters covariance for a smoothed trajectory
/// stored in MultiTrajecty based on formulas at Journal of Physics: Conference
/// Series 219 (2010) 032028.
///
/// @tparam source_link_t The source link type of the trajectory
/// @tparam parameters_t The track parameters type
///
/// @param multiTraj The MultiTrajectory containing the trajectory to be
/// investigated
/// @param entryIndex The trajectory entry index
///
/// @return The global track parameters covariance matrix and the starting
/// row/column for smoothed states
template <typename traj_t, typename parameters_t = BoundTrackParameters>
std::pair<ActsDynamicMatrix, std::unordered_map<std::size_t, std::size_t>>
globalTrackParametersCovariance(const traj_t& multiTraj,
                                const std::size_t& entryIndex) {
  using CovMatrix = typename parameters_t::CovarianceMatrix;
  using GainMatrix = CovMatrix;

  // The last smoothed state index
  std::size_t lastSmoothedIndex = Acts::MultiTrajectoryTraits::kInvalid;
  // The total number of smoothed states
  std::size_t nSmoothedStates = 0;
  // Visit all the states
  multiTraj.visitBackwards(entryIndex, [&](const auto& ts) {
    if (ts.hasSmoothed()) {
      if (lastSmoothedIndex == Acts::MultiTrajectoryTraits::kInvalid) {
        lastSmoothedIndex = ts.index();
      }
      nSmoothedStates++;
    }
  });

  // Set the size of global track parameters covariance for all smoothed states
  ActsDynamicMatrix fullGlobalTrackParamsCov(nSmoothedStates * eBoundSize,
                                             nSmoothedStates * eBoundSize);
  fullGlobalTrackParamsCov.setZero();
  // The index of state within the trajectory and the starting row/column for
  // this state in the global covariance matrix
  std::unordered_map<std::size_t, std::size_t> stateRowIndices;
  // Visit the smoothed states to calculate the full global track parameters
  // covariance
  std::size_t nProcessed = 0;
  auto prev_ts = multiTraj.getTrackState(lastSmoothedIndex);
  multiTraj.visitBackwards(lastSmoothedIndex, [&](const auto& ts) {
    const std::size_t iRow =
        fullGlobalTrackParamsCov.rows() - eBoundSize * (nProcessed + 1);
    // Fill the covariance of this state
    fullGlobalTrackParamsCov.block<eBoundSize, eBoundSize>(iRow, iRow) =
        ts.smoothedCovariance();
    // Fill the correlation between this state (indexed by i-1) and
    // beforehand smoothed states (indexed by j): C^n_{i-1, j}= G_{i-1} *
    // C^n_{i, j} for i <= j
    if (nProcessed > 0) {
      // Calculate the gain matrix
      GainMatrix G = ts.filteredCovariance() * prev_ts.jacobian().transpose() *
                     prev_ts.predictedCovariance().inverse();
      // Loop over the beforehand smoothed states
      for (std::size_t iProcessed = 1; iProcessed <= nProcessed; iProcessed++) {
        const std::size_t iCol = iRow + eBoundSize * iProcessed;
        CovMatrix prev_correlation =
            fullGlobalTrackParamsCov.block<eBoundSize, eBoundSize>(
                iRow + eBoundSize, iCol);
        CovMatrix correlation = G * prev_correlation;
        fullGlobalTrackParamsCov.block<eBoundSize, eBoundSize>(iRow, iCol) =
            correlation;
        fullGlobalTrackParamsCov.block<eBoundSize, eBoundSize>(iCol, iRow) =
            correlation.transpose();
      }
    }
    stateRowIndices.emplace(ts.index(), iRow);
    nProcessed++;
    prev_ts = ts;
  });

  return std::make_pair(fullGlobalTrackParamsCov, stateRowIndices);
}

}  // namespace Acts::detail
