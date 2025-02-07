// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <system_error>
#include <type_traits>

namespace Acts {

enum class VertexingError {
  // ensure all values are non-zero
  NumericFailure = 1,
  EmptyInput,
  SeedingError,
  NotConverged,
  ElementNotFound,
  NoCovariance,
  SingularMatrix,
  NonPositiveVariance,
  MatrixNotPositiveDefinite,
  InvalidInput,
};

std::error_code make_error_code(Acts::VertexingError e);

}  // namespace Acts

namespace std {
// register with STL
template <>
struct is_error_code_enum<Acts::VertexingError> : std::true_type {};
}  // namespace std
