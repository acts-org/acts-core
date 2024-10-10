// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Utilities/detail/Extendable.hpp"

#include <optional>

namespace Acts {

/// @brief Simple class holding result of propagation call
///
/// @tparam parameters_t Type of final track parameters
/// @tparam result_list  Result pack for additional propagation
///                      quantities
template <typename parameters_t, typename... result_list>
struct PropagatorResult : private detail::Extendable<result_list...> {
  using detail::Extendable<result_list...>::get;
  using detail::Extendable<result_list...>::tuple;

  /// Final track parameters
  std::optional<parameters_t> endParameters = std::nullopt;

  /// Full transport jacobian
  std::optional<BoundMatrix> transportJacobian = std::nullopt;

  /// Number of propagation steps that were carried out
  std::size_t steps = 0;

  /// Signed distance over which the parameters were propagated
  double pathLength = 0.;
};

}  // namespace Acts
