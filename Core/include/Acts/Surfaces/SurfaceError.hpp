// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

namespace Acts {

enum class SurfaceError {
  // ensure all values are non-zero
  GlobalPositionNotOnSurface = 1,
};

std::error_code make_error_code(Acts::SurfaceError e);

}  // namespace Acts

namespace std {
// register with STL
template <>
struct is_error_code_enum<Acts::SurfaceError> : std::true_type {};
}  // namespace std
