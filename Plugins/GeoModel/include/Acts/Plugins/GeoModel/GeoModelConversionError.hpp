// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

class GeoFullPhysVol;

namespace Acts {

enum class GeoModelConversionError {
  // ensure all values are non-zero
  WrongShapeForConverter = 1,
  InvalidShapeParameters,
  UnkownShape,
  MissingLogicalVolume
};

std::error_code make_error_code(Acts::GeoModelConversionError e);

}  // namespace Acts

namespace std {
// register with STL
template <>
struct is_error_code_enum<Acts::GeoModelConversionError> : std::true_type {};
}  // namespace std
