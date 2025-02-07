// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Podio/PodioUtil.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <mutex>
#include <string>

namespace ActsExamples {

/// Write particles to EDM4hep
///
/// Inpersistent information:
/// - particle ID
/// - process
class EDM4hepParticleWriter final : public WriterT<SimParticleContainer> {
 public:
  struct Config {
    /// Input particles collection to write.
    std::string inputParticles;
    /// Where to place the output file.
    std::string outputPath;
    /// Name of the particle collection in EDM4hep.
    std::string outputParticles = "MCParticles";
  };

  /// Construct the particle writer.
  ///
  /// @params cfg is the configuration object
  /// @params lvl is the logging level
  EDM4hepParticleWriter(const Config& cfg, Acts::Logging::Level lvl);

  ProcessCode finalize() final;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 protected:
  /// Type-specific write implementation.
  ///
  /// @param[in] ctx is the algorithm context
  /// @param[in] particles are the particle to be written
  ProcessCode writeT(const ActsExamples::AlgorithmContext& ctx,
                     const SimParticleContainer& particles) final;

 private:
  Config m_cfg;

  std::mutex m_writeMutex;

  Acts::PodioUtil::ROOTWriter m_writer;
};

}  // namespace ActsExamples
