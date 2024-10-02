// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsFatras/EventData/Barcode.hpp"

#include <string>
#include <vector>

namespace ActsExamples {

/// Simple algorithm, that allows to extract a subset of a
/// measurment-particles-map.
/// This allows to conveniently work on subsets of the geometry.
///
class MeasurementMapSelector final : public IAlgorithm {
  using Map = IndexMultimap<ActsFatras::Barcode>;

 public:
  struct Config {
    /// Input spacepoints collection.
    std::string inputMeasurementParticleMap;

    /// Input source links
    std::string inputSourceLinks;

    /// Output protoTracks collection.
    std::string outputMeasurementParticleMap;

    /// What spacepoints to keep
    std::vector<Acts::GeometryIdentifier> geometrySelection;
  };

  /// Constructor of the track finding algorithm
  ///
  /// @param cfg is the config struct to configure the algorithm
  /// @param level is the logging level
  MeasurementMapSelector(Config cfg, Acts::Logging::Level lvl)
      : IAlgorithm("MeasurementMapSelector", lvl), m_cfg(cfg) {
    m_inputSourceLinks.initialize(m_cfg.inputSourceLinks);
    m_inputMap.initialize(m_cfg.inputMeasurementParticleMap);
    m_outputMap.initialize(m_cfg.outputMeasurementParticleMap);
  }

  virtual ~MeasurementMapSelector() = default;

  /// Filter the measurements
  ///
  /// @param ctx is the algorithm context that holds event-wise information
  /// @return a process code to steer the algorithm flow
  ActsExamples::ProcessCode execute(
      const ActsExamples::AlgorithmContext& ctx) const final {
    const auto& inputSourceLinks = m_inputSourceLinks(ctx);
    const auto& inputMap = m_inputMap(ctx);

    Map outputMap;

    for (const auto geoId : m_cfg.geometrySelection) {
      auto range = selectLowestNonZeroGeometryObject(inputSourceLinks, geoId);
      for (const auto& sl : range) {
        const auto [begin, end] = inputMap.equal_range(sl.index());
        outputMap.insert(begin, end);
      }
    }

    m_outputMap(ctx, std::move(outputMap));

    return ProcessCode::SUCCESS;
  }

  const Config& config() const { return m_cfg; }

 private:
  // configuration
  Config m_cfg;

  ReadDataHandle<IndexSourceLinkContainer> m_inputSourceLinks{
      this, "InputSourceLinks"};
  ReadDataHandle<Map> m_inputMap{this, "InputMeasurementParticleMap"};
  WriteDataHandle<Map> m_outputMap{this, "OutputMeasurementParticleMap"};
};

}  // namespace ActsExamples
