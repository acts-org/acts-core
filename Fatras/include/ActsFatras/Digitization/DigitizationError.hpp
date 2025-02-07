// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

namespace ActsFatras {

enum class DigitizationError {
  // ensure all values are non-zero
  SmearingOutOfRange = 1,
  SmearingError,
  UndefinedSurface,
  MaskingError,
};

std::error_code make_error_code(DigitizationError e);

}  // namespace ActsFatras

namespace std {
// register with STL
template <>
struct is_error_code_enum<ActsFatras::DigitizationError> : std::true_type {};
}  // namespace std
