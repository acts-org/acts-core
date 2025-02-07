// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackContainerFrontendConcept.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/Utilities/Concepts.hpp"

namespace Acts {

/// @brief Concept for the ambiguity network used in the ambiguity resolution
///
/// The ambiguity network correspond to the AmbiguityTrackClassifier found in
/// the Onnx plugin. It is used to score the tracks and select the best ones.
///
/// The constructor of the Ambiguity Solver network should take string as input
/// corresponding to the path of the ONNX model.
/// The implementation of the Ambiguity Solver network should have two methods:
/// - inferScores: takes clusters (a list of track ID associated with a cluster
/// ID) and the track container and return an outputTensor (list of scores for
///                each track in the clusters).
/// - trackSelection: Takes clusters and the output tensor from the inferScores
///                   method and return the list of track ID to keep.
///
/// @tparam N the type of the network
template <typename network_t>
concept AmbiguityNetworkConcept = requires(
    TrackContainer<VectorTrackContainer, VectorMultiTrajectory,
                   detail::ValueHolder> &tracks,
    std::unordered_map<std::size_t, std::vector<std::size_t>> &clusters,
    std::vector<std::vector<float>> &outputTensor, const char *modelPath,
    network_t &n) {
  { network_t(modelPath) } -> std::same_as<network_t>;

  {
    n.inferScores(clusters, tracks)
  } -> std::same_as<std::vector<std::vector<float>>>;
  {
    n.trackSelection(clusters, outputTensor)
  } -> std::same_as<std::vector<std::size_t>>;
};

}  // namespace Acts
