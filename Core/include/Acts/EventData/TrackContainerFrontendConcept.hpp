// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/TrackContainerBackendConcept.hpp"
#include "Acts/EventData/Types.hpp"

#include <concepts>

namespace Acts {

template <typename T>
concept TrackContainerFrontend = requires() {
  { T::ReadOnly } -> std::same_as<const bool &>;

  requires std::same_as<typename T::IndexType, TrackIndexType>;

  requires TrackContainerBackend<typename T::TrackContainerBackend>;
  requires CommonMultiTrajectoryBackend<typename T::TrackStateContainerBackend>;

  typename T::TrackProxy;
  typename T::ConstTrackProxy;
  typename T::TrackStateProxy;
  typename T::ConstTrackStateProxy;
};

}  // namespace Acts
