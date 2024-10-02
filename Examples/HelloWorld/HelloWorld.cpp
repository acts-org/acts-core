// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/Framework/Sequencer.hpp"

#include <cstdlib>
#include <memory>

#include "HelloLoggerAlgorithm.hpp"
#include "HelloRandomAlgorithm.hpp"
#include "HelloWhiteBoardAlgorithm.hpp"

int main(void) {
  Acts::Logging::Level logLevel = Acts::Logging::INFO;

  // setup basic tools shared among algorithms
  auto rnd = std::make_shared<ActsExamples::RandomNumbers>(
      ActsExamples::RandomNumbers::Config{});

  // setup the sequencer first w/ config derived from options
  ActsExamples::Sequencer::Config seqCfg;
  seqCfg.events = 10;
  seqCfg.numThreads = -1;
  ActsExamples::Sequencer sequencer(seqCfg);

  // add HelloWorld algorithm that does nothing
  sequencer.addAlgorithm(
      std::make_shared<ActsExamples::HelloLoggerAlgorithm>(logLevel));

  // add HelloRandom algorithm that uses RandomNumbers to generate some
  // random numbers from various distributions.
  ActsExamples::HelloRandomAlgorithm::Config rndCfg;
  rndCfg.randomNumbers = rnd;
  rndCfg.gaussParameters = {{0., 2.5}};
  rndCfg.uniformParameters = {{-1.23, 4.25}};
  rndCfg.gammaParameters = {{1., 1.}};
  rndCfg.drawsPerEvent = 5000;
  rndCfg.output = "random_data";
  sequencer.addAlgorithm(
      std::make_shared<ActsExamples::HelloRandomAlgorithm>(rndCfg, logLevel));

  // add HelloWhiteBoardAlgorithm the reads/writes data from/to the event store
  ActsExamples::HelloWhiteBoardAlgorithm::Config wbCfg;
  // use data from previous algorithm as input
  wbCfg.input = rndCfg.output;
  wbCfg.output = "copied_data";
  sequencer.addAlgorithm(
      std::make_shared<ActsExamples::HelloWhiteBoardAlgorithm>(wbCfg,
                                                               logLevel));

  // Run all configured algorithms and return the appropriate status.
  return sequencer.run();
}
