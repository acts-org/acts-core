// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ExaTrkX/CugraphTrackBuilding.hpp"

#include <map>

#include <torch/script.h>

#include "weaklyConnectedComponentsCugraph.hpp"

namespace Acts {

std::vector<std::vector<int>> CugraphTrackBuilding::operator()(
    std::any, std::any edges, std::any edge_weights,
    std::vector<int> &spacepointIDs, torch::Device) {
  auto numSpacepoints = spacepointIDs.size();
  auto edgesAfterFiltering = std::any_cast<std::vector<std::int64_t>>(edges);
  auto numEdgesAfterF = edgesAfterFiltering.size() / 2;
  auto gOutputCTen = std::any_cast<at::Tensor>(edge_weights);

  if (numEdgesAfterF == 0) {
    return {};
  }

  // ************
  // Track Labeling with cugraph::connected_components
  // ************
  std::vector<std::int32_t> rowIndices;
  std::vector<std::int32_t> colIndices;
  std::vector<float> edgeWeights;
  std::vector<std::int32_t> trackLabels(numSpacepoints);
  std::copy(edgesAfterFiltering.begin(),
            edgesAfterFiltering.begin() + numEdgesAfterF,
            std::back_insert_iterator(rowIndices));
  std::copy(edgesAfterFiltering.begin() + numEdgesAfterF,
            edgesAfterFiltering.end(), std::back_insert_iterator(colIndices));
  std::copy(gOutputCTen.data_ptr<float>(),
            gOutputCTen.data_ptr<float>() + numEdgesAfterF,
            std::back_insert_iterator(edgeWeights));

  ACTS_VERBOSE("run weaklyConnectedComponents");
  weaklyConnectedComponents<std::int32_t, std::int32_t, float>(
      rowIndices, colIndices, edgeWeights, trackLabels, logger());

  ACTS_DEBUG("size of components: " << trackLabels.size());
  if (trackLabels.size() == 0) {
    return {};
  }

  std::vector<std::vector<int>> trackCandidates;
  trackCandidates.clear();

  int existTrkIdx = 0;
  // map labeling from MCC to customized track id.
  std::map<int, int> trackLableToIds;

  for (auto idx = 0ul; idx < numSpacepoints; ++idx) {
    int trackLabel = trackLabels[idx];
    int spacepointID = spacepointIDs[idx];

    int trkId;
    if (trackLableToIds.contains(trackLabel)) {
      trkId = trackLableToIds[trackLabel];
      trackCandidates[trkId].push_back(spacepointID);
    } else {
      // a new track, assign the track id
      // and create a vector
      trkId = existTrkIdx;
      trackCandidates.push_back(std::vector<int>{trkId});
      trackLableToIds[trackLabel] = trkId;
      existTrkIdx++;
    }
  }

  return trackCandidates;
}

}  // namespace Acts
