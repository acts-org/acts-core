// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/SensitiveSteppingAction.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Propagator/detail/SteppingLogger.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/MultiIndex.hpp"
#include "ActsExamples/Geant4/EventStore.hpp"
#include "ActsExamples/Geant4/SensitiveSurfaceMapper.hpp"
#include "ActsFatras/EventData/Barcode.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>

#include <G4RunManager.hh>
#include <G4Step.hh>
#include <G4StepPoint.hh>
#include <G4Track.hh>
#include <G4UnitsTable.hh>
#include <G4VPhysicalVolume.hh>
#include <G4VTouchable.hh>
#include <boost/version.hpp>

#if BOOST_VERSION >= 107800
#include <boost/describe.hpp>

BOOST_DESCRIBE_ENUM(G4StepStatus, fWorldBoundary, fGeomBoundary,
                    fAtRestDoItProc, fAlongStepDoItProc, fPostStepDoItProc,
                    fUserDefinedLimit, fExclusivelyForcedProc, fUndefined);

BOOST_DESCRIBE_ENUM(G4ProcessType, fNotDefined, fTransportation,
                    fElectromagnetic, fOptical, fHadronic, fPhotolepton_hadron,
                    fDecay, fGeneral, fParameterisation, fUserDefined,
                    fParallel, fPhonon, fUCN);

BOOST_DESCRIBE_ENUM(G4TrackStatus, fAlive, fStopButAlive, fStopAndKill,
                    fKillTrackAndSecondaries, fSuspend, fPostponeToNextEvent);
#endif

namespace {

std::array<Acts::Vector4, 4u> kinematicsOfStep(const G4Step* step) {
  static constexpr double convertLength = Acts::UnitConstants::mm / CLHEP::mm;
  static constexpr double convertEnergy = Acts::UnitConstants::GeV / CLHEP::GeV;
  static constexpr double convertTime = Acts::UnitConstants::ns / CLHEP::ns;

  const G4StepPoint* preStepPoint = step->GetPreStepPoint();
  const G4StepPoint* postStepPoint = step->GetPostStepPoint();

  Acts::Vector4 preStepPosition(convertLength * preStepPoint->GetPosition().x(),
                                convertLength * preStepPoint->GetPosition().y(),
                                convertLength * preStepPoint->GetPosition().z(),
                                convertTime * preStepPoint->GetGlobalTime());
  Acts::Vector4 preStepMomentum(convertEnergy * preStepPoint->GetMomentum().x(),
                                convertEnergy * preStepPoint->GetMomentum().y(),
                                convertEnergy * preStepPoint->GetMomentum().z(),
                                convertEnergy * preStepPoint->GetTotalEnergy());
  Acts::Vector4 postStepPosition(
      convertLength * postStepPoint->GetPosition().x(),
      convertLength * postStepPoint->GetPosition().y(),
      convertLength * postStepPoint->GetPosition().z(),
      convertTime * postStepPoint->GetGlobalTime());
  Acts::Vector4 postStepMomentum(
      convertEnergy * postStepPoint->GetMomentum().x(),
      convertEnergy * postStepPoint->GetMomentum().y(),
      convertEnergy * postStepPoint->GetMomentum().z(),
      convertEnergy * postStepPoint->GetTotalEnergy());

  return {preStepPosition, preStepMomentum, postStepPosition, postStepMomentum};
}

ActsFatras::Hit hitFromStep(const G4Step* step, ActsFatras::Barcode particleId,
                            Acts::GeometryIdentifier geoId,
                            std::int32_t index) {
  auto [preStepPosition, preStepMomentum, postStepPosition, postStepMomentum] =
      kinematicsOfStep(step);

  return ActsFatras::Hit(geoId, particleId,
                         0.5 * (preStepPosition + postStepPosition),
                         preStepMomentum, postStepMomentum, index);
}

Acts::detail::Step stepFromG4Step(const G4Step* step) {
  Acts::detail::Step pStep;
  auto [preStepPosition, preStepMomentum, postStepPosition, postStepMomentum] =
      kinematicsOfStep(step);

  pStep.navDir = Acts::Direction::Forward();
  pStep.position = 0.5 * (preStepPosition + postStepPosition).block<3, 1>(0, 0);
  pStep.momentum = 0.5 * (preStepMomentum + postStepMomentum).block<3, 1>(0, 0);
  pStep.nTotalTrials = 1;
  return pStep;
}

}  // namespace

