// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

namespace Acts {

enum class MagneticFieldError {
  OutOfBounds = 1,
  NotImplemented = 2,
};

std::error_code make_error_code(Acts::MagneticFieldError e);

}  // namespace Acts

namespace std {
// register with STL
template <>
struct is_error_code_enum<Acts::MagneticFieldError> : std::true_type {};
}  // namespace std
