// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

namespace Acts {

enum class MultiStepperError {
  // ensure all values are non-zero
  ComponentNotOnSurface = 1,
  StateOfMultipleComponentsRequested = 2,
  AverageTrackLeftCurrentVolume = 3,
  AllComponentsSteppingError = 4,
  AllComponentsConversionToBoundFailed = 5,
  SomeComponentsConversionToBoundFailed = 6
};

std::error_code make_error_code(Acts::MultiStepperError e);

}  // namespace Acts

// register with STL
namespace std {
template <>
struct is_error_code_enum<Acts::MultiStepperError> : std::true_type {};
}  // namespace std
