// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootMaterialTrackReader.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Io/Root/RootUtility.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>

#include <TChain.h>
#include <TTree.h>

namespace ActsExamples {

RootMaterialTrackReader::RootMaterialTrackReader(const Config& config,
                                                 Acts::Logging::Level level)
    : IReader(),
      m_logger{Acts::getDefaultLogger(name(), level)},
      m_cfg(config),
      m_rootMaterialTrack(config) {
  if (m_cfg.fileList.empty()) {
    throw std::invalid_argument{"No input files given"};
  }

  m_inputChain = std::make_unique<TChain>(m_cfg.treeName.c_str());

  // loop over the input files
  for (const auto& inputFile : m_cfg.fileList) {
    // add file to the input chain
    m_inputChain->Add(inputFile.c_str());
    ACTS_DEBUG("Adding File " << inputFile << " to tree '" << m_cfg.treeName
                              << "'.");
  }

  // get the number of entries, which also loads the tree
  std::size_t nentries = m_inputChain->GetEntries();

  m_rootMaterialTrack.initializeRead(*m_inputChain);

  m_events = static_cast<std::size_t>(m_inputChain->GetMaximum("event_id") + 1);
  m_batchSize = nentries / m_events;
  ACTS_DEBUG("The full chain has "
             << nentries << " entries for " << m_events
             << " events this corresponds to a batch size of: " << m_batchSize);

  // Sort the entry numbers of the events
  {
    // necessary to guarantee that m_inputChain->GetV1() is valid for the
    // entire range
    m_inputChain->SetEstimate(nentries + 1);

    m_entryNumbers.resize(nentries);
    m_inputChain->Draw("event_id", "", "goff");
    RootUtility::stableSort(m_inputChain->GetEntries(), m_inputChain->GetV1(),
                            m_entryNumbers.data(), false);
  }

  m_outputMaterialTracks.initialize(m_cfg.outputMaterialTracks);
}

RootMaterialTrackReader::~RootMaterialTrackReader() {}

std::string RootMaterialTrackReader::name() const {
  return "RootMaterialTrackReader";
}

std::pair<std::size_t, std::size_t> RootMaterialTrackReader::availableEvents()
    const {
  return {0u, m_events};
}

ProcessCode RootMaterialTrackReader::read(const AlgorithmContext& context) {
  ACTS_DEBUG("Trying to read recorded material from tracks.");

  if (m_inputChain == nullptr || context.eventNumber >= m_events) {
    return ProcessCode::SUCCESS;
  }

  // lock the mutex
  std::lock_guard<std::mutex> lock(m_read_mutex);
  // now read

  // The collection to be written
  std::unordered_map<std::size_t, Acts::RecordedMaterialTrack> mtrackCollection;

  // Loop over the entries for this event
  for (std::size_t ib = 0; ib < m_batchSize; ++ib) {
    // Read the correct entry: startEntry + ib
    auto entry = m_batchSize * context.eventNumber + ib;
    entry = m_entryNumbers.at(entry);
    ACTS_VERBOSE("Reading event: " << context.eventNumber
                                   << " with stored entry: " << entry);
    m_inputChain->GetEntry(entry);

    auto rmTrack = m_rootMaterialTrack.read();

    mtrackCollection[ib] = (std::move(rmTrack));
  }

  // Write to the collection to the EventStore
  m_outputMaterialTracks(context, std::move(mtrackCollection));

  // Return success flag
  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
