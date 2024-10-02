// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/PdgParticle.hpp"

#include <unordered_map>

class G4ParticleDefinition;

namespace ActsFatras {
/// This class converts a PDG ID into a corresponding Geant4 particle.
class PDGtoG4Converter {
 public:
  /// Constructor
  PDGtoG4Converter();

  /// Convert a PDG ID into the corresponding Geant4 particle.
  ///
  /// @param [in] pdgCode The PDG ID
  ///
  /// @return Pointer to the Geant4 particle
  G4ParticleDefinition* getParticleDefinition(Acts::PdgParticle pdgCode) const;

 private:
  /// Fills the internal lookup with PDG ids and their Geant4 particles.
  void fillPredefinedParticles();

  /// Add a certain Particle to the internal lookup.
  ///
  /// @param [in] pDef The Geant4 particle that will be added
  void addParticle(G4ParticleDefinition* pDef);

  /// The internal storage consisting of PDG ID and the Geant4 particle
  std::unordered_map<Acts::PdgParticle, G4ParticleDefinition*>
      m_pdgG4ParticleMap;
};
}  // namespace ActsFatras
