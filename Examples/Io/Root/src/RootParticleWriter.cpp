// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootParticleWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <cstdint>
#include <ios>
#include <limits>
#include <ostream>
#include <stdexcept>

#include <TFile.h>
#include <TTree.h>

ActsExamples::RootParticleWriter::RootParticleWriter(
    const ActsExamples::RootParticleWriter::Config& cfg,
    Acts::Logging::Level lvl)
    : WriterT(cfg.inputParticles, "RootParticleWriter", lvl), m_cfg(cfg) {
  // inputParticles is already checked by base constructor
  if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing file path");
  }
  if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }

  m_inputFinalParticles.maybeInitialize(m_cfg.inputFinalParticles);

  // open root file and create the tree
  m_outputFile = TFile::Open(m_cfg.filePath.c_str(), m_cfg.fileMode.c_str());
  if (m_outputFile == nullptr) {
    throw std::ios_base::failure("Could not open '" + m_cfg.filePath + "'");
  }
  m_outputFile->cd();
  m_outputTree = new TTree(m_cfg.treeName.c_str(), m_cfg.treeName.c_str());
  if (m_outputTree == nullptr) {
    throw std::bad_alloc();
  }

  // setup the branches
  m_outputTree->Branch("event_id", &m_eventId);
  m_outputTree->Branch("particle_id", &m_particleId);
  m_outputTree->Branch("particle_type", &m_particleType);
  m_outputTree->Branch("process", &m_process);
  m_outputTree->Branch("vx", &m_vx);
  m_outputTree->Branch("vy", &m_vy);
  m_outputTree->Branch("vz", &m_vz);
  m_outputTree->Branch("vt", &m_vt);
  m_outputTree->Branch("px", &m_px);
  m_outputTree->Branch("py", &m_py);
  m_outputTree->Branch("pz", &m_pz);
  m_outputTree->Branch("m", &m_m);
  m_outputTree->Branch("q", &m_q);
  m_outputTree->Branch("eta", &m_eta);
  m_outputTree->Branch("phi", &m_phi);
  m_outputTree->Branch("pt", &m_pt);
  m_outputTree->Branch("p", &m_p);
  m_outputTree->Branch("vertex_primary", &m_vertexPrimary);
  m_outputTree->Branch("vertex_secondary", &m_vertexSecondary);
  m_outputTree->Branch("particle", &m_particle);
  m_outputTree->Branch("generation", &m_generation);
  m_outputTree->Branch("sub_particle", &m_subParticle);

  if (m_inputFinalParticles.isInitialized()) {
    m_outputTree->Branch("e_loss", &m_eLoss);
    m_outputTree->Branch("total_x0", &m_pathInX0);
    m_outputTree->Branch("total_l0", &m_pathInL0);
    m_outputTree->Branch("number_of_hits", &m_numberOfHits);
    m_outputTree->Branch("outcome", &m_outcome);
  }
}

