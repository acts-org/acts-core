// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/SensitiveSurfaceMapper.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <stdexcept>

#include <G4LogicalVolume.hh>
#include <G4Material.hh>
#include <G4VPhysicalVolume.hh>
#include <globals.hh>

ActsExamples::SensitiveSurfaceMapper::SensitiveSurfaceMapper(
    const Config& cfg, std::unique_ptr<const Acts::Logger> logger)
    : m_cfg(cfg), m_logger(std::move(logger)) {
  if (m_cfg.trackingGeometry == nullptr) {
    throw std::invalid_argument("No Acts::TrackingGeometry provided.");
  }
}

void ActsExamples::SensitiveSurfaceMapper::remapSensitiveNames(
    G4VPhysicalVolume* g4PhysicalVolume, const Acts::Vector3 motherPosition,
    int& sCounter) const {
  auto g4LogicalVolume = g4PhysicalVolume->GetLogicalVolume();
  auto g4SensitiveDetector = g4LogicalVolume->GetSensitiveDetector();

  G4int nDaughters = g4LogicalVolume->GetNoDaughters();

  constexpr double convertLength = CLHEP::mm / Acts::UnitConstants::mm;

  // Get the relative translation of the G4 object
  auto g4RelTranslation = g4PhysicalVolume->GetTranslation();
  Acts::Vector3 g4RelPosition(g4RelTranslation[0] * convertLength,
                              g4RelTranslation[1] * convertLength,
                              g4RelTranslation[2] * convertLength);

  if (nDaughters == 0) {
    std::string volumeName = g4LogicalVolume->GetName();
    std::string volumeMaterialName = g4LogicalVolume->GetMaterial()->GetName();
    if (g4SensitiveDetector != nullptr or
        std::find(m_cfg.materialMappings.begin(), m_cfg.materialMappings.end(),
                  volumeMaterialName) != m_cfg.materialMappings.end() or
        std::find(m_cfg.volumeMappings.begin(), m_cfg.volumeMappings.end(),
                  volumeName) != m_cfg.volumeMappings.end()) {
      // Find the associated ACTS object
      Acts::Vector3 g4AbsPosition = g4RelPosition + motherPosition;
      auto actsLayer = m_cfg.trackingGeometry->associatedLayer(
          Acts::GeometryContext(), g4AbsPosition);

      // Prepare the mapped surface
      const Acts::Surface* mappedSurface = nullptr;

      if (actsLayer and actsLayer->surfaceArray()) {
        auto actsSurfaces = actsLayer->surfaceArray()->at(g4AbsPosition);
        if (not actsSurfaces.empty()) {
          // Fast matching: search
          for (const auto& as : actsSurfaces) {
            if (as->center(Acts::GeometryContext()).isApprox(g4AbsPosition)) {
              mappedSurface = as;
              break;
            }
          }
        }
        if (mappedSurface == nullptr) {
          // Slow matching: Fallback, loop over all layer surfaces
          for (const auto& as : actsLayer->surfaceArray()->surfaces()) {
            if (as->center(Acts::GeometryContext()).isApprox(g4AbsPosition)) {
              mappedSurface = as;
              break;
            }
          }
        }
      }
      // A mapped surface was found, a new name will be set that
      // contains the GeometryID/
      if (mappedSurface != nullptr) {
        ++sCounter;
        std::string mappedVolumeName = SensitiveSurfaceMapper::mappingPrefix;
        mappedVolumeName += std::to_string(mappedSurface->geometryId().value());
        ACTS_VERBOSE("Remap sensitive volume: " << g4PhysicalVolume->GetName());
        ACTS_VERBOSE("                    to: " << mappedVolumeName);
        g4PhysicalVolume->SetName(mappedVolumeName.c_str());
      }
    }
  } else {
    // Step down to all daughters
    for (G4int id = 0; id < nDaughters; ++id) {
      remapSensitiveNames(g4LogicalVolume->GetDaughter(id),
                          motherPosition + g4RelPosition, sCounter);
    }
  }
}
