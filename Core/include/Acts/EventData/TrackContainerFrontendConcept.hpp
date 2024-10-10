// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
