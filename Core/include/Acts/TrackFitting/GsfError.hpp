// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

namespace Acts {

enum class GsfError {
  StartParametersHaveNoCovariance,
  NoMeasurementStatesCreatedForward,
  NoMeasurementStatesCreatedBackward,
  NoMeasurementStatesCreatedFinal,
};

std::error_code make_error_code(GsfError e);

}  // namespace Acts

// register with STL
namespace std {
template <>
struct is_error_code_enum<Acts::GsfError> : std::true_type {};
}  // namespace std
