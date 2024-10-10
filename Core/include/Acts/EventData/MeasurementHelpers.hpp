// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <cstddef>

namespace Acts {

namespace detail {
/// Helper functor for @c visit_measurement. This is the actual functor given
/// to @c template_switch.
/// @tparam I Compile time int value
template <std::size_t I>
struct visit_measurement_callable {
  /// The invoked function. It will perform the head/top-left corner
  /// extraction, and pass thee results to the given lambda.
  /// @tparam L The lambda type
  /// @tparam A The parameter vector type
  /// @tparam B The covariance matrix type
  /// @note No requirements on @c A and @c B are made, to enable a single
  /// overload for both const and non-const matrices/vectors.
  /// @param param The parameter vector
  /// @param cov The covariance matrix
  /// @param lambda The lambda to call with the statically sized subsets
  template <typename L, typename A, typename B>
  auto static constexpr invoke(A&& param, B&& cov, L&& lambda) {
    return lambda(param.template head<I>(), cov.template topLeftCorner<I, I>());
  }
};
}  // namespace detail

/// Dispatch a lambda call on an overallocated parameter vector and covariance
/// matrix, based on a runtime dimension value. Inside the lambda call, the
/// vector and matrix will have fixed dimensions, but will still point back to
/// the originally given overallocated values.
/// @tparam L The lambda type
/// @tparam A The parameter vector type
/// @tparam B The covariance matrix type
/// @note No requirements on @c A and @c B are made, to enable a single
/// overload for both const and non-const matrices/vectors.
/// @param param The parameter vector
/// @param cov The covariance matrix
/// @param dim The actual dimension as a runtime value
/// @param lambda The lambda to call with the statically sized subsets
template <typename L, typename A, typename B>
auto visit_measurement(A&& param, B&& cov, std::size_t dim, L&& lambda) {
  return template_switch<detail::visit_measurement_callable, 1, eBoundSize>(
      dim, param, cov, lambda);
}

/// Dispatch a generic lambda on a measurement dimension. This overload doesn't
/// assume anything about what is needed inside the lambda, it communicates the
/// dimension via an integral constant type
/// @tparam L The generic lambda type to call
/// @param dim The runtime dimension of the measurement
/// @param lambda The generic lambda instance to call
/// @param args Additional arguments passed to @p lambda
/// @return Returns the lambda return value
template <typename L, typename... Args>
auto visit_measurement(std::size_t dim, L&& lambda, Args&&... args) {
  return template_switch_lambda<1, eBoundSize>(dim, lambda,
                                               std::forward<Args>(args)...);
}

}  // namespace Acts
