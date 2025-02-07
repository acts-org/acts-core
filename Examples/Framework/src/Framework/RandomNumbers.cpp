// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Framework/RandomNumbers.hpp"

#include "ActsExamples/Framework/AlgorithmContext.hpp"

namespace ActsExamples {

RandomNumbers::RandomNumbers(const Config& cfg) : m_cfg(cfg) {}

RandomEngine RandomNumbers::spawnGenerator(
    const AlgorithmContext& context) const {
  return RandomEngine(generateSeed(context));
}

RandomSeed RandomNumbers::generateSeed(const AlgorithmContext& context) const {
  return m_cfg.seed + context.eventNumber;
}

}  // namespace ActsExamples
