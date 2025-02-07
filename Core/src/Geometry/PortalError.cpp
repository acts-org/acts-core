// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/SurfaceError.hpp"

#include <string>

namespace {

class SurfaceErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final { return "SurfaceError"; }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::SurfaceError;

    switch (static_cast<SurfaceError>(c)) {
      case SurfaceError::GlobalPositionNotOnSurface:
        return "Global to local transformation failed: position not on "
               "surface.";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::make_error_code(Acts::SurfaceError e) {
  static SurfaceErrorCategory c;
  return {static_cast<int>(e), c};
}
