// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "HelloWhiteBoardAlgorithm.hpp"

#include "ActsExamples/Framework/WhiteBoard.hpp"

#include "HelloData.hpp"

ActsExamples::HelloWhiteBoardAlgorithm::HelloWhiteBoardAlgorithm(
    const Config& cfg, Acts::Logging::Level level)
    : ActsExamples::IAlgorithm("HelloWhiteBoard", level), m_cfg(cfg) {
  // non-optional config settings must be checked on construction.
  if (m_cfg.input.empty()) {
    throw std::invalid_argument("Missing input collection");
  }
  m_readHandle.initialize(m_cfg.input);
  if (m_cfg.output.empty()) {
    throw std::invalid_argument("Missing output collection");
  }
  m_writeHandle.initialize(m_cfg.output);
}

ActsExamples::ProcessCode ActsExamples::HelloWhiteBoardAlgorithm::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  // event-store is append-only and always returns a const reference.
  ACTS_INFO("Reading HelloDataCollection " << m_cfg.input);
  const auto& in = m_readHandle(ctx);
  ACTS_VERBOSE("Read HelloDataCollection with size " << in.size());

  // create a copy
  HelloDataCollection copy(in);

  // transfer the copy to the event store. this always transfers ownership
  // via r-value reference/ move construction.
  ACTS_INFO("Writing HelloDataCollection " << m_cfg.output);
  m_writeHandle(ctx, std::move(copy));

  return ActsExamples::ProcessCode::SUCCESS;
}
