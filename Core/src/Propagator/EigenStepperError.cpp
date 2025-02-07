// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Propagator/EigenStepperError.hpp"

#include <string>

namespace {

class EigenStepperErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final { return "EigenStepperError"; }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::EigenStepperError;

    switch (static_cast<EigenStepperError>(c)) {
      case EigenStepperError::StepSizeStalled:
        return "Step size fell below minimum threshold";
      case EigenStepperError::StepInvalid:
        return "Step calculation was invalid";
      case EigenStepperError::StepSizeAdjustmentFailed:
        return "Step size adjustment exceeds maximum trials";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::make_error_code(Acts::EigenStepperError e) {
  static EigenStepperErrorCategory c;
  return {static_cast<int>(e), c};
}
