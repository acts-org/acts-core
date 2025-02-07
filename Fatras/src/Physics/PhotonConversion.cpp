// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsFatras/Physics/ElectroMagnetic/PhotonConversion.hpp"

#include "Acts/Definitions/ParticleData.hpp"

const double ActsFatras::PhotonConversion::kElectronMass =
    Acts::findMass(Acts::PdgParticle::eElectron).value();
