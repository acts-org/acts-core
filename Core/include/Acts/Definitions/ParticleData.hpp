// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/PdgParticle.hpp"

#include <iosfwd>
#include <optional>
#include <string_view>

namespace Acts {

struct ParticleData {
  float charge{};
  float mass{};
  std::string_view name;
};

/// Find the charge for a given PDG particle number.
///
/// @return Charge in native units.
std::optional<float> findCharge(PdgParticle pdg);

/// Find the mass for a given PDG particle number.
///
/// @return Mass in native units.
std::optional<float> findMass(PdgParticle pdg);

/// Find a descriptive particle name for a given PDG particle number.
///
/// @return Particle name.
std::optional<std::string_view> findName(PdgParticle pdg);

/// Find all known particle data for a given PDG particle number.
///
/// @return Particle name.
std::optional<ParticleData> findParticleData(PdgParticle pdg);

/// Print PDG particle numbers with a descriptive name.
std::ostream& operator<<(std::ostream& os, PdgParticle pdg);

std::optional<std::string_view> pdgToShortAbsString(PdgParticle pdg);

}  // namespace Acts
