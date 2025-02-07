// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"

#include <vector>

namespace ActsExamples {

/// (Reconstructed) track parameters e.g. close to the vertex.
using TrackParameters = ::Acts::BoundTrackParameters;
/// Container of reconstructed track states for multiple tracks.
using TrackParametersContainer = std::vector<TrackParameters>;

using TrackContainer =
    Acts::TrackContainer<Acts::VectorTrackContainer,
                         Acts::VectorMultiTrajectory, std::shared_ptr>;

using ConstTrackContainer =
    Acts::TrackContainer<Acts::ConstVectorTrackContainer,
                         Acts::ConstVectorMultiTrajectory, std::shared_ptr>;

using TrackIndexType = TrackContainer::IndexType;

using TrackProxy = TrackContainer::TrackProxy;
using ConstTrackProxy = ConstTrackContainer::ConstTrackProxy;

using TrackStateProxy = TrackContainer::TrackStateProxy;
using ConstTrackStateProxy = ConstTrackContainer::ConstTrackStateProxy;

}  // namespace ActsExamples
