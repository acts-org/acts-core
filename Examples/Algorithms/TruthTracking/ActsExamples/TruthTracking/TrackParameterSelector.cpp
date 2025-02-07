// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TruthTracking/TrackParameterSelector.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Utilities/AngleHelpers.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"

#include <cmath>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>

ActsExamples::TrackParameterSelector::TrackParameterSelector(
    const Config& config, Acts::Logging::Level level)
    : IAlgorithm("TrackParameterSelector", level), m_cfg(config) {
  if (m_cfg.inputTrackParameters.empty()) {
    throw std::invalid_argument("Missing input track parameters");
  }
  if (m_cfg.outputTrackParameters.empty()) {
    throw std::invalid_argument("Missing output track parameters");
  }

  m_inputTrackParameters.initialize(m_cfg.inputTrackParameters);
  m_outputTrackParameters.initialize(m_cfg.outputTrackParameters);
}

ActsExamples::ProcessCode ActsExamples::TrackParameterSelector::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  // helper functions to select tracks
  auto within = [](double x, double min, double max) {
    return (min <= x) && (x < max);
  };
  auto isValidTrack = [&](const auto& trk) {
    const auto theta = trk.template get<Acts::eBoundTheta>();
    const auto eta = Acts::AngleHelpers::etaFromTheta(theta);
    // define charge selection
    return within(trk.transverseMomentum(), m_cfg.ptMin, m_cfg.ptMax) &&
           within(std::abs(eta), m_cfg.absEtaMin, m_cfg.absEtaMax) &&
           within(eta, m_cfg.etaMin, m_cfg.etaMax) &&
           within(trk.template get<Acts::eBoundPhi>(), m_cfg.phiMin,
                  m_cfg.phiMax) &&
           within(trk.template get<Acts::eBoundLoc0>(), m_cfg.loc0Min,
                  m_cfg.loc0Max) &&
           within(trk.template get<Acts::eBoundLoc1>(), m_cfg.loc1Min,
                  m_cfg.loc1Max) &&
           within(trk.template get<Acts::eBoundTime>(), m_cfg.timeMin,
                  m_cfg.timeMax);
  };

  const auto& inputTrackParameters = m_inputTrackParameters(ctx);
  TrackParametersContainer outputTrackParameters;
  outputTrackParameters.reserve(inputTrackParameters.size());

  // copy selected tracks and record initial track index
  for (std::uint32_t i = 0; i < inputTrackParameters.size(); ++i) {
    const auto& trk = inputTrackParameters[i];
    if (isValidTrack(trk)) {
      outputTrackParameters.push_back(trk);
    }
  }
  outputTrackParameters.shrink_to_fit();

  ACTS_DEBUG("event " << ctx.eventNumber << " selected "
                      << outputTrackParameters.size() << " from "
                      << inputTrackParameters.size()
                      << " tracks in track parameters");

  m_outputTrackParameters(ctx, std::move(outputTrackParameters));

  return ProcessCode::SUCCESS;
}
