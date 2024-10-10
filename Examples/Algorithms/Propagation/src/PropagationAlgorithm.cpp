// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Propagation/PropagationAlgorithm.hpp"

#include "Acts/Utilities/Enumerate.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Propagation/PropagatorInterface.hpp"

#include <stdexcept>
#include <utility>

namespace ActsExamples {

PropagationAlgorithm::PropagationAlgorithm(
    const PropagationAlgorithm::Config& config, Acts::Logging::Level level)
    : IAlgorithm("PropagationAlgorithm", level), m_cfg(config) {
  if (!m_cfg.propagatorImpl) {
    throw std::invalid_argument("Config needs to contain a propagator");
  }
  m_inputTrackParameters.initialize(m_cfg.inputTrackParameters);
  m_outputSummary.initialize(m_cfg.outputSummaryCollection);
  m_outputMaterialTracks.initialize(m_cfg.outputMaterialCollection);
}

ProcessCode PropagationAlgorithm::execute(
    const AlgorithmContext& context) const {
  // Input : the track parameters
  const auto& inputTrackParameters = m_inputTrackParameters(context);

  ACTS_DEBUG("Propagating " << inputTrackParameters.size()
                            << " input trackparameters");

  // Output : the propagation steps
  PropagationSummaries propagationSummaries;
  propagationSummaries.reserve(inputTrackParameters.size());

  // Output (optional): the recorded material
  std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>
      recordedMaterialTracks;
  if (m_cfg.recordMaterialInteractions) {
    recordedMaterialTracks.reserve(inputTrackParameters.size());
  }

  for (const auto [it, parameters] : Acts::enumerate(inputTrackParameters)) {
    // In case covariance transport is not desired, it has to be stripped
    // off the input parameters
    auto propagationResult =
        m_cfg.covarianceTransport
            ? m_cfg.propagatorImpl->execute(context, m_cfg, logger(),
                                            parameters)
            : m_cfg.propagatorImpl->execute(
                  context, m_cfg, logger(),
                  TrackParameters(parameters.referenceSurface().getSharedPtr(),
                                  parameters.parameters(), std::nullopt,
                                  parameters.particleHypothesis()));
    if (!propagationResult.ok()) {
      ACTS_ERROR("Propagation failed with " << propagationResult.error());
      continue;
    }

    PropagationOutput& propagationOutput = propagationResult.value();

    // Position / momentum for the output writing
    Acts::Vector3 position = parameters.position(context.geoContext);
    Acts::Vector3 direction = parameters.direction();

    // Record the propagator steps
    propagationSummaries.push_back(std::move(propagationOutput.first));

    if (m_cfg.recordMaterialInteractions) {
      // Record the material information
      recordedMaterialTracks.emplace(
          it, std::make_pair(std::make_pair(position, direction),
                             std::move(propagationOutput.second)));
    }
  }
  // Write the propagation step data to the event store
  m_outputSummary(context, std::move(propagationSummaries));

  // Write the recorded material to the event store
  if (m_cfg.recordMaterialInteractions) {
    m_outputMaterialTracks(context, std::move(recordedMaterialTracks));
  }
  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
