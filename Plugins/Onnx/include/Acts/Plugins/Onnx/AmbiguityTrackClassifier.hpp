// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/MultiTrajectoryHelpers.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackContainerFrontendConcept.hpp"
#include "Acts/EventData/TrackProxyConcept.hpp"
#include "Acts/Plugins/Onnx/OnnxRuntimeBase.hpp"
#include "Acts/TrackFinding/detail/AmbiguityTrackClustering.hpp"

#include <map>
#include <unordered_map>
#include <vector>

#include <onnxruntime_cxx_api.h>

namespace Acts {

/// Onnx model implementation for track scoring and selection
class AmbiguityTrackClassifier {
 public:
  /// Construct the ambiguity scoring algorithm.
  ///
  /// @param modelPath path to the model file
  AmbiguityTrackClassifier(const char* modelPath)
      : m_env(ORT_LOGGING_LEVEL_WARNING, "MLClassifier"),
        m_duplicateClassifier(m_env, modelPath) {}

  /// Compute a score for each track to be used in the track selection
  ///
  /// @param clusters is a map of clusters, each cluster correspond to a vector of track ID
  /// @param tracks is the input track container
  /// @return a vector of vector of track score. Due to the architecture of the network each track only have a size 1 score vector.
  template <TrackContainerFrontend track_container_t>
  std::vector<std::vector<float>> inferScores(
      std::unordered_map<std::size_t, std::vector<std::size_t>>& clusters,
      const track_container_t& tracks) const {
    // Compute the number of entry (since it is smaller than the number of
    // track)
    int trackNb = 0;
    for (const auto& [_, val] : clusters) {
      trackNb += val.size();
    }
    // Input of the neural network
    Acts::NetworkBatchInput networkInput(trackNb, 8);
    std::size_t inputID = 0;
    // Get the input feature of the network for all the tracks
    for (const auto& [key, val] : clusters) {
      for (const auto& trackID : val) {
        auto track = tracks.getTrack(trackID);
        auto trajState = Acts::MultiTrajectoryHelpers::trajectoryState(
            tracks.trackStateContainer(), track.tipIndex());
        networkInput(inputID, 0) = trajState.nStates;
        networkInput(inputID, 1) = trajState.nMeasurements;
        networkInput(inputID, 2) = trajState.nOutliers;
        networkInput(inputID, 3) = trajState.nHoles;
        networkInput(inputID, 4) = trajState.NDF;
        networkInput(inputID, 5) = (trajState.chi2Sum * 1.0) /
                                   (trajState.NDF != 0 ? trajState.NDF : 1);
        networkInput(inputID, 6) = Acts::VectorHelpers::eta(track.momentum());
        networkInput(inputID, 7) = Acts::VectorHelpers::phi(track.momentum());
        inputID++;
      }
    }
    // Use the network to compute a score for all the tracks.
    std::vector<std::vector<float>> outputTensor =
        m_duplicateClassifier.runONNXInference(networkInput);
    return outputTensor;
  }

  /// Select the track associated with each cluster based on the score vector
  ///
  /// @param clusters is a map of clusters, each cluster correspond to a vector of track ID
  /// @param outputTensor is the score vector obtained from inferScores.
  /// @return a vector of trackID corresponding tho the good tracks
  std::vector<std::size_t> trackSelection(
      std::unordered_map<std::size_t, std::vector<std::size_t>>& clusters,
      std::vector<std::vector<float>>& outputTensor) const {
    std::vector<std::size_t> goodTracks;
    std::size_t iOut = 0;
    // Loop over all the cluster and only keep the track with the highest score
    // in each cluster
    for (const auto& [key, val] : clusters) {
      std::size_t bestTrackID = 0;
      float bestTrackScore = 0;
      for (const auto& track : val) {
        if (outputTensor[iOut][0] > bestTrackScore) {
          bestTrackScore = outputTensor[iOut][0];
          bestTrackID = track;
        }
        iOut++;
      }
      goodTracks.push_back(bestTrackID);
    }
    return goodTracks;
  }

  /// Select the track associated with each cluster
  ///
  /// @param clusters is a map of clusters, each cluster correspond to a vector of track ID
  /// @param tracks is the input track container
  /// @return a vector of trackID corresponding tho the good tracks
  template <TrackContainerFrontend track_container_t>
  std::vector<std::size_t> solveAmbiguity(
      std::unordered_map<std::size_t, std::vector<std::size_t>>& clusters,
      const track_container_t& tracks) const {
    std::vector<std::vector<float>> outputTensor =
        inferScores(clusters, tracks);
    std::vector<std::size_t> goodTracks =
        trackSelection(clusters, outputTensor);

    return goodTracks;
  }

 private:
  // ONNX environment
  Ort::Env m_env;
  // ONNX model for the duplicate neural network
  Acts::OnnxRuntimeBase m_duplicateClassifier;
};

}  // namespace Acts
