// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootSeedWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"

#include <ios>
#include <stdexcept>

#include <TFile.h>
#include <TTree.h>

ActsExamples::RootSeedWriter::RootSeedWriter(
    const ActsExamples::RootSeedWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT(config.inputSeeds, "RootSeedWriter", level), m_cfg(config) {
  // inputParticles is already checked by base constructor
  if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing file path");
  }
  if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }

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
  m_outputTree->Branch("measurement_id_1", &m_measurementId_1,
                       "measurement_id_1/l");
  m_outputTree->Branch("measurement_id_2", &m_measurementId_2,
                       "measurement_id_2/l");
  m_outputTree->Branch("measurement_id_3", &m_measurementId_3,
                       "measurement_id_3/l");
  if (m_cfg.writingMode != "small") {
    m_outputTree->Branch("geometry_id_1", &m_geometryId_1, "geometry_id_1/l");
    m_outputTree->Branch("geometry_id_2", &m_geometryId_2, "geometry_id_2/l");
    m_outputTree->Branch("geometry_id_3", &m_geometryId_3, "geometry_id_3/l");
    m_outputTree->Branch("x_1", &m_x_1);
    m_outputTree->Branch("x_2", &m_x_2);
    m_outputTree->Branch("x_3", &m_x_3);
    m_outputTree->Branch("y_1", &m_y_1);
    m_outputTree->Branch("y_2", &m_y_2);
    m_outputTree->Branch("y_3", &m_y_3);
    m_outputTree->Branch("z_1", &m_z_1);
    m_outputTree->Branch("z_2", &m_z_2);
    m_outputTree->Branch("z_3", &m_z_3);
    m_outputTree->Branch("var_r_1", &m_var_r_1);
    m_outputTree->Branch("var_r_2", &m_var_r_2);
    m_outputTree->Branch("var_r_3", &m_var_r_3);
    m_outputTree->Branch("var_z_1", &m_var_z_1);
    m_outputTree->Branch("var_z_2", &m_var_z_2);
    m_outputTree->Branch("var_z_3", &m_var_z_3);
    m_outputTree->Branch("z_vertex", &m_z_vertex);
    m_outputTree->Branch("seed_quality", &m_seed_quality);
  }
}

ActsExamples::RootSeedWriter::~RootSeedWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ActsExamples::ProcessCode ActsExamples::RootSeedWriter::finalize() {
  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  ACTS_VERBOSE("Wrote seeds to tree '" << m_cfg.treeName << "' in '"
                                       << m_cfg.filePath << "'");
  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::RootSeedWriter::writeT(
    const AlgorithmContext& ctx, const ActsExamples::SimSeedContainer& seeds) {
  // ensure exclusive access to tree/file while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  // Get the event number
  m_eventId = ctx.eventNumber;
  for (const auto& seed : seeds) {
    const auto& spacepoints = seed.sp();

    const auto slink_1 =
        spacepoints[0]->sourceLinks()[0].get<IndexSourceLink>();
    const auto slink_2 =
        spacepoints[1]->sourceLinks()[0].get<IndexSourceLink>();
    const auto slink_3 =
        spacepoints[2]->sourceLinks()[0].get<IndexSourceLink>();

    m_measurementId_1 = slink_1.index();
    if (m_cfg.writingMode != "small") {
      m_geometryId_1 = slink_1.geometryId().value();
      m_x_1 = spacepoints[0]->x();
      m_y_1 = spacepoints[0]->y();
      m_z_1 = spacepoints[0]->z();
      m_var_r_1 = spacepoints[0]->varianceR();
      m_var_z_1 = spacepoints[0]->varianceZ();
    }

    m_measurementId_2 = slink_2.index();
    if (m_cfg.writingMode != "small") {
      m_geometryId_2 = slink_2.geometryId().value();
      m_x_2 = spacepoints[1]->x();
      m_y_2 = spacepoints[1]->y();
      m_z_2 = spacepoints[1]->z();
      m_var_r_2 = spacepoints[1]->varianceR();
      m_var_z_2 = spacepoints[1]->varianceZ();
    }

    m_measurementId_3 = slink_3.index();
    if (m_cfg.writingMode != "small") {
      m_geometryId_3 = slink_3.geometryId().value();
      m_x_3 = spacepoints[2]->x();
      m_y_3 = spacepoints[2]->y();
      m_z_3 = spacepoints[2]->z();
      m_var_r_3 = spacepoints[2]->varianceR();
      m_var_z_3 = spacepoints[2]->varianceZ();
    }

    if (m_cfg.writingMode != "small") {
      m_z_vertex = seed.z();
      m_seed_quality = seed.seedQuality();
    }
    // Fill the tree
    m_outputTree->Fill();
  }
  return ActsExamples::ProcessCode::SUCCESS;
}
