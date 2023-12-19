// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/AMVFInfo.hpp"
#include "Acts/Vertexing/IVertexFinder.hpp"
#include "Acts/Vertexing/ImpactPointEstimator.hpp"
#include "Acts/Vertexing/TrackLinearizer.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"

#include <type_traits>

namespace Acts {
/// @brief Implements an iterative vertex finder
///
////////////////////////////////////////////////////////////
///
/// Brief description of the algorithm implemented:
/// TODO
///
////////////////////////////////////////////////////////////
///
/// @tparam vfitter_t Vertex fitter type
/// @tparam sfinder_t Seed finder type
template <typename vfitter_t, typename sfinder_t>
class AdaptiveMultiVertexFinder final : public IVertexFinder {
  using FitterState_t = typename vfitter_t::State;
  using SeedFinderState_t = typename sfinder_t::State;

  template <typename T, typename = int>
  struct NeedsRemovedTracks : std::false_type {};

#ifndef DOXYGEN
  template <typename T>
  struct NeedsRemovedTracks<T, decltype((void)T::tracksToRemove, 0)>
      : std::true_type {};
#endif

 public:
  /// Configuration struct
  struct Config {
    /// @brief Config constructor
    ///
    /// @param fitter The vertex fitter
    /// @param sfinder The seed finder
    /// @param ipEst ImpactPointEstimator
    /// @param bIn Input magnetic field
    Config(vfitter_t fitter, sfinder_t sfinder, ImpactPointEstimator ipEst,
           std::shared_ptr<const MagneticFieldProvider> bIn)
        : vertexFitter(std::move(fitter)),
          seedFinder(std::move(sfinder)),
          ipEstimator(std::move(ipEst)),
          bField{std::move(bIn)} {}

    // Vertex fitter
    vfitter_t vertexFitter;

    // Vertex seed finder
    sfinder_t seedFinder;

    // ImpactPointEstimator
    ImpactPointEstimator ipEstimator;

    std::shared_ptr<const MagneticFieldProvider> bField;

    // Max z interval used for adding tracks to fit:
    // When adding a new vertex to the multi vertex fit,
    // only the tracks whose z at PCA is closer
    // to the seeded vertex than tracksMaxZinterval
    // are added to this new vertex.
    //
    // Note: If you cut too hard, you cut out
    // the good cases where the seed finder is not
    // reliable, but the fit would be still able to converge
    // towards the right vertex. If you cut too soft, you
    // consider a lot of tracks which just slow down the fit.
    double tracksMaxZinterval = 3. * Acts::UnitConstants::mm;

    // Maximum allowed significance of track position to vertex seed to consider
    // track as compatible to vertex. If useTime is set to true, the time
    // coordinate also contributes to the significance and tracksMaxSignificance
    // needs to be increased.
    double tracksMaxSignificance = 5.;

    // Max chi2 value for which tracks are considered compatible with
    // the fitted vertex. These tracks are removed from the seedTracks
    // after the fit has been performed.
    double maxVertexChi2 = 18.42;

    // Perform a 'real' multi-vertex fit as intended by the algorithm.
    // If switched to true, always all (!) tracks are considered to be
    // added to the new vertex candidate after seeding. If switched to
    // false, only the seedTracks, i.e. all tracks that are considered
    // as outliers of previously fitted vertices, are used.
    bool doRealMultiVertex = true;

    // Decides if you want to use the ```vertexCompatibility``` of the
    //  track (set to true) or the ```chi2Track``` (set to false) as an
    // estimate for a track being an outlier or not.
    // In case the track refitting is switched on in the AMVFitter, you
    // may want to use the refitted ```chi2Track```.
    bool useFastCompatibility = true;

    // Maximum significance on the distance between two vertices
    // to allow merging of two vertices.
    double maxMergeVertexSignificance = 3.;

    // Minimum weight a track has to have to be considered a compatible
    // track with a vertex candidate.
    //
    // Note: This value has to be the same as the one in the AMVFitter.
    double minWeight = 0.0001;

    // Maximal number of iterations in the finding procedure
    int maxIterations = 100;

    // Include also single track vertices
    bool addSingleTrackVertices = false;

    // Use 3d information for evaluating the vertex distance significance
    // for vertex merging/splitting
    bool do3dSplitting = false;

    // Maximum vertex contamination value
    double maximumVertexContamination = 0.5;

    // Use seed vertex as a constraint for the fit
    bool useSeedConstraint = true;

    // Variances of the 4D vertex position before the vertex fit if no beamspot
    // constraint is provided
    Vector4 initialVariances = Vector4::Constant(1e+8);

    // Default fitQuality for constraint vertex in case no beamspot
    // constraint is provided
    std::pair<double, double> defaultConstrFitQuality{0., -3.};