namespace ActsExamples::Geant4 {

SensitiveSteppingAction::SensitiveSteppingAction(
    const Config& cfg, std::unique_ptr<const Acts::Logger> logger)
    : G4UserSteppingAction(), m_cfg(cfg), m_logger(std::move(logger)) {}

void SensitiveSteppingAction::UserSteppingAction(const G4Step* step) {
  // Unit conversions G4->::ACTS
  static constexpr double convertLength = Acts::UnitConstants::mm / CLHEP::mm;
  static constexpr double convertEnergy = Acts::UnitConstants::GeV / CLHEP::GeV;

  // The particle after the step
  G4Track* track = step->GetTrack();
  G4PrimaryParticle* primaryParticle =
      track->GetDynamicParticle()->GetPrimaryParticle();

  // Get PreStepPoint and PostStepPoint
  const G4StepPoint* preStepPoint = step->GetPreStepPoint();
  const G4StepPoint* postStepPoint = step->GetPostStepPoint();

  // Bail out if charged & configured to do so
  G4double absCharge = std::abs(track->GetParticleDefinition()->GetPDGCharge());
  if (!m_cfg.charged && absCharge > 0.) {
    return;
  }

  // Bail out if neutral & configured to do so
  if (!m_cfg.neutral && absCharge == 0.) {
    return;
  }

  // Bail out if it is a primary & configured to be ignored
  if (!m_cfg.primary && primaryParticle != nullptr) {
    return;
  }

  // Bail out if it is a secondary & configured to be ignored
  if (!m_cfg.secondary && primaryParticle == nullptr) {
    return;
  }

  // Get the physical volume & check if it has the sensitive string name
  const G4VPhysicalVolume* volume = track->GetVolume();
  if (volume == nullptr) {
    throw std::runtime_error("No volume found, terminate simulation");
  }
  std::string volumeName = volume->GetName();

  if (volumeName.find(SensitiveSurfaceMapper::mappingPrefix) ==
          std::string::npos &&
      !m_cfg.stepLogging) {
    return;
  }

  // The G4Touchable for the matching
  const G4VTouchable* touchable = track->GetTouchable();

  Acts::GeometryIdentifier geoId{};

  // Find the range of candidate surfaces for the current position in the
  // mapping multimap
  auto [bsf, esf] = m_surfaceMapping.equal_range(volume);
  std::size_t nSurfaces = std::distance(bsf, esf);

  ACTS_VERBOSE("Found " << nSurfaces << " candidate surfaces for volume "
                        << volumeName);

  const Acts::Surface* surface = nullptr;
  if (nSurfaces == 0 && !m_cfg.stepLogging) {
    ACTS_ERROR("No candidate surfaces found for volume " << volumeName);
    return;
  } else if (nSurfaces == 1u) {
    geoId = bsf->second->geometryId();
    ACTS_VERBOSE("Unique assignment successful -> to surface " << geoId);
  } else {
    // Find the closest surface to the current position
    Acts::GeometryContext gctx;
    for (; bsf != esf; ++bsf) {
      surface = bsf->second;
      const G4ThreeVector& translation = touchable->GetTranslation();
      Acts::Vector3 g4VolumePosition(convertLength * translation.x(),
                                     convertLength * translation.y(),
                                     convertLength * translation.z());
      if (surface->center(gctx).isApprox(g4VolumePosition)) {
        geoId = surface->geometryId();
        break;
      }
    }
    ACTS_VERBOSE("Replica assignment successful -> to surface " << geoId);
  }

  // This is not the case if we have a particle-ID collision
  if (!eventStore().trackIdMapping.contains(track->GetTrackID())) {
    return;
  }

  // Output is only strictly valid if step logging is not enabled
  const auto particleId = eventStore().trackIdMapping.at(track->GetTrackID());
  if (!m_cfg.stepLogging && surface != nullptr) {
    ACTS_VERBOSE("Step of " << particleId << " in sensitive volume " << geoId);
  } else if (m_cfg.stepLogging) {
    if (!eventStore().propagationRecords.contains(track->GetTrackID())) {
      // Create the propagation summary
      double xVtx = track->GetVertexPosition().x() * convertLength;
      double yVtx = track->GetVertexPosition().y() * convertLength;
      double zVtx = track->GetVertexPosition().z() * convertLength;
      double xDirVtx = track->GetVertexMomentumDirection().x();
      double yDirVtx = track->GetVertexMomentumDirection().y();
      double zDirVtx = track->GetVertexMomentumDirection().z();
      double absMomentum = track->GetMomentum().mag() * convertEnergy;

      PropagationSummary iSummary(Acts::CurvilinearTrackParameters(
          Acts::Vector4(xVtx, yVtx, zVtx, 0.),
          Acts::Vector3(xDirVtx, yDirVtx, zDirVtx), absCharge / absMomentum,
          std::nullopt, Acts::ParticleHypothesis::pion()));

      eventStore().propagationRecords.insert({track->GetTrackID(), iSummary});
    }
    PropagationSummary& pSummary =
        eventStore().propagationRecords.at(track->GetTrackID());

    // Increase the step counter
    pSummary.nSteps += 1;

    double currentTrackLength = track->GetTrackLength() * convertLength;
    double currentStepLength = currentTrackLength - pSummary.pathLength;
    pSummary.pathLength = currentTrackLength;

    // Create a new step for the step logging
    Acts::detail::Step pStep = stepFromG4Step(step);
    pStep.geoID = geoId;
    pStep.surface = surface != nullptr ? surface->getSharedPtr() : nullptr;
    // Check if last step was on same surface
    if (!pSummary.steps.empty() && pSummary.steps.back().geoID == geoId &&
        pSummary.steps.back().surface != nullptr) {
      auto& lastStep = pSummary.steps.back();
      lastStep.stepSize = Acts::ConstrainedStep(currentStepLength);
      lastStep.position = 0.5 * (pStep.position + lastStep.position);
      lastStep.momentum = 0.5 * (pStep.momentum + lastStep.momentum);
    } else {
      // Record the propagation state
      pStep.stepSize = Acts::ConstrainedStep(currentStepLength);
      pSummary.steps.emplace_back(std::move(pStep));
    }
    // You have nothing to do from here
    return;
  }

  // Set particle hit count to zero, so we have this entry in the map later
  if (!eventStore().particleHitCount.contains(particleId)) {
    eventStore().particleHitCount[particleId] = 0;
  }

  // Extract if we are at volume boundaries
  const bool preOnBoundary = preStepPoint->GetStepStatus() == fGeomBoundary;
  const bool postOnBoundary = postStepPoint->GetStepStatus() == fGeomBoundary ||
                              postStepPoint->GetStepStatus() == fWorldBoundary;
  const bool particleStopped = (postStepPoint->GetKineticEnergy() == 0.0);
  const bool particleDecayed =
      (postStepPoint->GetProcessDefinedStep()->GetProcessType() == fDecay);

  auto print = [](auto s) {
#if BOOST_VERSION >= 107800
    return boost::describe::enum_to_string(s, "unmatched");
#else
    return s;
#endif
  };
  ACTS_VERBOSE("status: pre="
               << print(preStepPoint->GetStepStatus())
               << ", post=" << print(postStepPoint->GetStepStatus())
               << ", post E_kin=" << std::boolalpha
               << postStepPoint->GetKineticEnergy() << ", process_type="
               << print(
                      postStepPoint->GetProcessDefinedStep()->GetProcessType())
               << ", particle="
               << track->GetParticleDefinition()->GetParticleName()
               << ", process_name="
               << postStepPoint->GetProcessDefinedStep()->GetProcessName()
               << ", track status=" << print(track->GetTrackStatus()));

  // Case A: The step starts at the entry of the volume and ends at the exit.
  // Add hit to collection.
  if (preOnBoundary && postOnBoundary) {
    ACTS_VERBOSE("-> merge single step to hit");
    ++eventStore().particleHitCount[particleId];
    eventStore().hits.push_back(
        hitFromStep(step, particleId, geoId,
                    eventStore().particleHitCount.at(particleId) - 1));

    eventStore().numberGeantSteps += 1ul;
    eventStore().maxStepsForHit = std::max(eventStore().maxStepsForHit, 1ul);
    return;
  }

  // Case B: The step ends at the exit of the volume. Add hit to hit buffer, and
  // then combine hit buffer
  if (postOnBoundary || particleStopped || particleDecayed) {
    ACTS_VERBOSE("-> merge buffer to hit");
    auto& buffer = eventStore().hitBuffer;
    buffer.push_back(hitFromStep(step, particleId, geoId, -1));

    const auto pos4 =
        0.5 * (buffer.front().fourPosition() + buffer.back().fourPosition());

    ++eventStore().particleHitCount[particleId];
    eventStore().hits.emplace_back(
        geoId, particleId, pos4, buffer.front().momentum4Before(),
        buffer.back().momentum4After(),
        eventStore().particleHitCount.at(particleId) - 1);

    assert(std::ranges::all_of(
        buffer, [&](const auto& h) { return h.geometryId() == geoId; }));
    assert(std::ranges::all_of(
        buffer, [&](const auto& h) { return h.particleId() == particleId; }));

    eventStore().numberGeantSteps += buffer.size();
    eventStore().maxStepsForHit =
        std::max(eventStore().maxStepsForHit, buffer.size());

    buffer.clear();
    return;
  }

  // Case C: The step doesn't end at the exit of the volume. Add the hit to the
  // hit buffer.
  if (!postOnBoundary) {
    // ACTS_VERBOSE("-> add hit to buffer");
    eventStore().hitBuffer.push_back(hitFromStep(step, particleId, geoId, -1));
    return;
  }

  assert(false && "should never reach this");
}

}  // namespace ActsExamples::Geant4
