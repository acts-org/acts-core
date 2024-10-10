// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Track.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace ActsExamples {

/// Store reconstructed trajectories from track finding/fitting.
///
/// It contains a MultiTrajectory with a vector of entry indices for
/// individual trajectories, and a map of fitted parameters indexed by the
/// entry index. In case of track fitting, there is at most one trajectory
/// in the MultiTrajectory; In case of track finding, there could be
/// multiple trajectories in the MultiTrajectory.
struct Trajectories final {
 public:
  /// (Reconstructed) trajectory with multiple states.
  using MultiTrajectory = Acts::ConstVectorMultiTrajectory;
  /// Fitted parameters identified by indices in the multi trajectory.
  using IndexedParameters =
      std::unordered_map<Acts::MultiTrajectoryTraits::IndexType,
                         TrackParameters>;

  /// Default construct an empty object. Required for container compatibility
  /// and to signal an error.
  Trajectories() = default;
  /// Construct from fitted multi trajectory and parameters.
  ///
  /// @param multiTraj The multi trajectory
  /// /// @param tTips Tip indices that identify valid trajectories
  /// @param parameters Fitted track parameters indexed by trajectory index
  Trajectories(const MultiTrajectory& multiTraj,
               const std::vector<Acts::MultiTrajectoryTraits::IndexType>& tTips,
               const IndexedParameters& parameters)
      : m_multiTrajectory(&multiTraj),
        m_trackTips(tTips),
        m_trackParameters(parameters) {}

  /// Return true if there exists no valid trajectory.
  bool empty() const { return m_trackTips.empty(); }

  /// Access the underlying multi trajectory.
  const MultiTrajectory& multiTrajectory() const {
    throw_assert(m_multiTrajectory != nullptr, "MultiTrajectory is null");
    return *m_multiTrajectory;
  }

  /// Access the tip indices that identify valid trajectories.
  const std::vector<Acts::MultiTrajectoryTraits::IndexType>& tips() const {
    return m_trackTips;
  }

  /// Check if a trajectory exists for the given index.
  ///
  /// @param entryIndex The trajectory entry index
  /// @return Whether there is trajectory with provided entry index
  bool hasTrajectory(Acts::MultiTrajectoryTraits::IndexType entryIndex) const {
    return (0 < std::count(m_trackTips.begin(), m_trackTips.end(), entryIndex));
  }

  /// Check if fitted track parameters exists for the given index.
  ///
  /// @param entryIndex The trajectory entry index
  /// @return Whether having fitted track parameters or not
  bool hasTrackParameters(
      Acts::MultiTrajectoryTraits::IndexType entryIndex) const {
    return m_trackParameters.contains(entryIndex);
  }

  /// Access the fitted track parameters for the given index.
  ///
  /// @param entryIndex The trajectory entry index
  /// @return The fitted track parameters of the trajectory
  const TrackParameters& trackParameters(
      Acts::MultiTrajectoryTraits::IndexType entryIndex) const {
    auto it = m_trackParameters.find(entryIndex);
    if (it == m_trackParameters.end()) {
      throw std::runtime_error(
          "No fitted track parameters for trajectory with entry index = " +
          std::to_string(entryIndex));
    }
    return it->second;
  }

 private:
  // The track container
  const MultiTrajectory* m_multiTrajectory{nullptr};
  // The entry indices of trajectories stored in multiTrajectory
  std::vector<Acts::MultiTrajectoryTraits::IndexType> m_trackTips = {};
  // The fitted parameters at the provided surface for individual trajectories
  IndexedParameters m_trackParameters = {};
};

/// Container for multiple trajectories.
using TrajectoriesContainer = std::vector<Trajectories>;

}  // namespace ActsExamples
