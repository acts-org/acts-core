// This file is part of the Acts project.
//
// Copyright (C) 2021-2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Propagation/PropagationAlgorithm.hpp"

#include "Acts/Utilities/Enumerate.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Propagation/PropagatorInterface.hpp"

#include <stdexcept>

namespace ActsExamples {

ProcessCode PropagationAlgorithm::execute(
    const AlgorithmContext& context) const {
  // Input : the track parameters
  const auto& inputTrackParameters = m_inputTrackParameters(context);

  ACTS_DEBUG("Propagating " << inputTrackParameters.size()
                            << " input trackparameters");

  // Output : the propagation steps
  std::vector<std::vector<Acts::detail::Step>> propagationSteps;
  propagationSteps.reserve(inputTrackParameters.size());

  // Output (optional): the recorded material
  std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>
      recordedMaterialTracks;
  if (m_cfg.recordMaterialInteractions) {
    recordedMaterialTracks.reserve(inputTrackParameters.size());
  }

  for (const auto [it, trackParameters] :
       Acts::enumerate(inputTrackParameters)) {
    PropagationOutput pOutput = m_cfg.propagatorImpl->execute(
        context, m_cfg, logger(), trackParameters);

    // Position / momentum for the output writing
    Acts::Vector3 position = trackParameters.position(context.geoContext);
    Acts::Vector3 direction = trackParameters.direction();

    // Record the propagator steps
    propagationSteps.push_back(std::move(pOutput.first));
    if (m_cfg.recordMaterialInteractions) {
      // Record the material information
      recordedMaterialTracks.emplace(
          it, std::make_pair(std::make_pair(position, direction),
                             std::move(pOutput.second)));
    }
  }
  // Write the propagation step data to the event store
  m_outputPropagationSteps(context, std::move(propagationSteps));

  // Write the recorded material to the event store
  if (m_cfg.recordMaterialInteractions) {
    m_outputMaterialTracks(context, std::move(recordedMaterialTracks));
  }
  return ProcessCode::SUCCESS;
}

PropagationAlgorithm::PropagationAlgorithm(
    const PropagationAlgorithm::Config& config, Acts::Logging::Level level)
    : IAlgorithm("PropagationAlgorithm", level), m_cfg(config) {
  if (!m_cfg.propagatorImpl) {
    throw std::invalid_argument("Config needs to contain a propagator");
  }
  m_inputTrackParameters.initialize(m_cfg.inputTrackParameters);
  m_outputPropagationSteps.initialize(m_cfg.outputPropagationSteps);
  m_outputMaterialTracks.initialize(m_cfg.outputMaterialTracks);
}

}  // namespace ActsExamples
