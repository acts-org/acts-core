// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
