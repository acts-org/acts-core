// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
