// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
