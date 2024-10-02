// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Geant4/Geant4PhysicalVolumeSelectors.hpp"

#include "G4VPhysicalVolume.hh"

namespace Acts::Geant4PhysicalVolumeSelectors {

bool NameSelector::select(const G4VPhysicalVolume& g4PhysVol) const {
  std::string volumeName = g4PhysVol.GetName();
  bool matched = false;
  for (const auto& name : names) {
    matched = exact ? (volumeName == name)
                    : volumeName.find(name) != std::string::npos;
    if (matched) {
      break;
    }
  }
  return matched;
}

bool PositionSelector::select(const G4VPhysicalVolume& g4PhysVol) const {
  bool matched = false;
  G4ThreeVector pos = g4PhysVol.GetTranslation();
  for (auto range : m_ranges) {
    auto& [min, max] = range.second;
    EAxis axis = static_cast<EAxis>(range.first);
    matched = (pos[axis] >= min) && (pos[axis] <= max);
    if (!matched) {
      break;
    }
  }
  return matched;
}

}  // namespace Acts::Geant4PhysicalVolumeSelectors
