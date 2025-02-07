// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Propagator/PropagatorError.hpp"

#include <string>

namespace {

class PropagatorErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final { return "PropagatorError"; }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::PropagatorError;

    switch (static_cast<PropagatorError>(c)) {
      case PropagatorError::Failure:
        return "Propagation failed";
      case PropagatorError::StepCountLimitReached:
        return "Propagation reached the configured maximum number of steps";
      case PropagatorError::NextTargetLimitReached:
        return "Propagation reached the configured maximum number of next "
               "target calls";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::make_error_code(Acts::PropagatorError e) {
  static PropagatorErrorCategory c;
  return {static_cast<int>(e), c};
}
