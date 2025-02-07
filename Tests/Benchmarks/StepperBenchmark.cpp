// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Propagator/AtlasStepper.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Propagator/SympyStepper.hpp"

#include <memory>

#include "StepperBenchmarkCommons.hpp"

using namespace Acts;
using namespace Acts::Test;

using Stepper = EigenStepper<>;

int main(int argc, char* argv[]) {
  BenchmarkStepper benchmark;
  if (auto ret = benchmark.parseOptions(argc, argv)) {
    return *ret;
  }
  std::shared_ptr<MagneticFieldProvider> bField = benchmark.makeField();
  AtlasStepper atlasStepper(bField);
  benchmark.run(atlasStepper, "AtlasStepper");
  EigenStepper eigenStepper(bField);
  benchmark.run(eigenStepper, "EigenStepper");
  StraightLineStepper straightLineStepper;
  benchmark.run(straightLineStepper, "StraightLineStepper");
  SympyStepper sympyStepper(bField);
  benchmark.run(sympyStepper, "SympyStepper");
  return 0;
}
