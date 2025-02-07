// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

namespace Acts::Experimental {

enum class GlobalChiSquareFitterError {
  // ensure all values are non-zero
  AIsNotInvertible = 1,
  DidNotConverge = 2,
  NotEnoughMeasurements = 3,
  UpdatePushedToNewVolume = 4,
  UsedUnreachableMeasurements = 5,
};

std::error_code make_error_code(
    Acts::Experimental::GlobalChiSquareFitterError e);

}  // namespace Acts::Experimental

namespace std {
// register with STL
template <>
struct is_error_code_enum<Acts::Experimental::GlobalChiSquareFitterError>
    : std::true_type {};
}  // namespace std
