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
