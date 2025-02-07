// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/GeoModel/GeoModelConversionError.hpp"

#include <string>

namespace {

class GeoModelConversionErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final { return "GeoModelConversionError"; }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::GeoModelConversionError;

    switch (static_cast<GeoModelConversionError>(c)) {
      case GeoModelConversionError::WrongShapeForConverter:
        return "Wrong shape provided for this converter";
      case GeoModelConversionError::InvalidShapeParameters:
        return "Shape parameters can not be converted to Surface "
               "representation";
      case GeoModelConversionError::UnkownShape:
        return "Unknown Shape provided, no converter available";
      case GeoModelConversionError::MissingLogicalVolume:
        return "No logical volume found for the shape";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::make_error_code(Acts::GeoModelConversionError e) {
  static GeoModelConversionErrorCategory c;
  return {static_cast<int>(e), c};
}
