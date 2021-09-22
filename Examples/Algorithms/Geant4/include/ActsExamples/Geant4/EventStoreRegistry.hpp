// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Material/MaterialInteraction.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"

#include <map>
#include <vector>

namespace ActsExamples {

class WhiteBoard;

/// A registry for event data and the event store (per event)
///
/// The access is static, however, there is an individual instance
/// per event and hence the retrival/writing is parallel event/save
///
/// @note multiple threads within an event could lead to conflicts
class EventStoreRegistry {
 public:
  /// Nested containers struct to give access to the
  /// shared event data.
  struct State {
    /// The current event store
    WhiteBoard* store = nullptr;
    /// Initial and final particle collections
    SimParticleContainer::sequence_type particlesInitial;
    SimParticleContainer::sequence_type particlesFinal;
    /// The hits in sensitive detectors
    SimHitContainer::sequence_type hits;
    /// Tracks recorded in material mapping
    std::vector<Acts::RecordedMaterialTrack> materialTracks;
  };

  EventStoreRegistry() = delete;

  static State& eventData();
};

}  // namespace ActsExamples
