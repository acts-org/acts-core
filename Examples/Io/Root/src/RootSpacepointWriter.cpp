// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootSpacepointWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"

#include <ios>
#include <ostream>
#include <stdexcept>
#include <vector>

#include <TFile.h>
#include <TTree.h>

ActsExamples::RootSpacepointWriter::RootSpacepointWriter(
    const ActsExamples::RootSpacepointWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT(config.inputSpacepoints, "RootSpacepointWriter", level),
      m_cfg(config) {
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
  m_outputTree->Branch("measurement_id", &m_measurementId, "measurement_id/l");
  m_outputTree->Branch("geometry_id", &m_geometryId, "geometry_id/l");
  m_outputTree->Branch("x", &m_x);
  m_outputTree->Branch("y", &m_y);
  m_outputTree->Branch("z", &m_z);
  m_outputTree->Branch("var_r", &m_var_r);
  m_outputTree->Branch("var_z", &m_var_z);
}

ActsExamples::RootSpacepointWriter::~RootSpacepointWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ActsExamples::ProcessCode ActsExamples::RootSpacepointWriter::finalize() {
  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  ACTS_VERBOSE("Wrote hits to tree '" << m_cfg.treeName << "' in '"
                                      << m_cfg.filePath << "'");

  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::RootSpacepointWriter::writeT(
    const AlgorithmContext& ctx,
    const ActsExamples::SimSpacePointContainer& spacepoints) {
  // ensure exclusive access to tree/file while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  // Get the event number
  m_eventId = ctx.eventNumber;
  for (const auto& sp : spacepoints) {
    const auto& slinkPtr = sp.sourceLinks()[0].get<IndexSourceLink>();
    m_measurementId = slinkPtr.index();
    m_geometryId = slinkPtr.geometryId().value();
    // write sp position
    m_x = sp.x() / Acts::UnitConstants::mm;
    m_y = sp.y() / Acts::UnitConstants::mm;
    m_z = sp.z() / Acts::UnitConstants::mm;
    // write sp dimensions
    m_var_r = sp.varianceR() / Acts::UnitConstants::mm;
    m_var_z = sp.varianceZ() / Acts::UnitConstants::mm;
    // Fill the tree
    m_outputTree->Fill();
  }
  return ActsExamples::ProcessCode::SUCCESS;
}
