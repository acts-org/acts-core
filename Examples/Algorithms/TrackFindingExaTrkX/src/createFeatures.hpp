// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/TrackFindingExaTrkX/TrackFindingAlgorithmExaTrkX.hpp"

namespace ActsExamples {

std::vector<float> createFeatures(
    const SimSpacePointContainer &spacepoints,
    const std::optional<ClusterContainer> &clusters,
    const std::vector<TrackFindingAlgorithmExaTrkX::NodeFeature> &nodeFeatures,
    const std::vector<float> &featureScales);

}
