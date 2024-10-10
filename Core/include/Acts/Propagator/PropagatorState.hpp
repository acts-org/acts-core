// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/detail/Extendable.hpp"

#include <functional>

namespace Acts {

/// @brief Different stages during propagation
enum class PropagatorStage {
  invalid,          ///< Invalid stage
  prePropagation,   ///< Before the propagation
  postPropagation,  ///< After the propagation
  preStep,          ///< Before a step
  postStep,         ///< After a step
};

/// @brief private Propagator state for navigation and debugging
///
/// @tparam propagator_options_t Type of the Objections object
///
/// This struct holds the common state information for propagating
/// which is independent of the actual stepper implementation.
template <typename propagator_options_t, typename stepper_state_t,
          typename navigator_state_t, typename... extension_state_t>
struct PropagatorState : private detail::Extendable<extension_state_t...> {
  using options_type = propagator_options_t;

  /// Create the propagator state from the options
  ///
  /// @tparam propagator_options_t the type of the propagator options
  ///
  /// @param topts The options handed over by the propagate call
  /// @param steppingIn Stepper state instance to begin with
  /// @param navigationIn Navigator state instance to begin with
  PropagatorState(const propagator_options_t& topts, stepper_state_t steppingIn,
                  navigator_state_t navigationIn)
      : options(topts),
        stepping{std::move(steppingIn)},
        navigation{std::move(navigationIn)},
        geoContext(topts.geoContext) {}

  using detail::Extendable<extension_state_t...>::get;
  using detail::Extendable<extension_state_t...>::tuple;

  /// Propagation stage
  PropagatorStage stage = PropagatorStage::invalid;

  /// These are the options - provided for each propagation step
  propagator_options_t options;

  /// Stepper state - internal state of the Stepper
  stepper_state_t stepping;

  /// Navigation state - internal state of the Navigator
  navigator_state_t navigation;

  /// Context object for the geometry
  std::reference_wrapper<const GeometryContext> geoContext;

  /// Number of propagation steps that were carried out
  std::size_t steps = 0;

  /// Signed distance over which the parameters were propagated
  double pathLength = 0.;
};

}  // namespace Acts
