// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Definitions/Units.hpp"
#include "ActsFatras/EventData/Particle.hpp"

namespace Dataset {

using namespace Acts::UnitLiterals;

ActsFatras::Particle makeParticle(Acts::PdgParticle pdg, double z, double eta) {
  const auto id = ActsFatras::Barcode().setVertexPrimary(1).setParticle(1);
  return ActsFatras::Particle(id, pdg)
      .setPosition4(0.0, 0.0, z, 0.0)
      .setDirection(1.0 / std::cosh(eta), 0.0, std::tanh(eta))
      .setAbsoluteMomentum(1.5_GeV);
}

const auto centralElectron =
    makeParticle(Acts::PdgParticle::eElectron, 0_mm, 0.0);
const auto centralPositron =
    makeParticle(Acts::PdgParticle::ePositron, 0_mm, 0.0);
const auto centralMuon = makeParticle(Acts::PdgParticle::eMuon, 0_mm, 0.0);
const auto centralAntiMuon =
    makeParticle(Acts::PdgParticle::eAntiMuon, 0_mm, 0.0);
const auto backwardPion =
    makeParticle(Acts::PdgParticle::ePionMinus, -100_mm, -4.5);
const auto centralPion = makeParticle(Acts::PdgParticle::ePionMinus, 0_mm, 0.0);
const auto forwardPion =
    makeParticle(Acts::PdgParticle::ePionMinus, 100_mm, 4.5);
const auto centralNeutron =
    makeParticle(Acts::PdgParticle::eNeutron, 1_mm, 0.1);

}  // namespace Dataset
