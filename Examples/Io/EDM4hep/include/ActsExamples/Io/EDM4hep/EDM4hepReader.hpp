// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Plugins/Podio/PodioUtil.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IReader.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <memory>
#include <string>

#include <DD4hep/DetElement.h>
#include <edm4hep/MCParticleCollection.h>
#include <tbb/enumerable_thread_specific.h>

namespace ActsExamples {

namespace DD4hep {
struct DD4hepDetector;
}

/// Read particles from EDM4hep.
///
/// Inpersistent information:
/// - particle ID
/// - process
class EDM4hepReader final : public IReader {
 public:
  struct Config {
    /// Where to read input file from.
    std::string inputPath;
    /// Name of the particle collection in EDM4hep.
    std::string inputParticles = "MCParticles";
    /// Names of the sim hit collections
    std::vector<std::string> inputSimHits{};
    /// Particles at creation
    std::string outputParticlesInitial;
    /// Particles at their endpoints
    std::string outputParticlesFinal;
    /// Particles from the generator
    std::string outputParticlesGenerator;

    /// Output simulated (truth) hits collection.
    std::string outputSimHits;

    /// Directory into which to write graphviz files for particles
    /// Empty string means no output
    std::string graphvizOutput = "";

    /// DD4hep detector for cellID resolution.
    std::shared_ptr<DD4hep::DD4hepDetector> dd4hepDetector;

    /// Tracking geometry for cellID resolution.
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry;

    /// Whether to sort sim hits in time to produce index sequence
    bool sortSimHitsInTime = false;
  };

  using ParentRelationship = std::unordered_map<std::size_t, std::size_t>;

  /// Construct the particle reader.
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  EDM4hepReader(const Config& config, Acts::Logging::Level level);

  std::string name() const final;

  /// Return the available events range.
  std::pair<std::size_t, std::size_t> availableEvents() const final;

  /// Read out data from the input stream.
  ProcessCode read(const ActsExamples::AlgorithmContext& ctx) final;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

  void processChildren(const edm4hep::MCParticle& particle, SimBarcode parentId,
                       SimParticleContainer::sequence_type& particles,
                       ParentRelationship& parentRelationship,
                       std::unordered_map<int, std::size_t>& particleMap,
                       std::size_t& nSecondaryVertices,
                       std::size_t& maxGen) const;

  static void setSubParticleIds(
      const SimParticleContainer::sequence_type::iterator& begin,
      const SimParticleContainer::sequence_type::iterator& end);

 private:
  const Acts::Logger& logger() const { return *m_logger; }

  Config m_cfg;
  std::pair<std::size_t, std::size_t> m_eventsRange;
  std::unique_ptr<const Acts::Logger> m_logger;

  std::unordered_map<unsigned int, const Acts::Surface*> m_surfaceMap;

  tbb::enumerable_thread_specific<Acts::PodioUtil::ROOTReader> m_reader;

  Acts::PodioUtil::ROOTReader& reader();

  WriteDataHandle<SimParticleContainer> m_outputParticlesInitial{
      this, "OutputParticlesInitial"};
  WriteDataHandle<SimParticleContainer> m_outputParticlesFinal{
      this, "OutputParticlesFinal"};
  WriteDataHandle<SimParticleContainer> m_outputParticlesGenerator{
      this, "OutputParticlesGenerator"};

  WriteDataHandle<SimHitContainer> m_outputSimHits{this, "OutputSimHits"};

  void graphviz(std::ostream& os,
                const SimParticleContainer::sequence_type& particles,
                const ParentRelationship& parents) const;
};

}  // namespace ActsExamples