ActsExamples::RootParticleWriter::~RootParticleWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ActsExamples::ProcessCode ActsExamples::RootParticleWriter::finalize() {
  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  ACTS_INFO("Wrote particles to tree '" << m_cfg.treeName << "' in '"
                                        << m_cfg.filePath << "'");

  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::RootParticleWriter::writeT(
    const AlgorithmContext& ctx, const SimParticleContainer& particles) {
  const SimParticleContainer* finalParticles = nullptr;
  if (m_inputFinalParticles.isInitialized()) {
    finalParticles = &m_inputFinalParticles(ctx);
  }

  // ensure exclusive access to tree/file while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  auto nan = std::numeric_limits<float>::quiet_NaN();

  m_eventId = ctx.eventNumber;
  for (const auto& particle : particles) {
    m_particleId.push_back(particle.particleId().value());
    m_particleType.push_back(particle.pdg());
    m_process.push_back(static_cast<std::uint32_t>(particle.process()));
    // position
    m_vx.push_back(Acts::clampValue<float>(particle.fourPosition().x() /
                                           Acts::UnitConstants::mm));
    m_vy.push_back(Acts::clampValue<float>(particle.fourPosition().y() /
                                           Acts::UnitConstants::mm));
    m_vz.push_back(Acts::clampValue<float>(particle.fourPosition().z() /
                                           Acts::UnitConstants::mm));
    m_vt.push_back(Acts::clampValue<float>(particle.fourPosition().w() /
                                           Acts::UnitConstants::mm));
    // momentum
    const auto p = particle.absoluteMomentum() / Acts::UnitConstants::GeV;
    m_p.push_back(Acts::clampValue<float>(p));
    m_px.push_back(Acts::clampValue<float>(p * particle.direction().x()));
    m_py.push_back(Acts::clampValue<float>(p * particle.direction().y()));
    m_pz.push_back(Acts::clampValue<float>(p * particle.direction().z()));
    // particle constants
    m_m.push_back(
        Acts::clampValue<float>(particle.mass() / Acts::UnitConstants::GeV));
    m_q.push_back(
        Acts::clampValue<float>(particle.charge() / Acts::UnitConstants::e));
    // derived kinematic quantities
    m_eta.push_back(Acts::clampValue<float>(
        Acts::VectorHelpers::eta(particle.direction())));
    m_phi.push_back(Acts::clampValue<float>(
        Acts::VectorHelpers::phi(particle.direction())));
    m_pt.push_back(Acts::clampValue<float>(
        p * Acts::VectorHelpers::perp(particle.direction())));
    // decoded barcode components
    m_vertexPrimary.push_back(particle.particleId().vertexPrimary());
    m_vertexSecondary.push_back(particle.particleId().vertexSecondary());
    m_particle.push_back(particle.particleId().particle());
    m_generation.push_back(particle.particleId().generation());
    m_subParticle.push_back(particle.particleId().subParticle());

    bool wroteFinalParticle = false;
    if (finalParticles != nullptr) {
      // get the final particle
      auto it = finalParticles->find(particle);
      if (it == finalParticles->end()) {
        ACTS_ERROR("Could not find final particle for "
                   << particle.particleId() << " in event " << ctx.eventNumber);
      } else {
        const auto& finalParticle = *it;
        // get the energy loss
        m_eLoss.push_back(Acts::clampValue<float>(
            (particle.energy() - finalParticle.energy()) /
            Acts::UnitConstants::GeV));
        // get the path in X0
        m_pathInX0.push_back(Acts::clampValue<float>(finalParticle.pathInX0() /
                                                     Acts::UnitConstants::mm));
        // get the path in L0
        m_pathInL0.push_back(Acts::clampValue<float>(finalParticle.pathInL0() /
                                                     Acts::UnitConstants::mm));
        // get the number of hits
        m_numberOfHits.push_back(finalParticle.numberOfHits());
        // get the particle outcome
        m_outcome.push_back(
            static_cast<std::uint32_t>(finalParticle.outcome()));

        wroteFinalParticle = true;
      }
    }
    if (!wroteFinalParticle) {
      m_eLoss.push_back(nan);
      m_pathInX0.push_back(nan);
      m_pathInL0.push_back(nan);
      m_numberOfHits.push_back(-1);
      m_outcome.push_back(0);
    }
  }

  m_outputTree->Fill();

  m_particleId.clear();
  m_particleType.clear();
  m_process.clear();
  m_vx.clear();
  m_vy.clear();
  m_vz.clear();
  m_vt.clear();
  m_p.clear();
  m_px.clear();
  m_py.clear();
  m_pz.clear();
  m_m.clear();
  m_q.clear();
  m_eta.clear();
  m_phi.clear();
  m_pt.clear();
  m_vertexPrimary.clear();
  m_vertexSecondary.clear();
  m_particle.clear();
  m_generation.clear();
  m_subParticle.clear();
  m_eLoss.clear();
  m_numberOfHits.clear();
  m_outcome.clear();
  m_pathInX0.clear();
  m_pathInL0.clear();

  return ProcessCode::SUCCESS;
}
