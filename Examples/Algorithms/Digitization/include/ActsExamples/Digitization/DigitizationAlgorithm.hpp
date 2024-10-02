// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/Digitization/MeasurementCreation.hpp"
#include "ActsExamples/Digitization/SmearingConfig.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsFatras/Digitization/Channelizer.hpp"
#include "ActsFatras/Digitization/Segmentizer.hpp"
#include "ActsFatras/Digitization/UncorrelatedHitSmearer.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace ActsFatras {
class Barcode;
}  // namespace ActsFatras

namespace ActsExamples {
struct AlgorithmContext;

/// Algorithm that turns simulated hits into measurements by truth smearing.
class DigitizationAlgorithm final : public IAlgorithm {
 public:
  /// Construct the smearing algorithm.
  ///
  /// @param config is the algorithm configuration
  /// @param level is the logging level
  DigitizationAlgorithm(DigitizationConfig config, Acts::Logging::Level level);

  /// Build measurement from simulation hits at input.
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const override;

  /// Get const access to the config
  const DigitizationConfig& config() const { return m_cfg; }

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
  DigitizationConfig m_cfg;
  /// Digitizers within geometry hierarchy
  Acts::GeometryHierarchyMap<Digitizer> m_digitizers;
  /// Geometric digtizer
  ActsFatras::Channelizer m_channelizer;

  using CellsMap =
      std::map<Acts::GeometryIdentifier, std::vector<Cluster::Cell>>;

  ReadDataHandle<SimHitContainer> m_simContainerReadHandle{this,
                                                           "SimHitContainer"};

  WriteDataHandle<IndexSourceLinkContainer> m_sourceLinkWriteHandle{
      this, "SourceLinks"};
  WriteDataHandle<MeasurementContainer> m_measurementWriteHandle{
      this, "Measurements"};
  WriteDataHandle<CellsMap> m_cellsWriteHandle{this, "Cells"};
  WriteDataHandle<ClusterContainer> m_clusterWriteHandle{this, "Clusters"};
  WriteDataHandle<IndexMultimap<ActsFatras::Barcode>>
      m_measurementParticlesMapWriteHandle{this, "MeasurementParticlesMap"};
  WriteDataHandle<IndexMultimap<Index>> m_measurementSimHitsMapWriteHandle{
      this, "MeasurementSimHitsMap"};

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
    for (int i = 0; i < static_cast<int>(kSmearDIM); ++i) {
      impl.smearing.indices[i] = cfg.smearingDigiConfig.at(i).index;
      impl.smearing.smearFunctions[i] =
          cfg.smearingDigiConfig.at(i).smearFunction;
    }
    return impl;
  }
};

}  // namespace ActsExamples
