// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Acts include(s)
#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

// Acts Examples include(s)
#include "ActsExamples/Traccc/Common/Converter.hpp"
#include "ActsExamples/Traccc/Common/TracccChainConfig.hpp"

// Covfie Plugin include(s)
#include "Acts/Plugins/Covfie/FieldConversion.hpp"

// Detray include(s).
#include "detray/core/detector.hpp"

// VecMem include(s).
#include <vecmem/memory/host_memory_resource.hpp>

// System include(s)
#include <memory>
#include <string>

namespace ActsExamples::Traccc::Common {

class TracccChainAlgorithmBase : public IAlgorithm {
 public:
  using DetectorHostType =
      detray::detector<detray::default_metadata, detray::host_container_types>;
  using FieldType = Acts::CovfiePlugin::ConstantField;
  using CellsMapType =
      std::map<Acts::GeometryIdentifier, std::vector<Cluster::Cell>>;

  struct Config {
    std::string inputCells;
    std::string inputMeasurements;
    std::string outputTracks;
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry = nullptr;
    std::shared_ptr<const Acts::ConstantBField> field;
    Acts::GeometryHierarchyMap<DigiComponentsConfig> digitizationConfigs;
    std::shared_ptr<const TracccChainConfig> chainConfig;
  };

  /// Construct the traccc algorithm.
  ///
  /// @param cfg is the algorithm configuration
  /// @param lvl is the logging level
  TracccChainAlgorithmBase(Config cfg, Acts::Logging::Level lvl);

  /// Const access to the config
  const Config& config() const { return m_cfg; }

 protected:
  Config m_cfg;

  ReadDataHandle<CellsMapType> m_inputCells{this, "InputCells"};
  ReadDataHandle<MeasurementContainer> m_inputMeasurements{this,
                                                           "InputMeasurements"};
  WriteDataHandle<ConstTrackContainer> m_outputTracks{this, "OutputTracks"};

  // Memory resource, detector, and converter should be declared in that order
  // to ensure order of destructor call.
  vecmem::host_memory_resource hostMemoryResource;
  const DetectorHostType detector;
  const FieldType field;
  const Converter converter;

 private:
  /// @brief Test if the configuration is valid.
  void TestValidConfig() {
    if (m_cfg.inputCells.empty()) {
      throw std::invalid_argument("Missing input cells");
    }

    if (m_cfg.field == nullptr) {
      throw std::invalid_argument("Missing field");
    }

    if (m_cfg.trackingGeometry == nullptr) {
      throw std::invalid_argument("Missing track geometry");
    }

    if (m_cfg.digitizationConfigs.empty()) {
      throw std::invalid_argument("Missing digitization configuration");
    }
  }
};

}  // namespace ActsExamples::Traccc::Common
