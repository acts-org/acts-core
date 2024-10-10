// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Propagator/detail/SteppingLogger.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsExamples/Propagation/PropagationAlgorithm.hpp"

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

class TFile;
class TTree;

namespace ActsExamples {
struct AlgorithmContext;

/// @class RootPropagationSummaryWriter
///
/// Write out the steps of test propgations for stepping validation,
/// each step sequence is one entry in the root file for optimised
/// data writing speed.
/// The event number is part of the written data.
///
/// A common file can be provided for the writer to attach his TTree,
/// this is done by setting the Config::rootFile pointer to an existing file
///
/// Safe to use from multiple writer threads - uses a std::mutex lock.
class RootPropagationSummaryWriter : public WriterT<PropagationSummaries> {
 public:
  struct Config {
    /// particle collection to write
    std::string inputSummaryCollection = "propagation_summary";

    /// path of the output file
    std::string filePath = "";
    /// file access mode
    std::string fileMode = "RECREATE";
    /// name of the output tree
    std::string treeName = "propagation_summary";
    /// common root file
    TFile* rootFile = nullptr;
  };

  /// Constructor with
  /// @param cfg configuration struct
  /// @param output logging level
  RootPropagationSummaryWriter(
      const Config& cfg, Acts::Logging::Level level = Acts::Logging::INFO);

  /// Virtual destructor
  ~RootPropagationSummaryWriter() override;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// This implementation holds the actual writing method
  /// and is called by the WriterT<>::write interface
  ///
  /// @param context The Algorithm context with per event information
  /// @param summaries is the data to be written out
  ProcessCode writeT(const AlgorithmContext& context,
                     const PropagationSummaries& summaries) override;

 private:
  /// the configuration object
  Config m_cfg;

  /// protect multi-threaded writes
  std::mutex m_writeMutex;
  /// the output file name
  TFile* m_outputFile = nullptr;
  /// the output tree
  TTree* m_outputTree = nullptr;

  /// event number
  int m_eventNr = 0;
  /// track number
  int m_trackNr = 0;

  /// initial trajectory parameters
  float m_d0 = 0;
  float m_z0 = 0;
  float m_phi = 0;
  float m_theta = 0;
  float m_qOverP = 0;
  float m_t = 0;

  /// derived initial trajectory parameters
  float m_eta = 0;
  float m_pt = 0;
  float m_p = 0;

  /// Propagation summary statstics
  int m_nSensitives = 0;
  int m_nMaterials = 0;
  int m_nPortals = 0;

  // steper statistics
  int m_nSteps = 0;
  int m_nStepTrials = 0;
  int m_pathLength = 0;
};

}  // namespace ActsExamples
