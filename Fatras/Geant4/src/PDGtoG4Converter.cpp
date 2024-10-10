// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsFatras/Geant4/PDGtoG4Converter.hpp"

#include <cstdlib>
#include <utility>

#include "G4ParticleDefinition.hh"
#include "G4ParticleTypes.hh"

ActsFatras::PDGtoG4Converter::PDGtoG4Converter() : m_pdgG4ParticleMap() {
  /// Fill the storage
  fillPredefinedParticles();
}

G4ParticleDefinition* ActsFatras::PDGtoG4Converter::getParticleDefinition(
    Acts::PdgParticle pdgCode) const {
  std::unordered_map<Acts::PdgParticle, G4ParticleDefinition*>::const_iterator
      it = m_pdgG4ParticleMap.find(pdgCode);

  // Find the corresponding particle and return it
  if (it != m_pdgG4ParticleMap.end()) {
    return it->second;
  } else {
    // Rescue mechanism: If the particle is neutral and its anti-particle is
    // stored then return that one instead
    it = m_pdgG4ParticleMap.find(makeAbsolutePdgParticle(pdgCode));
    if (it != m_pdgG4ParticleMap.end() &&
        std::abs(it->second->GetPDGCharge()) < 0.1) {
      return it->second;
    }
  }
  // No particle found
  return nullptr;
}

void ActsFatras::PDGtoG4Converter::fillPredefinedParticles() {
  // Gauge and Higgs Bosons
  addParticle(G4Gamma::GammaDefinition());

  // Leptons
  addParticle(G4Electron::ElectronDefinition());
  addParticle(G4NeutrinoE::NeutrinoEDefinition());
  addParticle(G4MuonMinus::MuonMinusDefinition());
  addParticle(G4NeutrinoMu::NeutrinoMuDefinition());
  addParticle(G4TauMinus::TauMinusDefinition());
  addParticle(G4NeutrinoTau::NeutrinoTauDefinition());
  addParticle(G4Positron::PositronDefinition());
  addParticle(G4AntiNeutrinoE::AntiNeutrinoEDefinition());
  addParticle(G4MuonPlus::MuonPlusDefinition());
  addParticle(G4AntiNeutrinoMu::AntiNeutrinoMuDefinition());
  addParticle(G4TauPlus::TauPlusDefinition());
  addParticle(G4AntiNeutrinoTau::AntiNeutrinoTauDefinition());

  // Light I=1 Mesons
  addParticle(G4PionZero::PionZeroDefinition());
  addParticle(G4PionPlus::PionPlusDefinition());
  addParticle(G4PionMinus::PionMinusDefinition());

  // Light I=0 Mesons
  addParticle(G4Eta::EtaDefinition());
  addParticle(G4EtaPrime::EtaPrimeDefinition());

  // Strange Mesons
  addParticle(G4KaonZeroLong::KaonZeroLongDefinition());
  addParticle(G4KaonZeroShort::KaonZeroShortDefinition());
  addParticle(G4KaonZero::KaonZeroDefinition());
  addParticle(G4KaonPlus::KaonPlusDefinition());
  addParticle(G4AntiKaonZero::AntiKaonZeroDefinition());
  addParticle(G4KaonMinus::KaonMinusDefinition());

  // Charmed Mesons
  addParticle(G4DMesonPlus::DMesonPlusDefinition());
  addParticle(G4DMesonZero::DMesonZeroDefinition());
  addParticle(G4DsMesonPlus::DsMesonPlusDefinition());
  addParticle(G4DMesonMinus::DMesonMinusDefinition());
  addParticle(G4AntiDMesonZero::AntiDMesonZeroDefinition());
  addParticle(G4DsMesonMinus::DsMesonMinusDefinition());

  // Bottom Mesons
  addParticle(G4BMesonZero::BMesonZeroDefinition());
  addParticle(G4BMesonPlus::BMesonPlusDefinition());
  addParticle(G4BsMesonZero::BsMesonZeroDefinition());
  addParticle(G4AntiBMesonZero::AntiBMesonZeroDefinition());
  addParticle(G4BMesonMinus::BMesonMinusDefinition());
  addParticle(G4AntiBsMesonZero::AntiBsMesonZeroDefinition());

  // ccbar Mesons
  addParticle(G4JPsi::JPsiDefinition());

  // Light Baryons
  addParticle(G4Proton::ProtonDefinition());
  addParticle(G4Neutron::NeutronDefinition());
  addParticle(G4AntiProton::AntiProtonDefinition());
  addParticle(G4AntiNeutron::AntiNeutronDefinition());

  // Strange Baryons
  addParticle(G4Lambda::LambdaDefinition());
  addParticle(G4SigmaPlus::SigmaPlusDefinition());
  addParticle(G4SigmaZero::SigmaZeroDefinition());
  addParticle(G4SigmaMinus::SigmaMinusDefinition());
  addParticle(G4XiZero::XiZeroDefinition());
  addParticle(G4XiMinus::XiMinusDefinition());
  addParticle(G4OmegaMinus::OmegaMinusDefinition());
  addParticle(G4AntiLambda::AntiLambdaDefinition());
  addParticle(G4AntiSigmaPlus::AntiSigmaPlusDefinition());
  addParticle(G4AntiSigmaZero::AntiSigmaZeroDefinition());
  addParticle(G4AntiSigmaMinus::AntiSigmaMinusDefinition());
  addParticle(G4AntiXiZero::AntiXiZeroDefinition());
  addParticle(G4AntiXiMinus::AntiXiMinusDefinition());
  addParticle(G4AntiOmegaMinus::AntiOmegaMinusDefinition());

  // Charmed Baryons
  addParticle(G4LambdacPlus::LambdacPlusDefinition());
  addParticle(G4SigmacPlusPlus::SigmacPlusPlusDefinition());
  addParticle(G4SigmacPlus::SigmacPlusDefinition());
  addParticle(G4SigmacZero::SigmacZeroDefinition());
  addParticle(G4XicPlus::XicPlusDefinition());
  addParticle(G4XicZero::XicZeroDefinition());
  addParticle(G4OmegacZero::OmegacZeroDefinition());
  addParticle(G4AntiLambdacPlus::AntiLambdacPlusDefinition());
  addParticle(G4AntiSigmacPlusPlus::AntiSigmacPlusPlusDefinition());
  addParticle(G4AntiSigmacPlus::AntiSigmacPlusDefinition());
  addParticle(G4AntiSigmacZero::AntiSigmacZeroDefinition());
  addParticle(G4AntiXicPlus::AntiXicPlusDefinition());
  addParticle(G4AntiXicZero::AntiXicZeroDefinition());
  addParticle(G4AntiOmegacZero::AntiOmegacZeroDefinition());
}

void ActsFatras::PDGtoG4Converter::addParticle(G4ParticleDefinition* pDef) {
  if (pDef == nullptr) {
    return;
  }

  m_pdgG4ParticleMap[static_cast<Acts::PdgParticle>(pDef->GetPDGEncoding())] =
      pDef;
}
