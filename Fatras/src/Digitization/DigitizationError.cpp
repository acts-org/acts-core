// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsFatras/Digitization/DigitizationError.hpp"

#include <string>

namespace {

/// Custom error category for digitization errors.
class DigitizationErrorCategory : public std::error_category {
 public:
  /// Return a short descriptive name for the category.
  const char* name() const noexcept final { return "DigitizationError"; }

  /// Return what each enum means in text.
  std::string message(int c) const final {
    using ActsFatras::DigitizationError;

    switch (static_cast<DigitizationError>(c)) {
      case DigitizationError::SmearingOutOfRange:
        return "Smeared out of surface bounds.";
      case DigitizationError::SmearingError:
        return "Smearing error occurred.";
      case DigitizationError::UndefinedSurface:
        return "Surface undefined for this operation.";
      case DigitizationError::MaskingError:
        return "Surface mask could not be applied.";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code ActsFatras::make_error_code(ActsFatras::DigitizationError e) {
  static DigitizationErrorCategory c;
  return {static_cast<int>(e), c};
}
