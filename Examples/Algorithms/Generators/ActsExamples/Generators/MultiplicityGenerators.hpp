// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/Generators/EventGenerator.hpp"

#include <random>

namespace ActsExamples {

struct FixedMultiplicityGenerator
    : public EventGenerator::MultiplicityGenerator {
  std::size_t n = 1;

  explicit FixedMultiplicityGenerator(std::size_t _n) : n{_n} {}
  FixedMultiplicityGenerator() = default;

  std::size_t operator()(RandomEngine& /*rng*/) const override { return n; }
};

struct PoissonMultiplicityGenerator
    : public EventGenerator::MultiplicityGenerator {
  double mean = 1;
  explicit PoissonMultiplicityGenerator(double _mean) : mean{_mean} {}
  PoissonMultiplicityGenerator() = default;

  std::size_t operator()(RandomEngine& rng) const override {
    return (0 < mean) ? std::poisson_distribution<std::size_t>(mean)(rng) : 0;
  }
};

}  // namespace ActsExamples
