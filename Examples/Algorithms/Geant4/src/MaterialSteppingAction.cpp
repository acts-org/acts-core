// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/MaterialSteppingAction.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "ActsExamples/Geant4/EventStore.hpp"

#include <cstddef>
#include <ostream>
#include <unordered_map>
#include <utility>

#include <G4Material.hh>
#include <G4RunManager.hh>
#include <G4Step.hh>

ActsExamples::MaterialSteppingAction::MaterialSteppingAction(
    const Config& cfg, std::unique_ptr<const Acts::Logger> logger)
    : G4UserSteppingAction(), m_cfg(cfg), m_logger(std::move(logger)) {}

ActsExamples::MaterialSteppingAction::~MaterialSteppingAction() = default;

void ActsExamples::MaterialSteppingAction::UserSteppingAction(
    const G4Step* step) {
  // Get the material & check if it is present
  G4Material* material = step->GetPreStepPoint()->GetMaterial();
  if (material == nullptr) {
    return;
  }

  // First check for exclusion
  std::string materialName = material->GetName();
  for (const auto& emat : m_cfg.excludeMaterials) {
    if (emat == materialName) {
      ACTS_VERBOSE("Exclude step in material '" << materialName << "'.");
      return;
    }
  }

  constexpr double convertLength = Acts::UnitConstants::mm / CLHEP::mm;
  constexpr double convertDensity =
      (Acts::UnitConstants::g / Acts::UnitConstants::mm3) /
      (CLHEP::gram / CLHEP::mm3);

  ACTS_VERBOSE("Performing a step with step size = "
               << convertLength * step->GetStepLength());

  // Quantities valid for elemental materials and mixtures
  double X0 = convertLength * material->GetRadlen();
  double L0 = convertLength * material->GetNuclearInterLength();
  double rho = convertDensity * material->GetDensity();

  // Get{A,Z} is only meaningful for single-element materials (according to
  // the Geant4 docs). Need to compute average manually.
  const G4ElementVector* elements = material->GetElementVector();
  const G4double* fraction = material->GetFractionVector();
  std::size_t nElements = material->GetNumberOfElements();
  double Ar = 0.;
  double Z = 0.;
  if (nElements == 1) {
    Ar = material->GetA() / (CLHEP::gram / CLHEP::mole);
    Z = material->GetZ();
  } else {
    for (std::size_t i = 0; i < nElements; i++) {
      Ar += elements->at(i)->GetA() * fraction[i] / (CLHEP::gram / CLHEP::mole);
      Z += elements->at(i)->GetZ() * fraction[i];
    }
  }

  // Construct passed material slab for the step
  const auto slab =
      Acts::MaterialSlab(Acts::Material::fromMassDensity(X0, L0, Ar, Z, rho),
                         convertLength * step->GetStepLength());

  // Create the RecordedMaterialSlab
  const auto& rawPos = step->GetPreStepPoint()->GetPosition();
  const auto& rawDir = step->GetPreStepPoint()->GetMomentum();
  Acts::MaterialInteraction mInteraction;
  mInteraction.position =
      Acts::Vector3(convertLength * rawPos.x(), convertLength * rawPos.y(),
                    convertLength * rawPos.z());
  mInteraction.direction = Acts::Vector3(rawDir.x(), rawDir.y(), rawDir.z());
  mInteraction.direction.normalized();
  mInteraction.materialSlab = slab;
  mInteraction.pathCorrection = (step->GetStepLength() / CLHEP::mm);

  G4Track* g4Track = step->GetTrack();
  std::size_t trackID = g4Track->GetTrackID();
  auto& materialTracks = eventStore().materialTracks;
  if (!materialTracks.contains(trackID - 1)) {
    Acts::RecordedMaterialTrack rmTrack;
    const auto& g4Vertex = g4Track->GetVertexPosition();
    Acts::Vector3 vertex(g4Vertex[0], g4Vertex[1], g4Vertex[2]);
    const auto& g4Direction = g4Track->GetMomentumDirection();
    Acts::Vector3 direction(g4Direction[0], g4Direction[1], g4Direction[2]);
    rmTrack.first = {vertex, direction};
    rmTrack.second.materialInteractions.push_back(mInteraction);
    materialTracks[trackID - 1] = rmTrack;
  } else {
    materialTracks[trackID - 1].second.materialInteractions.push_back(
        mInteraction);
  }
}
