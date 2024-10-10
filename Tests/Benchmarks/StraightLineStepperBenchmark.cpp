// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Propagator/StraightLineStepper.hpp"

#include "StepperBenchmarkCommons.hpp"

using namespace Acts;
using namespace Acts::Test;

using Stepper = StraightLineStepper;

int main(int argc, char* argv[]) {
  BenchmarkStepper benchmark;
  if (auto ret = benchmark.parseOptions(argc, argv)) {
    return *ret;
  }
  Stepper stepper;
  benchmark.run(stepper, "StraightLineStepper");
  return 0;
}
