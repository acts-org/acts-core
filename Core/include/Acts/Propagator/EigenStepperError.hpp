// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

namespace Acts {

enum class EigenStepperError {
  // ensure all values are non-zero
  StepSizeStalled = 1,
  StepInvalid,
  StepSizeAdjustmentFailed,
};

std::error_code make_error_code(EigenStepperError e);

}  // namespace Acts

namespace std {
// register with STL
template <>
struct is_error_code_enum<Acts::EigenStepperError> : std::true_type {};
}  // namespace std
