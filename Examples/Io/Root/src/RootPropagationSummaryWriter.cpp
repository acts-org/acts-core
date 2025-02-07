// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/Root/RootPropagationSummaryWriter.hpp"

#include "Acts/Utilities/VectorHelpers.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Propagation/PropagationAlgorithm.hpp"

#include <ios>
#include <ostream>
#include <stdexcept>

#include <TFile.h>
#include <TTree.h>

namespace ActsExamples {

RootPropagationSummaryWriter::RootPropagationSummaryWriter(
    const RootPropagationSummaryWriter::Config& cfg, Acts::Logging::Level level)
    : WriterT(cfg.inputSummaryCollection, "RootPropagationSummaryWriter",
              level),
      m_cfg(cfg),
      m_outputFile(cfg.rootFile) {
  // An input collection name and tree name must be specified
  if (m_cfg.inputSummaryCollection.empty()) {
    throw std::invalid_argument("Missing input collection");
  }
  if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }

  // Setup ROOT I/O
  if (m_outputFile == nullptr) {
    m_outputFile = TFile::Open(m_cfg.filePath.c_str(), m_cfg.fileMode.c_str());
    if (m_outputFile == nullptr) {
      throw std::ios_base::failure("Could not open '" + m_cfg.filePath + "'");
    }
  }
  m_outputFile->cd();

  m_outputTree = new TTree(m_cfg.treeName.c_str(),
                           "TTree from RootPropagationSummaryWriter");
  if (m_outputTree == nullptr) {
    throw std::bad_alloc();
  }

  // Set the branches
  m_outputTree->Branch("event_nr", &m_eventNr);
  m_outputTree->Branch("track_nr", &m_trackNr);

  m_outputTree->Branch("d0", &m_d0);
  m_outputTree->Branch("z0", &m_z0);
  m_outputTree->Branch("phi", &m_phi);
  m_outputTree->Branch("theta", &m_theta);
  m_outputTree->Branch("qOverP", &m_qOverP);
  m_outputTree->Branch("t", &m_t);

  m_outputTree->Branch("eta", &m_eta);
  m_outputTree->Branch("pt", &m_pt);
  m_outputTree->Branch("p", &m_p);

  m_outputTree->Branch("nSensitives", &m_nSensitives);
  m_outputTree->Branch("nMaterials", &m_nMaterials);
  m_outputTree->Branch("nPortals", &m_nPortals);

  m_outputTree->Branch("nAttemptedSteps", &m_nAttemptedSteps);
  m_outputTree->Branch("nRejectedSteps", &m_nRejectedSteps);
  m_outputTree->Branch("nSuccessfulSteps", &m_nSuccessfulSteps);
  m_outputTree->Branch("nReverseSteps", &m_nReverseSteps);
  m_outputTree->Branch("pathLength", &m_pathLength);
  m_outputTree->Branch("absolutePathLength", &m_absolutePathLength);

  m_outputTree->Branch("nRenavigations", &m_nRenavigations);
  m_outputTree->Branch("nVolumeSwitches", &m_nVolumeSwitches);
}

RootPropagationSummaryWriter::~RootPropagationSummaryWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ProcessCode RootPropagationSummaryWriter::finalize() {
  // Write the tree
  m_outputFile->cd();
  m_outputTree->Write();
  /// Close the file if it's yours
  if (m_cfg.rootFile == nullptr) {
    m_outputFile->Close();
  }

  ACTS_VERBOSE("Wrote particles to tree '" << m_cfg.treeName << "' in '"
                                           << m_cfg.filePath << "'");

  return ProcessCode::SUCCESS;
}

ProcessCode RootPropagationSummaryWriter::writeT(
    const AlgorithmContext& context, const PropagationSummaries& summaries) {
  // Exclusive access to the tree while writing
  std::scoped_lock lock(m_writeMutex);

  // Get the event number
  m_eventNr = static_cast<int>(context.eventNumber);

  // Loop over the step vector of each test propagation in this
  for (const auto& [trackNr, summary] : Acts::enumerate(summaries)) {
    // Set the track number
    m_trackNr = static_cast<int>(trackNr);

    // Set the initial trajectory parameters
    const auto& startParameters = summary.startParameters;
    m_d0 = static_cast<float>(startParameters.parameters()[Acts::eBoundLoc0]);
    m_z0 = static_cast<float>(startParameters.parameters()[Acts::eBoundLoc1]);
    m_phi = static_cast<float>(startParameters.parameters()[Acts::eBoundPhi]);
    m_theta =
        static_cast<float>(startParameters.parameters()[Acts::eBoundTheta]);
    m_qOverP =
        static_cast<float>(startParameters.parameters()[Acts::eBoundQOverP]);
    m_t = static_cast<float>(startParameters.parameters()[Acts::eBoundTime]);

    // Derived initial trajectory parameters
    m_eta = static_cast<float>(
        Acts::VectorHelpers::eta(startParameters.direction()));
    m_pt = static_cast<float>(startParameters.transverseMomentum());
    m_p = static_cast<float>(startParameters.absoluteMomentum());

    m_nMaterials = 0;
    m_nSensitives = 0;
    m_nPortals = 0;

    // Loop over the steps & count for the statistics
    std::ranges::for_each(summary.steps, [&](const auto& step) {
      // Check if the step is a sensitive step
      if (step.geoID.sensitive() > 0) {
        m_nSensitives++;
      }
      // Check if the step is a portal step
      if (step.geoID.boundary() > 0) {
        m_nPortals++;
      }

      if (step.surface != nullptr) {
        // Check if the step is a material step
        if (step.surface->surfaceMaterial() != nullptr) {
          m_nMaterials++;
        }
      }
    });

    // Stepper statistics
    m_nAttemptedSteps = summary.statistics.stepping.nAttemptedSteps;
    m_nRejectedSteps = summary.statistics.stepping.nRejectedSteps;
    m_nSuccessfulSteps = summary.statistics.stepping.nSuccessfulSteps;
    m_nReverseSteps = summary.statistics.stepping.nReverseSteps;
    m_pathLength = summary.statistics.stepping.pathLength;
    m_absolutePathLength = summary.statistics.stepping.absolutePathLength;

    // Navigator statistics
    m_nRenavigations = summary.statistics.navigation.nRenavigations;
    m_nVolumeSwitches = summary.statistics.navigation.nVolumeSwitches;

    m_outputTree->Fill();
  }

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
