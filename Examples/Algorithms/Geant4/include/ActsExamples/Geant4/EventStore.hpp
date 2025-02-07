// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Material/MaterialInteraction.hpp"
#include "ActsExamples/EventData/PropagationSummary.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsFatras/EventData/ParticleOutcome.hpp"

#include <set>
#include <unordered_map>
#include <vector>

#include "G4Types.hh"

namespace ActsExamples {
class WhiteBoard;
}

namespace ActsExamples::Geant4 {

/// Common event store for all Geant4 related sub algorithms
struct EventStore {
 public:
  /// The current event store
  WhiteBoard* store = nullptr;

  /// Use a std::set here because it allows for fast insertion and ensures
  /// uniqueness. Thus particle collisions are detected early.
  using ParticleContainer =
      std::set<SimParticle, ActsExamples::detail::CompareParticleId>;

  /// Initial particle collection
  ParticleContainer particlesInitial;

  /// Simulated particle collection
  ParticleContainer particlesSimulated;

  /// The hits in sensitive detectors
  SimHitContainer::sequence_type hits;

  /// Hit buffer for step merging (multiple steps in sensitive volume)
  std::vector<ActsFatras::Hit> hitBuffer;

  /// Some statistics for the step merging
  std::size_t numberGeantSteps = 0;
  std::size_t maxStepsForHit = 0;

  /// Tracks recorded in material mapping
  std::unordered_map<std::size_t, Acts::RecordedMaterialTrack> materialTracks;

  /// Stepping records for step plotting
  std::unordered_map<G4int, PropagationSummary> propagationRecords;

  /// Particle hit count (for hit indexing)
  std::unordered_map<SimBarcode, std::size_t> particleHitCount;
  /// Particle status
  std::unordered_map<SimBarcode, ActsFatras::ParticleOutcome> particleOutcome;
  /// Geant4 Track ID to Barcode mapping
  std::unordered_map<G4int, SimBarcode> trackIdMapping;
  /// Geant4 Track ID subparticle counter (for subparticle indexing)
  std::unordered_map<G4int, std::size_t> trackIdSubparticleCount;

  /// Data handles to read particles from the whiteboard
  const ReadDataHandle<SimParticleContainer>* inputParticles{nullptr};

  /// Count particle ID collisions
  std::size_t particleIdCollisionsInitial = 0;
  std::size_t particleIdCollisionsFinal = 0;
  std::size_t parentIdNotFound = 0;

  /// Store subparticle count for {primVertex, secVertex, part, gen}
  /// This is done using a pseudo-barcode that contains all fields but not the
  /// subparticle counter. This can be used as key in a map to store the
  /// subparticle information
  using BarcodeWithoutSubparticle =
      Acts::MultiIndex<std::uint64_t, 16, 16, 16, 16>;
  std::unordered_map<BarcodeWithoutSubparticle, std::size_t> subparticleMap;
};

}  // namespace ActsExamples::Geant4
