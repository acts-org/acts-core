// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/Digitization/MeasurementCreation.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsFatras/Digitization/Channelizer.hpp"
#include "ActsFatras/Digitization/Segmentizer.hpp"
#include "ActsFatras/Digitization/UncorrelatedHitSmearer.hpp"

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace ActsExamples {

/// Algorithm that turns simulated hits into measurements by truth smearing.
class DigitizationAlgorithm final : public IAlgorithm {
 public:
  class Config {
   public:
    /// Input collection of simulated hits.
    std::string inputSimHits = "simhits";
    /// Output measurements collection.
    std::string outputMeasurements = "measurements";
    /// Output cells map (geoID -> collection of cells).
    std::string outputCells = "cells";
    /// Output cluster collection.
    std::string outputClusters = "clusters";
    /// Output collection to map measured hits to contributing particles.
    std::string outputMeasurementParticlesMap = "measurement_particles_map";
    /// Output collection to map measured hits to simulated hits.
    std::string outputMeasurementSimHitsMap = "measurement_simhits_map";
    /// Output collection to map particles to measurements.
    std::string outputParticleMeasurementsMap = "particle_measurements_map";
    /// Output collection to map particles to simulated hits.
    std::string outputSimHitMeasurementsMap = "simhit_measurements_map";

    /// Map of surface by identifier to allow local - to global
    std::unordered_map<Acts::GeometryIdentifier, const Acts::Surface*>
        surfaceByIdentifier;
    /// Random numbers tool.
    std::shared_ptr<const RandomNumbers> randomNumbers = nullptr;
    /// Flag to determine whether cell data should be written to the
    /// `outputCells` collection; if true, writes (rather voluminous) cell data.
    bool doOutputCells = false;
    /// Flag to determine whether or not to run the clusterization; if true,
    /// clusters, measurements, and sim-hit-maps are output.
    bool doClusterization = true;
    /// Do we merge hits or not
    bool doMerge = false;
    /// How close do parameters have to be to consider merged
    double mergeNsigma = 1.0;
    /// Consider clusters that share a corner as merged (8-cell connectivity)
    bool mergeCommonCorner = false;
    /// Energy deposit threshold for accepting a hit
    /// For a generic readout frontend we assume 1000 e/h pairs, in Si each
    /// e/h-pair requiers on average an energy of 3.65 eV (PDG  review 2023,
    /// Table 35.10)
    /// @NOTE The default is set to 0 because this works only well with Geant4
    double minEnergyDeposit = 0.0;  // 1000 * 3.65 * Acts::UnitConstants::eV;
    /// The digitizers per GeometryIdentifiers
    Acts::GeometryHierarchyMap<DigiComponentsConfig> digitizationConfigs;
  };

  /// Construct the smearing algorithm.
  ///
  /// @param config is the algorithm configuration
  /// @param level is the logging level
  DigitizationAlgorithm(Config config, Acts::Logging::Level level);

  /// Build measurement from simulation hits at input.
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const override;

  /// Get const access to the config
  const Config& config() const { return m_cfg; }

 private:
  /// Helper method for creating digitized parameters from clusters
  ///
  /// @todo ADD random smearing
  /// @param geoCfg is the geometric digitization configuration
  /// @param channels are the input channels
  /// @param rng the Random number engine for the charge generation smearing
  ///
  /// @return the list of digitized parameters
  DigitizedParameters localParameters(
      const GeometricConfig& geoCfg,
      const std::vector<ActsFatras::Segmentizer::ChannelSegment>& channels,
      RandomEngine& rng) const;

  /// Nested smearer struct that holds geometric digitizer and smearing
  /// Support up to 4 dimensions.
  template <std::size_t kSmearDIM>
  struct CombinedDigitizer {
    GeometricConfig geometric;
    ActsFatras::BoundParametersSmearer<RandomEngine, kSmearDIM> smearing;
  };

  // Support max 4 digitization dimensions - either digital or smeared
  using Digitizer = std::variant<CombinedDigitizer<0>, CombinedDigitizer<1>,
                                 CombinedDigitizer<2>, CombinedDigitizer<3>,
                                 CombinedDigitizer<4>>;

  /// Configuration of the Algorithm
  Config m_cfg;
  /// Digitizers within geometry hierarchy
  Acts::GeometryHierarchyMap<Digitizer> m_digitizers;
  /// Geometric digitizer
  ActsFatras::Channelizer m_channelizer;

  using CellsMap =
      std::map<Acts::GeometryIdentifier, std::vector<Cluster::Cell>>;

  ReadDataHandle<SimHitContainer> m_inputHits{this, "InputHits"};

  WriteDataHandle<MeasurementContainer> m_outputMeasurements{
      this, "OutputMeasurements"};
  WriteDataHandle<CellsMap> m_outputCells{this, "OutputCells"};
  WriteDataHandle<ClusterContainer> m_outputClusters{this, "OutputClusters"};

  WriteDataHandle<IndexMultimap<SimBarcode>> m_outputMeasurementParticlesMap{
      this, "OutputMeasurementParticlesMap"};
  WriteDataHandle<IndexMultimap<Index>> m_outputMeasurementSimHitsMap{
      this, "OutputMeasurementSimHitsMap"};

  WriteDataHandle<InverseMultimap<SimBarcode>> m_outputParticleMeasurementsMap{
      this, "OutputParticleMeasurementsMap"};
  WriteDataHandle<InverseMultimap<Index>> m_outputSimHitMeasurementsMap{
      this, "OutputSimHitMeasurementsMap"};

  /// Construct a fixed-size smearer from a configuration.
  ///
  /// It's templated on the smearing dimension given by @tparam kSmearDIM
  ///
  /// @param cfg Is the digitization configuration input
  ///
  /// @return a variant of a Digitizer
  template <std::size_t kSmearDIM>
  static Digitizer makeDigitizer(const DigiComponentsConfig& cfg) {
    CombinedDigitizer<kSmearDIM> impl;
    // Copy the geometric configuration
    impl.geometric = cfg.geometricDigiConfig;
    // Prepare the smearing configuration
    for (std::size_t i = 0; i < kSmearDIM; ++i) {
      impl.smearing.indices[i] = cfg.smearingDigiConfig.at(i).index;
      impl.smearing.smearFunctions[i] =
          cfg.smearingDigiConfig.at(i).smearFunction;
    }
    return impl;
  }
};

}  // namespace ActsExamples
