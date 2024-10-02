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
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/FullBilloirVertexFitter.hpp"
#include "Acts/Vertexing/HelicalTrackLinearizer.hpp"
#include "Acts/Vertexing/IVertexFinder.hpp"
#include "Acts/Vertexing/ImpactPointEstimator.hpp"
#include "Acts/Vertexing/TrackLinearizer.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"

#include <functional>

namespace Acts {

/// @class IterativeVertexFinder
///
/// @brief Implements an iterative vertex finder
///
////////////////////////////////////////////////////////////
///
/// Brief description of the algorithm implemented:
/// Iterative vertex finder which iteratively finds and fits vertices:
/// 1. A list of seed tracks (`seedTracks`, which is the same as the
///   input track list to the finder at the very first iteration) is used
///   to retrieve a single vertex seed using the ZScanVertexFinder.
/// 2. All tracks compatible with the current vertex seed are kept and used
///   for fitting the single vertex.
/// 3.1 If the vertex is a 'good' vertex (i.e. meets requirements) and no
///   track reassignment after first fit is required, go to step 4. If vertex
///   is not a good vertex, remove all tracks in tracksToFit from seedTracks.
/// 3.2 If vertex meets requirements and track reassignment after first fit
///   is required, iterate over all previously found vertices ("old vertex")
///   and over all their tracksAtVertex. Compare compatibility of each track
///   with old vertex and current vertex. If track is more compatible with
///   current vertex, remove track from old vertex, put track back to
///   tracksToFit and refit current vertex with additional track.
/// 4. If good vertex, `removeUsedCompatibleTracks` method is called, which
///   removes all used tracks that are compatible with the fitted vertex
///   from `tracksToFit` and `seedTracks`. It also removes outliers tracks
///   from tracksAtVertex if not compatible.
/// 5. Add vertex to vertexCollection
/// 6. Repeat until no seedTracks are left or max. number of vertices found
class IterativeVertexFinder final : public IVertexFinder {
  using VertexFitter = FullBilloirVertexFitter;

 public:
  /// Configuration struct
  struct Config {
    /// @brief Config constructor
    ///
    /// @param fitter Vertex fitter
    /// @param sfinder The seed finder
    /// @param est ImpactPointEstimator
    Config(VertexFitter fitter, std::shared_ptr<IVertexFinder> sfinder,
           ImpactPointEstimator est)
        : vertexFitter(std::move(fitter)),
          seedFinder(std::move(sfinder)),
          ipEst(std::move(est)) {}

    /// Vertex fitter
    VertexFitter vertexFitter;

    /// Track linearizer
    TrackLinearizer trackLinearizer;

    /// Vertex seed finder
    std::shared_ptr<IVertexFinder> seedFinder;

    /// ImpactPointEstimator
    ImpactPointEstimator ipEst;

    /// Vertex finder configuration variables.
    /// Tracks that are within a distance of
    ///
    /// significanceCutSeeding * sqrt(sigma(d0)^2+sigma(z0)^2)
    ///
    /// are considered compatible with the vertex.
    double significanceCutSeeding = 10;
    double maximumChi2cutForSeeding = 36.;
    int maxVertices = 50;

    /// Assign a certain fraction of compatible tracks to a different (so-called
    /// split) vertex if boolean is set to true.
    bool createSplitVertices = false;
    /// Inverse of the fraction of tracks that will be assigned to the split
    /// vertex. E.g., if splitVerticesTrkInvFraction = 2, about 50% of
    /// compatible tracks will be assigned to the split vertex.
    int splitVerticesTrkInvFraction = 2;
    bool reassignTracksAfterFirstFit = false;
    bool doMaxTracksCut = false;
    int maxTracks = 5000;
    double cutOffTrackWeight = 0.01;
    /// If `reassignTracksAfterFirstFit` is set this threshold will be used to
    /// decide if a track should be checked for reassignment to other vertices
    double cutOffTrackWeightReassign = 1;

    /// Function to extract parameters from InputTrack
    InputTrack::Extractor extractParameters;

    /// Magnetic field provider
    std::shared_ptr<const MagneticFieldProvider> field;
  };

  /// State struct
  struct State {
    State(const MagneticFieldProvider& field,
          const Acts::MagneticFieldContext& _magContext)
        : magContext(_magContext),
          ipState{field.makeCache(magContext)},
          fieldCache(field.makeCache(magContext)) {}

    std::reference_wrapper<const Acts::MagneticFieldContext> magContext;

    /// The IP estimator state
    ImpactPointEstimator::State ipState;

    MagneticFieldProvider::Cache fieldCache;
  };

  /// @brief Constructor for user-defined InputTrack type
  ///
  /// @param cfg Configuration object
  /// @param logger The logging instance
  IterativeVertexFinder(Config cfg,
                        std::unique_ptr<const Logger> logger = getDefaultLogger(
                            "IterativeVertexFinder", Logging::INFO));

