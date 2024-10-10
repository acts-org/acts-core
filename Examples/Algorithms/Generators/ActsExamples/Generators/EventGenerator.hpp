// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimVertex.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IReader.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ActsExamples {
struct AlgorithmContext;

/// Event generator based on separate particles and vertex generators.
///
/// This must be a reader and not just an algorithm since it might read in
/// pre-computed generator samples, e.g. via HepMC, and therefore has an
/// internal state that will be modified.
class EventGenerator final : public ActsExamples::IReader {
 public:
  /// Combined set of generator functions.
  ///
  /// Each generator creates a number of primary vertices (multiplicity),
  /// each with an separate vertex position and time (vertex), and a set of
  /// associated particles grouped into secondary vertices (process) anchored
  /// at the primary vertex position. The first group of particles generated
  /// by the process are the particles associated directly to the primary
  /// vertex.
  ///
  /// The process generator is responsible for defining all components of the
  /// particle barcode except the primary vertex. The primary vertex will be
  /// set/overwritten by the event generator.

  /// @brief Generator interface for event multiplicity of vertices
  struct MultiplicityGenerator {
    /// @brief Virtual destructor required
    virtual ~MultiplicityGenerator() = default;
    /// @brief Generate the multiplicity of vertices
    ///
    /// @param rng Shared random number generator instance
    /// @return std::size_t The multiplicity for the event
    virtual std::size_t operator()(RandomEngine& rng) const = 0;
  };

  /// @brief Generator interface for a vertex position
  struct PrimaryVertexPositionGenerator {
    /// @brief Virtual destructor required
    virtual ~PrimaryVertexPositionGenerator() = default;
    /// @brief Generate a vertex position
    ///
    /// @param rng Shared random number generator instance
    /// @return Acts::Vector4 The vertex position
    virtual Acts::Vector4 operator()(RandomEngine& rng) const = 0;
  };

  /// @brief Generator interface for vertices and particles
  struct ParticlesGenerator {
    /// @brief Virtual destructor required
    virtual ~ParticlesGenerator() = default;
    /// @brief Generate vertices and particles for one interaction
    /// @note This method cannot be `const` because the Pythia8 generator
    ///       uses the Pythia8 interfaces, which is non-const
    ///
    /// @param rng Shared random number generator instance
    /// @return The vertex and particle containers
    virtual std::pair<SimVertexContainer, SimParticleContainer> operator()(
        RandomEngine& rng) = 0;
  };

  /// @brief Combined struct which contains all generator components
  struct Generator {
    std::shared_ptr<MultiplicityGenerator> multiplicity = nullptr;
    std::shared_ptr<PrimaryVertexPositionGenerator> vertex = nullptr;
    std::shared_ptr<ParticlesGenerator> particles = nullptr;
  };

  struct Config {
    /// Name of the output particles collection.
    std::string outputParticles;
    /// Name of the vertex collection.
    std::string outputVertices;
    /// List of generators that should be used to generate the event.
    std::vector<Generator> generators;
    /// The random number service.
    std::shared_ptr<const RandomNumbers> randomNumbers;
  };

  EventGenerator(const Config& cfg, Acts::Logging::Level lvl);

  /// Name of the reader.
  std::string name() const final;
  /// Available events range. Always return
  /// [0,std::numeric_limits<std::size_t>::max()) since we generate them.
  std::pair<std::size_t, std::size_t> availableEvents() const final;
  /// Generate an event.
  ProcessCode read(const AlgorithmContext& ctx) final;

  /// Const access to the config
  const Config& config() const { return m_cfg; }

 private:
  const Acts::Logger& logger() const { return *m_logger; }

  Config m_cfg;
  std::unique_ptr<const Acts::Logger> m_logger;

  WriteDataHandle<SimParticleContainer> m_outputParticles{this,
                                                          "OutputParticles"};
  WriteDataHandle<SimVertexContainer> m_outputVertices{this, "OutputVertices"};
};

}  // namespace ActsExamples
