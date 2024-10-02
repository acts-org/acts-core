// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Utilities/HitSelector.hpp"

ActsExamples::HitSelector::HitSelector(const Config& config,
                                       Acts::Logging::Level level)
    : IAlgorithm("HitSelector", level), m_cfg(config) {
  m_inputHits.initialize(m_cfg.inputHits);
  m_outputHits.initialize(m_cfg.outputHits);
}

ActsExamples::ProcessCode ActsExamples::HitSelector::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  const auto& hits = m_inputHits(ctx);
  SimHitContainer selectedHits;

  std::copy_if(hits.begin(), hits.end(),
               std::inserter(selectedHits, selectedHits.begin()),
               [&](const auto& hit) { return hit.time() < m_cfg.maxTime; });

  ACTS_DEBUG("selected " << selectedHits.size() << " from " << hits.size()
                         << " hits");

  m_outputHits(ctx, std::move(selectedHits));

  return {};
}
