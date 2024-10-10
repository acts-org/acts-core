// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/MaterialMapping/MaterialValidation.hpp"

#include "ActsExamples/MaterialMapping/IMaterialWriter.hpp"

#include <stdexcept>

namespace ActsExamples {

MaterialValidation::MaterialValidation(const MaterialValidation::Config& cfg,
                                       Acts::Logging::Level level)
    : IAlgorithm("MaterialValidation", level), m_cfg(cfg) {
  // Prepare the I/O collections
  m_outputMaterialTracks.initialize(m_cfg.outputMaterialTracks);
  // Check the configuration - material validater
  if (m_cfg.materialValidater == nullptr) {
    throw std::invalid_argument("Missing material validater.");
  }
  // Check the configuration - random number service
  if (m_cfg.randomNumberSvc == nullptr) {
    throw std::invalid_argument("Missing random number service.");
  }
}

ProcessCode MaterialValidation::execute(const AlgorithmContext& context) const {
  // Create a random number generator
  ActsExamples::RandomEngine rng =
      m_cfg.randomNumberSvc->spawnGenerator(context);

  // Setup random number distributions for some quantities
  std::uniform_real_distribution<double> phiDist(m_cfg.phiRange.first,
                                                 m_cfg.phiRange.second);
  std::uniform_real_distribution<double> etaDist(m_cfg.etaRange.first,
                                                 m_cfg.etaRange.second);

  // The output recorded material track collection
  std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>
      recordedMaterialTracks;

  // Loop over the number of tracks
  for (std::size_t iTrack = 0; iTrack < m_cfg.ntracks; ++iTrack) {
    // Generate a random phi and eta
    Acts::ActsScalar phi = phiDist(rng);
    Acts::ActsScalar eta = etaDist(rng);
    Acts::ActsScalar theta = 2 * std::atan(std::exp(-eta));
    Acts::Vector3 direction(std::cos(phi) * std::sin(theta),
                            std::sin(phi) * std::sin(theta), std::cos(theta));

    // Record the material
    auto rMaterial = m_cfg.materialValidater->recordMaterial(
        context.geoContext, context.magFieldContext, m_cfg.startPosition,
        direction);

    recordedMaterialTracks.emplace_hint(recordedMaterialTracks.end(), iTrack,
                                        rMaterial);
  }

  // Write the mapped and unmapped material tracks to the output
  m_outputMaterialTracks(context, std::move(recordedMaterialTracks));

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
