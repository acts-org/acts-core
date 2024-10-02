// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Propagator/detail/SteppingLogger.hpp"
#include "ActsExamples/EventData/PropagationSummary.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsExamples/Utilities/Paths.hpp"

#include <fstream>

namespace ActsExamples {

/// @class ObjPropagationStepsWriter
///
/// Write out the steps of test propgations for stepping validation
/// Writes one file per event with form:
///
///     event000000001-propagation-steps.obj
///     event000000002-propagation-steps.obj
///
/// One Thread per write call and hence thread safe
class ObjPropagationStepsWriter : public WriterT<PropagationSummaries> {
 public:
  struct Config {
    /// which collection to write
    std::string collection;
    /// where to place output files
    std::string outputDir;
    /// scale output values
    double outputScalor = 1.0;
    /// floating point precision
    std::size_t outputPrecision = 6;
  };

  /// Constructor with arguments
  ///
  /// @param cfg configuration struct
  /// @param level Output logging level
  ObjPropagationStepsWriter(const Config& cfg,
                            Acts::Logging::Level level = Acts::Logging::INFO);

  /// Virtual destructor
  ~ObjPropagationStepsWriter() override = default;

  /// End-of-run hook
  ProcessCode finalize() override { return ActsExamples::ProcessCode::SUCCESS; }

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  /// Internal configuration representation
  Config m_cfg;

 protected:
  /// This implementation holds the actual writing method
  /// and is called by the WriterT<>::write interface
  ProcessCode writeT(const AlgorithmContext& context,
                     const PropagationSummaries& summaries) final;
};

}  // namespace ActsExamples
