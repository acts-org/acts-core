// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/MultiTrajectoryBackendConcept.hpp"
#include "Acts/EventData/TrackContainerBackendConcept.hpp"
#include "Acts/EventData/Types.hpp"

#include <concepts>

namespace Acts {

template <typename T>
concept TrackProxyConcept = requires() {
  { T::ReadOnly } -> std::same_as<const bool &>;

  requires TrackContainerBackend<typename T::Container>;

  requires CommonMultiTrajectoryBackend<typename T::Trajectory>;

  requires std::same_as<typename T::IndexType, TrackIndexType>;
};

}  // namespace Acts
