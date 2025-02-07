// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Propagator/EigenStepper.hpp"

#include "StepperBenchmarkCommons.hpp"

using namespace Acts;
using namespace Acts::Test;

using Stepper = EigenStepper<>;

int main(int argc, char* argv[]) {
  BenchmarkStepper benchmark;
  if (auto ret = benchmark.parseOptions(argc, argv)) {
    return *ret;
  }
  auto bField = benchmark.makeField();
  Stepper stepper(std::move(bField));
  benchmark.run(stepper, "EigenStepper");
  return 0;
}
