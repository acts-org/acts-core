// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/PortalError.hpp"

#include <string>

namespace {

class PortalErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final { return "PortalError"; }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::PortalError;

    switch (static_cast<PortalError>(c)) {
      case PortalError::PositionNotOnAnyChildPortalLink:
        return "Position not on any of the composite child portal links";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::make_error_code(Acts::PortalError e) {
  static PortalErrorCategory c;
  return {static_cast<int>(e), c};
}
