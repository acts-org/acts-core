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
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Utilities/AnnealingUtility.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/AMVFInfo.hpp"
#include "Acts/Vertexing/ImpactPointEstimator.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "Acts/Vertexing/TrackLinearizer.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexingError.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"

#include <algorithm>
#include <functional>

namespace Acts {

/// @class AdaptiveMultiVertexFitter
/// @brief Implements an adaptive multi-vertex fitter as described
///   in detail in Section 5.3.5 in:
///   Ref. (1): CERN-THESIS-2010-027, Author: Piacquadio, Giacinto:
///   `Identification of b-jets and investigation of the discovery potential
///   of a Higgs boson in the WH−−>lvbb¯ channel with the ATLAS experiment`
///
class AdaptiveMultiVertexFitter {
 public:
  /// @brief The fitter state
  struct State {
    State(const MagneticFieldProvider& field,
          const Acts::MagneticFieldContext& magContext)
        : ipState{field.makeCache(magContext)},
          fieldCache(field.makeCache(magContext)) {}

    // Vertex collection to be fitted
    std::vector<Vertex*> vertexCollection;

    // Annealing state
    AnnealingUtility::State annealingState;

    ImpactPointEstimator::State ipState;

    MagneticFieldProvider::Cache fieldCache;

    // Map to store vertices information
    // @TODO Does this have to be a mutable pointer?
    std::map<Vertex*, VertexInfo> vtxInfoMap;

    std::multimap<InputTrack, Vertex*> trackToVerticesMultiMap;

    std::map<std::pair<InputTrack, Vertex*>, TrackAtVertex> tracksAtVerticesMap;

    // Adds a vertex to trackToVerticesMultiMap
    void addVertexToMultiMap(Vertex& vtx) {
      for (auto trk : vtxInfoMap[&vtx].trackLinks) {
        trackToVerticesMultiMap.emplace(trk, &vtx);
      }
    }

    // Removes a vertex from trackToVerticesMultiMap
    void removeVertexFromMultiMap(Vertex& vtx) {
      for (auto iter = trackToVerticesMultiMap.begin();
           iter != trackToVerticesMultiMap.end();) {
        if (iter->second == &vtx) {
          iter = trackToVerticesMultiMap.erase(iter);
        } else {
          ++iter;
        }
      }
    }

    Result<void> removeVertexFromCollection(Vertex& vtxToRemove,
                                            const Logger& logger) {
      auto it = std::ranges::find(vertexCollection, &vtxToRemove);
      // Check if the value was found before erasing
      if (it == vertexCollection.end()) {
        ACTS_ERROR("vtxToRemove is not part of vertexCollection.");
        return VertexingError::ElementNotFound;
      }
      // Erase the element if found
      vertexCollection.erase(it);
      return {};
    }
  };

  struct Config {
    /// @brief Config constructor
    ///
    /// @param est ImpactPointEstimator
    Config(ImpactPointEstimator est) : ipEst(std::move(est)) {}

    // ImpactPointEstimator
    ImpactPointEstimator ipEst;

    /// Annealing tool used for a thermodynamic annealing scheme for the
    /// track weight factors in such a way that with high temperature values
    /// (at the beginning) only a slight preference is given to tracks
    /// compatible with the estimated vertex position. With lower temperatures
    /// the weighting get stricter such that all incompatible tracks will be
    /// dropped at the end while keeping all compatible tracks with a weight=1.
    ///   Ref. (1): CERN-THESIS-2010-027, Author: Piacquadio, Giacinto:
    ///   `Identification of b-jets and investigation of the discovery potential
    ///   of a Higgs boson in the WH−−>lvbb¯ channel with the ATLAS experiment`
    AnnealingUtility annealingTool;

    // Number of max iterations
    unsigned int maxIterations{30};

    // Max distance to linearization point allowed
    // without relinearization
    double maxDistToLinPoint{0.5};

    // Minimum track weight needed for track to be considered
    double minWeight{0.0001};

    // Max relative shift of vertex during one iteration
    double maxRelativeShift{0.01};

    // Do smoothing after multivertex fit
    bool doSmoothing{false};

    // Use time information when calculating the vertex compatibility
    bool useTime{false};

    // Function to extract parameters from InputTrack
    InputTrack::Extractor extractParameters;

    TrackLinearizer trackLinearizer;
  };