  /// @brief Finds vertices corresponding to input trackVector
  ///
  /// @param trackVector Input tracks
  /// @param vertexingOptions Vertexing options
  /// @param anyState State for fulfilling interfaces
  ///
  /// @return Collection of vertices found by finder
  Result<std::vector<Vertex>> find(
      const std::vector<InputTrack>& trackVector,
      const VertexingOptions& vertexingOptions,
      IVertexFinder::State& anyState) const override;

  IVertexFinder::State makeState(
      const MagneticFieldContext& mctx) const override {
    return IVertexFinder::State{State{*m_cfg.field, mctx}};
  }

  void setTracksToRemove(
      IVertexFinder::State& /*anyState*/,
      const std::vector<InputTrack>& /*removedTracks*/) const override {
    // Nothing to do here
  }

 private:
  /// Configuration object
  const Config m_cfg;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to logging instance
  const Logger& logger() const { return *m_logger; }

  /// @brief Method that calls seed finder to retrieve a vertex seed
  ///
  /// @param state The state object
  /// @param seedTracks Seeding tracks
  /// @param vertexingOptions Vertexing options
  ///
  /// @return Vertex seed
  Result<std::optional<Vertex>> getVertexSeed(
      State& state, const std::vector<InputTrack>& seedTracks,
      const VertexingOptions& vertexingOptions) const;

  /// @brief Removes all tracks in tracksToRemove from seedTracks
  ///
  /// @param tracksToRemove Tracks to be removed from seedTracks
  /// @param seedTracks List to remove tracks from
  void removeTracks(const std::vector<InputTrack>& tracksToRemove,
                    std::vector<InputTrack>& seedTracks) const;

  /// @brief Function for calculating how compatible
  /// a given track is to a given vertex
  ///
  /// @param params Track parameters
  /// @param vertex The vertex
  /// @param perigeeSurface The perigee surface at vertex position
  /// @param vertexingOptions Vertexing options
  /// @param state The state object
  Result<double> getCompatibility(const BoundTrackParameters& params,
                                  const Vertex& vertex,
                                  const Surface& perigeeSurface,
                                  const VertexingOptions& vertexingOptions,
                                  State& state) const;

  /// @brief Function that removes used tracks compatible with
  /// current vertex (`vertex`) from `tracksToFit` and `seedTracks`
  /// as well as outliers from vertex.tracksAtVertex
  ///
  /// @param vertex Current vertex
  /// @param tracksToFit Tracks used to fit `vertex`
  /// @param seedTracks Tracks used for vertex seeding
  /// @param vertexingOptions Vertexing options
  /// @param state The state object
  Result<void> removeUsedCompatibleTracks(
      Vertex& vertex, std::vector<InputTrack>& tracksToFit,
      std::vector<InputTrack>& seedTracks,
      const VertexingOptions& vertexingOptions, State& state) const;

  /// @brief Function that fills vector with tracks compatible with seed vertex
  ///
  /// @param seedTracks List of all available tracks used for seeding
  /// @param seedVertex Seed vertex
  /// @param tracksToFitOut Tracks to fit
  /// @param tracksToFitSplitVertexOut Tracks to fit to split vertex
  /// @param vertexingOptions Vertexing options
  /// @param state The state object
  Result<void> fillTracksToFit(
      const std::vector<InputTrack>& seedTracks, const Vertex& seedVertex,
      std::vector<InputTrack>& tracksToFitOut,
      std::vector<InputTrack>& tracksToFitSplitVertexOut,
      const VertexingOptions& vertexingOptions, State& state) const;

  /// @brief Function that reassigns tracks from other vertices
  ///        to the current vertex if they are more compatible
  ///
  /// @param vertexCollection Collection of vertices
  /// @param currentVertex Current vertex to assign tracks to
  /// @param tracksToFit Tracks to fit vector
  /// @param seedTracks Seed tracks vector
  /// @param origTracks Vector of original track objects
  /// @param vertexingOptions Vertexing options
  /// @param state The state object
  ///
  /// @return Bool if currentVertex is still a good vertex
  Result<bool> reassignTracksToNewVertex(
      std::vector<Vertex>& vertexCollection, Vertex& currentVertex,
      std::vector<InputTrack>& tracksToFit, std::vector<InputTrack>& seedTracks,
      const std::vector<InputTrack>& origTracks,
      const VertexingOptions& vertexingOptions, State& state) const;

  /// @brief Counts all tracks that are significant for a vertex
  ///
  /// @param vtx The vertex
  ///
  /// @return Number of significant tracks
  int countSignificantTracks(const Vertex& vtx) const;
};

}  // namespace Acts
