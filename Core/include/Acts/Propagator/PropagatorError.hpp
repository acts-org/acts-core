// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

namespace Acts {

enum class PropagatorError {
  // ensure all values are non-zero
  Failure = 1,
  StepCountLimitReached,
  NextTargetLimitReached,
};

std::error_code make_error_code(Acts::PropagatorError e);

}  // namespace Acts

namespace std {
// register with STL
template <>
struct is_error_code_enum<Acts::PropagatorError> : std::true_type {};
}  // namespace std