  /// @brief Constructor for user-defined InputTrack_t type !=
  /// BoundTrackParameters
  ///
  /// @param cfg Configuration object
  /// object
  /// @param logger The logging instance
  AdaptiveMultiVertexFitter(Config cfg,
                            std::unique_ptr<const Logger> logger =
                                getDefaultLogger("AdaptiveMultiVertexFitter",
                                                 Logging::INFO))
      : m_cfg(std::move(cfg)), m_logger(std::move(logger)) {
    if (!m_cfg.extractParameters.connected()) {
      throw std::invalid_argument(
          "AdaptiveMultiVertexFitter: No function to extract parameters "
          "from InputTrack_t provided.");
    }

    if (!m_cfg.trackLinearizer.connected()) {
      throw std::invalid_argument(
          "AdaptiveMultiVertexFitter: No track linearizer provided.");
    }
  }

  /// @brief Adds a new vertex to an existing multi-vertex fit.
  /// 1. The 3D impact parameters are calculated for all tracks associated
  /// with newVertex.
  /// 2. A list of all vertices that are connected with newVertex via shared
  /// tracks is created. It also considers indirect connections (e.g., vertex A
  /// which shares a track with vertex B which, in turn, shares a track with
  /// newVertex).
  /// 3. The multivertex fit is performed for all vertices on said list.
  ///
  /// @param state Fitter state
  /// @param newVertex Vertex to be added to fit
  /// @param vertexingOptions Vertexing options
  ///
  /// @return Result<void> object
  Result<void> addVtxToFit(State& state, Vertex& newVertex,
                           const VertexingOptions& vertexingOptions) const;

  /// @brief Performs a simultaneous fit of all vertices in
  /// state.vertexCollection
  ///
  /// @param state Fitter state
  /// @param vertexingOptions Vertexing options
  ///
  /// @return Result<void> object
  Result<void> fit(State& state,
                   const VertexingOptions& vertexingOptions) const;

 private:
  /// Configuration object
  const Config m_cfg;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to logging instance
  const Logger& logger() const { return *m_logger; }

  /// @brief Tests if vertex is already in list of vertices or not
  ///
  /// @param vtx Vertex to test
  /// @param verticesVec Vector of vertices to search
  ///
  /// @return True if vtx is already in verticesVec
  bool isAlreadyInList(Vertex* vtx,
                       const std::vector<Vertex*>& verticesVec) const;

  /// @brief 1) Calls ImpactPointEstimator::estimate3DImpactParameters
  /// for all tracks that are associated with vtx (i.e., all elements
  /// of the trackLinks vector in the VertexInfo of vtx).
  /// 2) Saves the 3D impact parameters in the VertexInfo of vtx.
  ///
  /// @param state Vertex fitter state
  /// @param vtx Vertex object
  /// @param vertexingOptions Vertexing options
  Result<void> prepareVertexForFit(
      State& state, Vertex* vtx,
      const VertexingOptions& vertexingOptions) const;

  /// @brief Sets the vertexCompatibility for all TrackAtVertex objects
  /// at the current vertex
  ///
  /// @param state Fitter state
  /// @param currentVtx Current vertex
  /// @param vertexingOptions Vertexing options
  Result<void> setAllVertexCompatibilities(
      State& state, Vertex* currentVtx,
      const VertexingOptions& vertexingOptions) const;

  /// @brief Sets weights to the track according to Eq.(5.46) in Ref.(1)
  ///  and updates the vertices by calling the VertexUpdater
  ///
  /// @param state Fitter state
  /// @param vertexingOptions Vertexing options
  Result<void> setWeightsAndUpdate(
      State& state, const VertexingOptions& vertexingOptions) const;

  /// @brief Collects the compatibility values of the track `trk`
  /// wrt to all of its associated vertices
  ///
  /// @param state Fitter state
  /// @param trk Track
  ///
  /// @return Vector of compatibility values
  std::vector<double> collectTrackToVertexCompatibilities(
      State& state, const InputTrack& trk) const;

  /// @brief Determines if any vertex position has shifted more than
  /// m_cfg.maxRelativeShift in the last iteration
  ///
  /// @param state Fitter state
  ///
  /// @return False if at least one shift was larger than maxRelativeShift
  bool checkSmallShift(State& state) const;

  /// @brief Updates tracks for current vertex with knowledge
  /// of current vertex position
  ///
  /// @param state Fitter state
  void doVertexSmoothing(State& state) const;

  /// @brief Logs vertices in state.vertexCollection and associated tracks
  ///
  /// @param state Fitter state
  /// @param geoContext Geometry context
  void logDebugData(const State& state,
                    const GeometryContext& geoContext) const;
};

}  // namespace Acts
