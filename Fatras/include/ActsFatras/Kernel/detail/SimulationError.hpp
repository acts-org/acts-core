// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

namespace ActsFatras::detail {

enum class SimulationError {
  // ensure all values are non-zero
  eInvalidInputParticleId = 1,
};

/// Construct and error_code from the enum.
///
/// Must use snake_case naming for STL compatibility.
std::error_code make_error_code(SimulationError e);

}  // namespace ActsFatras::detail

// Register the error enum as STL-compatible.
namespace std {
template <>
struct is_error_code_enum<ActsFatras::detail::SimulationError>
    : std::true_type {};
}  // namespace std
