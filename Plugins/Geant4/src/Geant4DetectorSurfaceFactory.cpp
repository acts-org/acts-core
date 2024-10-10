// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Geant4/Geant4DetectorSurfaceFactory.hpp"

#include "Acts/Plugins/Geant4/Geant4Converters.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorElement.hpp"
#include "Acts/Plugins/Geant4/Geant4PhysicalVolumeSelectors.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/StringHelpers.hpp"

#include <utility>

#include "G4LogicalVolume.hh"
#include "G4Transform3D.hh"
#include "G4VPhysicalVolume.hh"

void Acts::Geant4DetectorSurfaceFactory::construct(
    Cache& cache, const G4Transform3D& g4ToGlobal,
    const G4VPhysicalVolume& g4PhysVol, const Options& option) {
  // Get Rotation and translation
  auto g4Translation = g4PhysVol.GetTranslation();
  auto g4Rotation = g4PhysVol.GetRotation();

  auto newTranslation =
      g4ToGlobal.getTranslation() + g4ToGlobal.getRotation() * g4Translation;
  auto newRotation = (g4Rotation == nullptr)
                         ? g4ToGlobal.getRotation() * CLHEP::HepRotation()
                         : g4ToGlobal.getRotation() * g4Rotation->inverse();

  G4Transform3D newToGlobal(newRotation, newTranslation);

  // Get the logical volume
  auto g4LogicalVolume = g4PhysVol.GetLogicalVolume();
  std::size_t nDaughters = g4LogicalVolume->GetNoDaughters();
  for (std::size_t d = 0; d < nDaughters; ++d) {
    auto daughter = g4LogicalVolume->GetDaughter(d);
    construct(cache, newToGlobal, *daughter, option);
  }

  // Check if the volume is accepted by a sensitive or passive selector
  bool sensitive = option.sensitiveSurfaceSelector != nullptr &&
                   option.sensitiveSurfaceSelector->select(g4PhysVol);
  bool passive = option.passiveSurfaceSelector != nullptr &&
                 option.passiveSurfaceSelector->select(g4PhysVol);

  if (sensitive || passive) {
    // Conversion and selection code
    ++cache.matchedG4Volumes;

    // Attempt the conversion
    auto surface = Acts::Geant4PhysicalVolumeConverter{}.surface(
        g4PhysVol, Geant4AlgebraConverter{}.transform(newToGlobal),
        option.convertMaterial, option.convertedMaterialThickness);

    if (surface != nullptr) {
      ++cache.convertedSurfaces;
      // Count the material conversion
      if (surface->surfaceMaterial() != nullptr) {
        ++cache.convertedMaterials;
      }

      if (sensitive) {
        // empty geometry context is fine as the transform was just passed down
        // without context before
        auto detectorElement = std::make_shared<Acts::Geant4DetectorElement>(
            surface, g4PhysVol, surface->transform({}), 0.1);
        surface->assignDetectorElement(*detectorElement);

        cache.sensitiveSurfaces.push_back(
            {std::move(detectorElement), std::move(surface)});
      } else {
        cache.passiveSurfaces.push_back(std::move(surface));
      }
    }
  }
}
