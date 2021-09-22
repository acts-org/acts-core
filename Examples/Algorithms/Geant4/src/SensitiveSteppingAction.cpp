// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/SensitiveSteppingAction.hpp"

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/Geant4/EventStoreRegistry.hpp"
#include "ActsExamples/Geant4/SensitiveSurfaceMapper.hpp"

#include <G4RunManager.hh>
#include <G4Step.hh>
#include <G4StepPoint.hh>
#include <G4Track.hh>
#include <G4UnitsTable.hh>
#include <G4VPhysicalVolume.hh>

ActsExamples::SensitiveSteppingAction::SensitiveSteppingAction(
    const Config& cfg, std::unique_ptr<const Acts::Logger> logger)
    : G4UserSteppingAction(), m_cfg(cfg), m_logger(std::move(logger)) {}

void ActsExamples::SensitiveSteppingAction::UserSteppingAction(
    const G4Step* step) {
  // Unit conversions G4->::ACTS
  constexpr double convertTime = Acts::UnitConstants::s / CLHEP::s;
  constexpr double convertLength = Acts::UnitConstants::mm / CLHEP::mm;
  constexpr double convertEnergy = Acts::UnitConstants::GeV / CLHEP::GeV;

  // The particle after the step
  G4Track* track = step->GetTrack();
  G4PrimaryParticle* primaryParticle =
      track->GetDynamicParticle()->GetPrimaryParticle();

  // Bail out if charged & configured to do so
  G4double absCharge = std::abs(track->GetParticleDefinition()->GetPDGCharge());
  if (not m_cfg.charged and absCharge > 0.) {
    return;
  }

  // Bail out if neutral & configured to do so
  if (not m_cfg.neutral and absCharge == 0.) {
    return;
  }

  // Bail out if it is a primary & configured to be ignored
  if (not m_cfg.primary and primaryParticle != nullptr) {
    return;
  }

  // Bail out if it is a secondary & configured to be ignored
  if (not m_cfg.secondary and primaryParticle == nullptr) {
    return;
  }

  // Get the physical volume & check if it has the sensitive string name
  G4VPhysicalVolume* volume = track->GetVolume();
  std::string volumeName = volume->GetName();

  std::string mappingPfx(SensitiveSurfaceMapper::mappingPrefix);

  if (volumeName.find(mappingPfx) != std::string::npos) {
    ACTS_VERBOSE("Step in senstive volume " << volumeName);

    // Get PreStepPoint and PostStepPoint
    G4StepPoint* preStepPoint = step->GetPreStepPoint();
    G4StepPoint* postStepPoint = step->GetPostStepPoint();

    G4ThreeVector preStepPosition = convertLength * preStepPoint->GetPosition();
    G4double preStepTime = convertTime * preStepPoint->GetGlobalTime();
    G4ThreeVector postStepPosition =
        convertLength * postStepPoint->GetPosition();
    G4double postStepTime = convertTime * postStepPoint->GetGlobalTime();

    G4ThreeVector preStepMomentum = convertEnergy * preStepPoint->GetMomentum();
    G4double preStepEnergy = convertEnergy * preStepPoint->GetTotalEnergy();
    G4ThreeVector postStepMomentum =
        convertEnergy * postStepPoint->GetMomentum();
    G4double postStepEnergy = convertEnergy * postStepPoint->GetTotalEnergy();

    Acts::ActsScalar hX = 0.5 * (preStepPosition[0] + postStepPosition[0]);
    Acts::ActsScalar hY = 0.5 * (preStepPosition[1] + postStepPosition[1]);
    Acts::ActsScalar hZ = 0.5 * (preStepPosition[2] + postStepPosition[2]);
    Acts::ActsScalar hT = 0.5 * (preStepTime + postStepTime);

    Acts::ActsScalar mXpre = preStepMomentum[0];
    Acts::ActsScalar mYpre = preStepMomentum[1];
    Acts::ActsScalar mZpre = preStepMomentum[2];
    Acts::ActsScalar mEpre = preStepEnergy;
    Acts::ActsScalar mXpost = postStepMomentum[0];
    Acts::ActsScalar mYpost = postStepMomentum[1];
    Acts::ActsScalar mZpost = postStepMomentum[2];
    Acts::ActsScalar mEpost = postStepEnergy;

    // Cast out the GeometryIdentifier
    volumeName.erase(0, mappingPfx.size());

    Acts::GeometryIdentifier::Value sGeoVal = std::stoul(volumeName);
    Acts::GeometryIdentifier geoID(sGeoVal);

    SimBarcode particleID(track->GetTrackID());
    Acts::Vector4 particlePosition(hX, hY, hZ, hT);
    Acts::Vector4 beforeMomentum(mXpre, mYpre, mZpre, mEpre);
    Acts::Vector4 afterMomentum(mXpost, mYpost, mZpost, mEpost);

    // Retrieve the event data registry
    auto& eventData = EventStoreRegistry::eventData();

    // Fill into the registry
    eventData.hits.emplace_back(geoID, particleID, particlePosition,
                                beforeMomentum, afterMomentum,
                                eventData.hits.size());
  }
}
