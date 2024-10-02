// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimVertex.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

class TFile;
class TTree;

namespace ActsExamples {
struct AlgorithmContext;

/// Write out vertices as a flat TTree.
///
/// Each entry in the TTree corresponds to one vertex for optimum writing
/// speed. The event number is part of the written data.
///
/// Safe to use from multiple writer threads. To avoid thread-saftey issues,
/// the writer must be the sole owner of the underlying file. Thus, the
/// output file pointer can not be given from the outside.
class RootVertexWriter final : public WriterT<SimVertexContainer> {
 public:
  struct Config {
    /// Input vertex collection to write.
    std::string inputVertices;
    /// Path to the output file.
    std::string filePath;
    /// Output file access mode.
    std::string fileMode = "RECREATE";
    /// Name of the tree within the output file.
    std::string treeName = "vertices";
  };

  /// Construct the vertex writer.
  ///
  /// @params cfg is the configuration object
  /// @params lvl is the logging level
  RootVertexWriter(const Config& cfg, Acts::Logging::Level lvl);

  /// Ensure underlying file is closed.
  ~RootVertexWriter() override;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// Type-specific write implementation.
  ///
  /// @param[in] ctx is the algorithm context
  /// @param[in] vertices are the vertices to be written
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const SimVertexContainer& vertices) override;

 private:
  Config m_cfg;

  std::mutex m_writeMutex;

  TFile* m_outputFile = nullptr;
  TTree* m_outputTree = nullptr;

  /// Event identifier.
  std::uint32_t m_eventId = 0;
  /// Event-unique particle identifier a.k.a barcode.
  std::vector<std::uint64_t> m_vertexId;
  /// Production process type, i.e. what generated the vertex.
  std::vector<std::uint32_t> m_process;
  /// Production position components in mm.
  std::vector<float> m_vx;
  std::vector<float> m_vy;
  std::vector<float> m_vz;
  std::vector<float> m_vt;
  /// Outgoing particles from the vertex.
  std::vector<std::vector<std::uint64_t>> m_outgoingParticles;
  // Decoded vertex identifier; see Barcode definition for details.
  std::vector<std::uint32_t> m_vertexPrimary;
  std::vector<std::uint32_t> m_vertexSecondary;
  std::vector<std::uint32_t> m_generation;
};

}  // namespace ActsExamples
