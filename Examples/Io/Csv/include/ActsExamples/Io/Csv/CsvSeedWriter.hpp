// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsFatras/EventData/Barcode.hpp"

using namespace Acts::UnitLiterals;

namespace ActsExamples {

/// @class CsvSeedWriter
///
/// Write out the seed reconstructed by the seeding algorithm in
/// comma-separated-value format.
///
/// This writes one file per event into the configured output directory. By
/// default it writes to the current working directory.
/// Files are named using the following schema
///
///     event000000001-seed.csv
///     event000000002-seed.csv
///
/// and each line in the file corresponds to one seed.
class CsvSeedWriter : public WriterT<TrackParametersContainer> {
 public:
  struct Config {
    /// Input estimated track parameters collection.
    std::string inputTrackParameters;
    /// Input seed collection.
    std::string inputSimSeeds;
    /// Input collection of simulated hits.
    std::string inputSimHits;
    /// Input hit-particles map collection.
    std::string inputMeasurementParticlesMap;
    /// Input collection to map measured hits to simulated hits.
    std::string inputMeasurementSimHitsMap;
    /// output filename.
    std::string fileName = "Seed.csv";
    /// output directory
    std::string outputDir;
  };

  /// Constructor
  ///
  /// @param config Configuration struct
  /// @param level Message level declaration
  CsvSeedWriter(const Config& config,
                Acts::Logging::Level level = Acts::Logging::INFO);

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// @brief Write method called by the base class
  /// @param [in] ctx is the algorithm context for event information
  /// @param [in] trackParams are parameters to write
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const TrackParametersContainer& trackParams) override;

 private:
  Config m_cfg;  ///< The config class

  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
  ReadDataHandle<SimSeedContainer> m_inputSimSeeds{this, "InputSimSeeds"};
  ReadDataHandle<SimHitContainer> m_inputSimHits{this, "InputSimHits"};
  ReadDataHandle<HitParticlesMap> m_inputMeasurementParticlesMap{
      this, "InputMeasurementParticlesMap"};
  ReadDataHandle<HitSimHitsMap> m_inputMeasurementSimHitsMap{
      this, "InputMeasurementSimHitsMap"};

  /// @brief Struct for brief seed summary info
  ///
  struct SeedInfo {
    std::size_t seedID = 0;
    ActsFatras::Barcode particleId;
    float seedPt = -1;
    float seedPhi = 0;
    float seedEta = 0;
    float vertexZ = 0;
    float quality = -1;
    boost::container::small_vector<Acts::Vector3, 3> globalPosition;
    float truthDistance = -1;
    std::string seedType = "unknown";
    ProtoTrack measurementsID;
  };  // trackInfo struct
};

}  // namespace ActsExamples
