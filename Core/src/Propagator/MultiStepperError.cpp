// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Propagator/MultiStepperError.hpp"

#include <string>

namespace {

class MultiStepperErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final { return "MultiStepperError"; }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::MultiStepperError;

    switch (static_cast<MultiStepperError>(c)) {
      case MultiStepperError::ComponentNotOnSurface:
        return "Component is not on a surface";
      case MultiStepperError::StateOfMultipleComponentsRequested:
        return "The global BoundState/CurvilinearState can only be computed if "
               "only one component exists";
      case MultiStepperError::AverageTrackLeftCurrentVolume:
        return "The average track has left the current volume";
      case MultiStepperError::AllComponentsSteppingError:
        return "Stepping error occurred in all components";
      case MultiStepperError::AllComponentsConversionToBoundFailed:
        return "The conversion to the bound state failed for all components";
      case MultiStepperError::SomeComponentsConversionToBoundFailed:
        return "The conversion to the bound state failed for some components";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::make_error_code(Acts::MultiStepperError e) {
  static MultiStepperErrorCategory c;
  return {static_cast<int>(e), c};
}
