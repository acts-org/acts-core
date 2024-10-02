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

enum class PropagatorError {
  // ensure all values are non-zero
  Failure = 1,
  WrongDirection,
  StepCountLimitReached,
};

std::error_code make_error_code(Acts::PropagatorError e);

}  // namespace Acts

namespace std {
// register with STL
template <>
struct is_error_code_enum<Acts::PropagatorError> : std::true_type {};
}  // namespace std