    // Use the full available vertex covariance information after
    // seeding for the IP estimation. In original implementation
    // this is not (!) done, however, this is probably not correct.
    // So definitely consider setting this to true.
    bool useVertexCovForIPEstimation = false;

    // Use time information when assigning tracks to vertices. If this is set to
    // true, useTime of the vertex fitter configuration should also be set to
    // true, and time seeding should be enabled.
    bool useTime = false;

    // Function to extract parameters from InputTrack
    InputTrack::Extractor extractParameters;
  };  // Config struct

  /// State struct for fulfilling interface
  struct State {};

  /// @brief Constructor for user-defined InputTrack_t type !=
  /// BoundTrackParameters
  ///
  /// @param cfg Configuration object
  /// @param logger The logging instance
  AdaptiveMultiVertexFinder(Config cfg,
                            std::unique_ptr<const Logger> logger =
                                getDefaultLogger("AdaptiveMultiVertexFinder",
                                                 Logging::INFO))
      : m_cfg(std::move(cfg)), m_logger(std::move(logger)) {
    if (!m_cfg.extractParameters.connected()) {
      throw std::invalid_argument(
          "AdaptiveMultiVertexFinder: "
          "No function to extract parameters "
          "from InputTrack provided.");
    }

    if (!m_cfg.seedFinder.hasTrivialState()) {
      throw std::invalid_argument(
          "AdaptiveMultiVertexFinder: "
          "Seed finder state must be trivial.");
    }
  }

  AdaptiveMultiVertexFinder(AdaptiveMultiVertexFinder&&) = default;

  /// @brief Function that performs the adaptive
  /// multi-vertex finding
  ///
  /// @param allTracks Input track collection
  /// @param vertexingOptions Vertexing options
  /// @param state State for fulfilling interfaces
  ///
  /// @return Vector of all reconstructed vertices
  Result<std::vector<Vertex>> find(const std::vector<InputTrack>& allTracks,
                                   const VertexingOptions& vertexingOptions,
                                   IVertexFinder::State& state) const override;

  IVertexFinder::State makeState() const override {
    return IVertexFinder::State{State{}};
  }

  bool hasTrivialState() const override {
    return true;
  }

 private:
  /// Configuration object
  Config m_cfg;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to logging instance
  const Logger& logger() const {
    return *m_logger;
  }

  /// @brief Calls the seed finder and sets constraints on the found seed
  /// vertex if desired
  ///
  /// @param trackVector All tracks to be used for seeding
  /// @param currentConstraint Vertex constraint
  /// @param vertexingOptions Vertexing options
  /// @param seedFinderState The seed finder state
  /// @param removedSeedTracks Seed track that have been removed
  /// from seed track collection in last iteration
  ///
  /// @return The seed vertex
  Result<Vertex> doSeeding(
      const std::vector<InputTrack>& trackVector, Vertex& currentConstraint,
      const VertexingOptions& vertexingOptions,
      IVertexFinder::State& seedFinderState,
      const std::vector<InputTrack>& removedSeedTracks) const;

  /// @brief Sets constraint vertex after seeding
  ///
  /// @param currentConstraint Vertex constraint
  /// @param useVertexConstraintInFit Indicates whether constraint is used during vertex fit
  /// @param seedVertex Seed vertex
  void setConstraintAfterSeeding(Vertex& currentConstraint,
                                 bool useVertexConstraintInFit,
                                 Vertex& seedVertex) const;

  /// @brief Calculates the IP significance of a track to a given vertex
  ///
  /// @param track The track
  /// @param vtx The vertex
  /// @param vertexingOptions Vertexing options
  ///
  /// @return The IP significance
  Result<double> getIPSignificance(
      const InputTrack& track, const Vertex& vtx,
      const VertexingOptions& vertexingOptions) const;

  /// @brief Adds compatible track to vertex candidate
  ///
  /// @param tracks The tracks
  /// @param vtx The vertex candidate
  /// @param[out] fitterState The vertex fitter state
  /// @param vertexingOptions Vertexing options
  Result<void> addCompatibleTracksToVertex(
      const std::vector<InputTrack>& tracks, Vertex& vtx,
      FitterState_t& fitterState,
      const VertexingOptions& vertexingOptions) const;

  /// @brief Method that tries to recover from cases where no tracks
  /// were added to the vertex candidate after seeding
  ///
  /// @param allTracks The tracks to be considered (either origTrack or
  /// seedTracks)
  /// @param seedTracks The seed tracks
  /// @param[out] vtx The vertex candidate
  /// @param currentConstraint Vertex constraint
  /// @param[out] fitterState The vertex fitter state
  /// @param vertexingOptions Vertexing options
  ///
  /// return True if recovery was successful, false otherwise
  Result<bool> canRecoverFromNoCompatibleTracks(
      const std::vector<InputTrack>& allTracks,
      const std::vector<InputTrack>& seedTracks, Vertex& vtx,
      const Vertex& currentConstraint, FitterState_t& fitterState,
      const VertexingOptions& vertexingOptions) const;

