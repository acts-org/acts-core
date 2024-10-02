// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/GaussianTrackDensity.hpp"
#include "Acts/Vertexing/IVertexFinder.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"

namespace Acts {

/// @class TrackDensityVertexFinder
///
/// @brief Finds a vertex seed based on the maximum
/// of a track density function.
/// Each track is modelled as a 2d density
/// function around its d0/z0 perigee parameter values.
/// The z seed position is then found as the position
/// of the maximum of all summed track density functions.
///
/// Ref. (1): https://cds.cern.ch/record/2670380
class TrackDensityVertexFinder final : public IVertexFinder {
 public:
  /// @brief The Config struct
  struct Config {
    // The track density estimator
    GaussianTrackDensity trackDensityEstimator;
  };

  /// State struct for fulfilling interface
  struct State {};

  /// @brief Function that finds single vertex candidate
  ///
  /// @param trackVector Input track collection
  /// @param vertexingOptions Vertexing options
  /// @param state State for fulfilling interfaces
  ///
  /// @return Vector of vertices, filled with a single
  ///         vertex (for consistent interfaces)
  Result<std::vector<Vertex>> find(const std::vector<InputTrack>& trackVector,
                                   const VertexingOptions& vertexingOptions,
                                   IVertexFinder::State& state) const override;

  IVertexFinder::State makeState(
      const Acts::MagneticFieldContext& /*mctx*/) const override {
    return IVertexFinder::State{State{}};
  }

  void setTracksToRemove(
      IVertexFinder::State& /*state*/,
      const std::vector<InputTrack>& /*removedTracks*/) const override {
    // Nothing to do here
  }

  /// @brief Constructor for user-defined InputTrack type
  ///
  /// @param cfg Configuration object
  TrackDensityVertexFinder(const Config& cfg) : m_cfg(cfg) {}

 private:
  Config m_cfg;
};

}  // namespace Acts
