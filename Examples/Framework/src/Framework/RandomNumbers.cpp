// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

//
//  RandomNumbers.cpp
//  ActsExamples
//
//  Created by Andreas Salzburger on 17/05/16.
//
//

#include "ActsExamples/Framework/RandomNumbers.hpp"

#include "ActsExamples/Framework/AlgorithmContext.hpp"

ActsExamples::RandomNumbers::RandomNumbers(const Config& cfg) : m_cfg(cfg) {}

ActsExamples::RandomEngine ActsExamples::RandomNumbers::spawnGenerator(
    const AlgorithmContext& context) const {
  return RandomEngine(generateSeed(context));
}

std::uint64_t ActsExamples::RandomNumbers::generateSeed(
    const AlgorithmContext& context) const {
  return m_cfg.seed + context.eventNumber;
}
