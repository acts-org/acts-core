// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
