// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Traccc plugin include(s)
#include "ActsExamples/Traccc/Common/TracccChainAlgorithmBase.hpp"

// Acts include(s)
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"

// Detray include(s).
#include "detray/core/detector.hpp"
#include "detray/io/frontend/detector_reader.hpp"

// VecMem include(s).
#include <vecmem/memory/memory_resource.hpp>

// System include(s)
#include <memory>
#include <string>

namespace {

// Temporarily used to get the corresponding detray detector for the Acts
// geometry. Will be replaced when the detray plugin containing geometry
// conversion is complete.
template <typename detector_t>
inline auto readDetector(vecmem::memory_resource* mr,
                         const std::string& detectorFilePath,
                         const std::string& materialFilePath = "",
                         const std::string& gridFilePath = "") {
  // Set up the detector reader configuration.
  detray::io::detector_reader_config cfg;
  cfg.add_file(detectorFilePath);
  if (!materialFilePath.empty()) {
    cfg.add_file(materialFilePath);
  }
  if (!gridFilePath.empty()) {
    cfg.add_file(gridFilePath);
  }

  // Read the detector.
  auto [det, names] = detray::io::read_detector<detector_t>(*mr, cfg);
  return std::move(det);
}

}  // namespace

/// Construct the traccc algorithm.
///
/// @param cfg is the algorithm configuration
/// @param lvl is the logging level
ActsExamples::Traccc::Common::TracccChainAlgorithmBase::
    TracccChainAlgorithmBase(Config cfg, Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("TracccChainAlgorithm", lvl),
      m_cfg(std::move(cfg)),
      detector((TestValidConfig(), readDetector<DetectorHostType>(
                                       &hostMemoryResource,
                                       "/home/frederik/Desktop/CERN-TECH/input/"
                                       "odd-detray_geometry_detray.json"))),
      field(Acts::CovfiePlugin::covfieField(*m_cfg.field)),
      converter{*m_cfg.trackingGeometry,
                detector,
                Conversion::tracccConfig(m_cfg.digitizationConfigs),
                traccc::io::alt_read_geometry(detector),
                Acts::TracccPlugin::createBarcodeMap(detector),
                logger()} {
  m_inputCells.initialize(m_cfg.inputCells);
  m_inputMeasurements.initialize(m_cfg.inputMeasurements);
  m_outputTracks.initialize(m_cfg.outputTracks);
}
