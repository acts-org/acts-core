// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsFatras/Kernel/detail/SimulationError.hpp"

#include <string>

namespace ActsFatras::detail {
namespace {

// Define a custom error code category derived from std::error_category
class SimulationErrorCategory final : public std::error_category {
 public:
  const char* name() const noexcept final { return "SimulationError"; }
  std::string message(int c) const final {
    switch (static_cast<SimulationError>(c)) {
      case SimulationError::eInvalidInputParticleId:
        return "Input particle id with non-zero generation or sub-particle";
      default:
        return "unknown";
    }
  }
};

const SimulationErrorCategory s_simulatorErrorCategory;

}  // namespace

std::error_code make_error_code(SimulationError e) {
  return {static_cast<int>(e), s_simulatorErrorCategory};
}

}  // namespace ActsFatras::detail
