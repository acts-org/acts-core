// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

namespace Acts {

enum class PortalError {
  // ensure all values are non-zero
  PositionNotOnAnyChildPortalLink = 1,
};

std::error_code make_error_code(Acts::PortalError e);

}  // namespace Acts

namespace std {
// register with STL
template <>
struct is_error_code_enum<Acts::PortalError> : std::true_type {};
}  // namespace std
