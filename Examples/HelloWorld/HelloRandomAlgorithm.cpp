// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "HelloRandomAlgorithm.hpp"

#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <random>

#include "HelloData.hpp"

ActsExamples::HelloRandomAlgorithm::HelloRandomAlgorithm(
    const HelloRandomAlgorithm::Config& cfg, Acts::Logging::Level level)
    : IAlgorithm("HelloRandom", level), m_cfg(cfg) {
  if (!m_cfg.randomNumbers) {
    throw std::invalid_argument("Missing random number service");
  }
  if (m_cfg.output.empty()) {
    throw std::invalid_argument("Missing output collection");
  }
  m_writeHandle.initialize(m_cfg.output);
}

ActsExamples::ProcessCode ActsExamples::HelloRandomAlgorithm::execute(
    const AlgorithmContext& ctx) const {
  ACTS_INFO("Running random number generation");

  // Create the local random number generator
  ActsExamples::RandomEngine rng = m_cfg.randomNumbers->spawnGenerator(ctx);

  // Spawn some random number distributions
  std::normal_distribution<double> gaussDist(m_cfg.gaussParameters[0],
                                             m_cfg.gaussParameters[1]);
  std::uniform_real_distribution<double> uniformDist(
      m_cfg.uniformParameters[0], m_cfg.uniformParameters[1]);
  std::gamma_distribution<double> gammaDist(m_cfg.gammaParameters[0],
                                            m_cfg.gammaParameters[1]);
  std::poisson_distribution<int> poissonDist(m_cfg.poissonParameter);

  ACTS_INFO(m_cfg.drawsPerEvent << " draws per event will be done");

  // generate collection of random numbers
  HelloDataCollection collection;
  for (std::size_t idraw = 0; idraw < m_cfg.drawsPerEvent; ++idraw) {
    double gauss = gaussDist(rng);
    double uniform = uniformDist(rng);
    double gamma = gammaDist(rng);
    int poisson = poissonDist(rng);

    ACTS_VERBOSE("Gauss   : " << gauss);
    ACTS_VERBOSE("Uniform : " << uniform);
    ACTS_VERBOSE("Gamma   : " << gamma);
    ACTS_VERBOSE("Poisson : " << poisson);

    HelloData x{};
    x.x = gauss;
    x.a = uniform;
    x.b = gamma;
    x.t = poisson;
    collection.push_back(x);
  }

  // transfer generated data to the event store.
  m_writeHandle(ctx, std::move(collection));

  return ActsExamples::ProcessCode::SUCCESS;
}
