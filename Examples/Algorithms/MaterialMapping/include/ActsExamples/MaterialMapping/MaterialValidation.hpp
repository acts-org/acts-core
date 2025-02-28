// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/MaterialValidater.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/MaterialMapping/IMaterialWriter.hpp"

#include <numbers>

namespace ActsExamples {

/// @class MaterialValidation
///
class MaterialValidation : public IAlgorithm {
 public:
  /// @class nested Config class
  /// of the MaterialMapping algorithm
  struct Config {
    /// Number of tracks per event
    std::size_t ntracks = 1000;

    /// Start position for the scan
    Acts::Vector3 startPosition = Acts::Vector3(0., 0., 0.);

    /// Start direction for the scan: phi
    std::pair<double, double> phiRange = {-std::numbers::pi, std::numbers::pi};

    /// Start direction for the scan: eta
    std::pair<double, double> etaRange = {-4., 4.};

    /// Random number service
    std::shared_ptr<RandomNumbers> randomNumberSvc = nullptr;

    // The validater
    std::shared_ptr<Acts::MaterialValidater> materialValidater = nullptr;

    /// Output collection name
    std::string outputMaterialTracks = "material_tracks";
  };

  /// Constructor
  ///
  /// @param cfg The configuration struct carrying the used tools
  /// @param level The output logging level
  explicit MaterialValidation(const Config& cfg,
                              Acts::Logging::Level level = Acts::Logging::INFO);

  /// Destructor
  /// - it also writes out the file
  ~MaterialValidation() override = default;

  /// Framework execute method
  ///
  /// @param context The algorithm context for event consistency
  ActsExamples::ProcessCode execute(
      const AlgorithmContext& context) const override;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;  //!< internal config object

  WriteDataHandle<std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>>
      m_outputMaterialTracks{this, "OutputMaterialTracks"};
};

}  // namespace ActsExamples