  /// @brief Method that tries to prepare the vertex for the fit
  ///
  /// @param allTracks The tracks to be considered (either origTrack or
  /// seedTracks)
  /// @param seedTracks The seed tracks
  /// @param[out] vtx The vertex candidate
  /// @param currentConstraint Vertex constraint
  /// @param[out] fitterState The vertex fitter state
  /// @param vertexingOptions Vertexing options
  ///
  /// @return True if preparation was successful, false otherwise
  Result<bool> canPrepareVertexForFit(
      const std::vector<InputTrack>& allTracks,
      const std::vector<InputTrack>& seedTracks, Vertex& vtx,
      const Vertex& currentConstraint, FitterState_t& fitterState,
      const VertexingOptions& vertexingOptions) const;

  /// @brief Method that checks if vertex is a good vertex and if
  /// compatible tracks are available
  ///
  /// @param vtx The vertex candidate
  /// @param seedTracks The seed tracks
  /// @param fitterState The vertex fitter state
  /// @param useVertexConstraintInFit Indicates whether constraint is used in the vertex fit
  ///
  /// @return pair(nCompatibleTracks, isGoodVertex)
  std::pair<int, bool> checkVertexAndCompatibleTracks(
      Vertex& vtx, const std::vector<InputTrack>& seedTracks,
      FitterState_t& fitterState, bool useVertexConstraintInFit) const;

  /// @brief Method that removes all tracks that are compatible with
  /// current vertex from seedTracks
  ///
  /// @param vtx The vertex candidate
  /// @param[out] seedTracks The seed tracks
  /// @param fitterState The vertex fitter state
  /// @param[out] removedSeedTracks Collection of seed track that will be
  /// removed
  void removeCompatibleTracksFromSeedTracks(
      Vertex& vtx, std::vector<InputTrack>& seedTracks,
      FitterState_t& fitterState,
      std::vector<InputTrack>& removedSeedTracks) const;

  /// @brief Method that tries to remove an incompatible track
  /// from seed tracks after removing a compatible track failed.
  ///
  /// @param vtx The vertex candidate
  /// @param[out] seedTracks The seed tracks
  /// @param fitterState The vertex fitter state
  /// @param[out] removedSeedTracks Collection of seed track that will be
  /// removed
  /// @param[in] geoCtx The geometry context to access global positions
  ///
  /// @return Incompatible track was removed
  bool removeTrackIfIncompatible(Vertex& vtx,
                                 std::vector<InputTrack>& seedTracks,
                                 FitterState_t& fitterState,
                                 std::vector<InputTrack>& removedSeedTracks,
                                 const GeometryContext& geoCtx) const;

  /// @brief Method that evaluates if the new vertex candidate should
  /// be kept, i.e. saved, or not
  ///
  /// @param vtx The vertex candidate
  /// @param allVertices All so far found vertices
  /// @param fitterState The vertex fitter state
  ///
  /// @return Keep new vertex
  bool keepNewVertex(Vertex& vtx, const std::vector<Vertex*>& allVertices,
                     FitterState_t& fitterState) const;

  /// @brief Method that evaluates if the new vertex candidate is
  /// merged with one of the previously found vertices
  ///
  /// @param vtx The vertex candidate
  /// @param allVertices All so far found vertices
  ///
  /// @return Vertex is merged
  bool isMergedVertex(const Vertex& vtx,
                      const std::vector<Vertex*>& allVertices) const;

  /// @brief Method that deletes last vertex from list of all vertices
  /// and refits all vertices afterwards
  ///
  /// @param vtx The last added vertex which will be removed
  /// @param allVertices Vector containing the unique_ptr to vertices
  /// @param allVerticesPtr Vector containing the actual addresses
  /// @param fitterState The current vertex fitter state
  /// @param vertexingOptions Vertexing options
  Result<void> deleteLastVertex(
      Vertex& vtx, std::vector<std::unique_ptr<Vertex>>& allVertices,
      std::vector<Vertex*>& allVerticesPtr, FitterState_t& fitterState,
      const VertexingOptions& vertexingOptions) const;

  /// @brief Prepares the output vector of vertices
  ///
  /// @param allVerticesPtr Vector of pointers to vertices
  /// @param fitterState The vertex fitter state
  ///
  /// @return The output vertex collection
  Result<std::vector<Vertex>> getVertexOutputList(
      const std::vector<Vertex*>& allVerticesPtr,
      FitterState_t& fitterState) const;
};

}  // namespace Acts

#include "Acts/Vertexing/AdaptiveMultiVertexFinder.ipp"
