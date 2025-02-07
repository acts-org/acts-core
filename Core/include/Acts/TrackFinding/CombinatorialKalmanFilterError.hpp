// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

namespace Acts {

enum class CombinatorialKalmanFilterError {
  // ensure all values are non-zero
  UpdateFailed = 1,
  SmoothFailed,
  OutputConversionFailed,
  MeasurementSelectionFailed,
  PropagationReachesMaxSteps,
};

std::error_code make_error_code(Acts::CombinatorialKalmanFilterError e);

}  // namespace Acts

namespace std {
// register with STL
template <>
struct is_error_code_enum<Acts::CombinatorialKalmanFilterError>
    : std::true_type {};
}  // namespace std
