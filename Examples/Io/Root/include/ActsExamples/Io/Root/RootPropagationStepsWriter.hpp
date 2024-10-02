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
#include "ActsExamples/EventData/PropagationSummary.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

class TFile;
class TTree;

namespace ActsExamples {
struct AlgorithmContext;

/// @class RootPropagationStepsWriter
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
class RootPropagationStepsWriter : public WriterT<PropagationSummaries> {
 public:
  struct Config {
    /// particle collection to write
    std::string collection = "propagation_steps";
    /// path of the output file
    std::string filePath = "";
    /// file access mode
    std::string fileMode = "RECREATE";
    /// name of the output tree
    std::string treeName = "propagation_steps";
    /// common root file
    TFile* rootFile = nullptr;
  };

  /// Constructor with
  /// @param cfg configuration struct
  /// @param output logging level
  RootPropagationStepsWriter(const Config& cfg,
                             Acts::Logging::Level level = Acts::Logging::INFO);

  /// Virtual destructor
  ~RootPropagationStepsWriter() override;

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

  // initial trajectory parameters
  float m_d0 = 0;
  float m_z0 = 0;
  float m_phi = 0;
  float m_theta = 0;
  float m_qOverP = 0;
  float m_t = 0;

  // derived initial trajectory parameters
  float m_eta = 0;
  float m_pt = 0;
  float m_p = 0;

  /// volume identifier
  std::vector<int> m_volumeID;
  /// boundary identifier
  std::vector<int> m_boundaryID;
  /// layer identifier if
  std::vector<int> m_layerID;
  /// surface identifier
  std::vector<int> m_approachID;
  /// surface identifier
  std::vector<int> m_sensitiveID;
  /// surface identifier
  std::vector<int> m_extraID;
  /// flag material if present
  std::vector<int> m_material;
  /// global x
  std::vector<float> m_x;
  /// global y
  std::vector<float> m_y;
  /// global z
  std::vector<float> m_z;
  /// global r
  std::vector<float> m_r;
  /// global direction x
  std::vector<float> m_dx;
  /// global direction y
  std::vector<float> m_dy;
  /// global direction z
  std::vector<float> m_dz;
  /// step type
  std::vector<int> m_step_type;
  /// accuracy
  std::vector<float> m_step_acc;
  /// actor check
  std::vector<float> m_step_act;
  /// aborter
  std::vector<float> m_step_abt;
  /// user
  std::vector<float> m_step_usr;
  /// Number of iterations needed by the stepsize finder (e.g. Runge-Kutta) of
  /// the stepper.
  std::vector<std::size_t> m_nStepTrials;
};

}  // namespace ActsExamples
