// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/MagneticField/MagneticFieldError.hpp"

#include <string>

namespace {

class MagneticFieldErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final { return "MagneticFieldError"; }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::MagneticFieldError;

    switch (static_cast<MagneticFieldError>(c)) {
      case MagneticFieldError::OutOfBounds:
        return "Interpolation out of bounds was requested";
      case MagneticFieldError::NotImplemented:
        return "The requested functionality is not implemented";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::make_error_code(Acts::MagneticFieldError e) {
  static MagneticFieldErrorCategory c;
  return {static_cast<int>(e), c};
}
