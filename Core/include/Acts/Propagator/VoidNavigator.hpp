// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Propagator/NavigatorOptions.hpp"
namespace Acts {

class Surface;

/// @brief The void navigator struct as a default navigator
///
/// It does not provide any navigation action, the compiler
/// should eventually optimise that the function call is not done
///
struct VoidNavigator {
  struct Config {};

  struct Options : public NavigatorPlainOptions {
    void setPlainOptions(const NavigatorPlainOptions& options) {
      static_cast<NavigatorPlainOptions&>(*this) = options;
    }
  };

  /// @brief Nested State struct, minimal requirement
  struct State {
    Options options;
  };

  State makeState(const Options& options) const {
    State state;
    state.options = options;
    return state;
  }

  const Surface* currentSurface(const State& /*state*/) const {
    return nullptr;
  }

  const Surface* startSurface(const State& /*state*/) const { return nullptr; }

  const Surface* targetSurface(const State& /*state*/) const { return nullptr; }

  bool targetReached(const State& /*state*/) const { return false; }

  bool navigationBreak(const State& /*state*/) const { return false; }

  void currentSurface(State& /*state*/, const Surface* /*surface*/) const {}

  void targetReached(State& /*state*/, bool /*targetReached*/) const {}

  void navigationBreak(State& /*state*/, bool /*navigationBreak*/) const {}

  /// Navigation call - void
  ///
  /// @tparam propagator_state_t is the type of Propagatgor state
  /// @tparam stepper_t Type of the Stepper
  ///
  /// Empty call, compiler should optimise that
  template <typename propagator_state_t, typename stepper_t>
  void initialize(propagator_state_t& /*state*/,
                  const stepper_t& /*stepper*/) const {}

  /// Navigation call - void
  ///
  /// @tparam propagator_state_t is the type of Propagatgor state
  /// @tparam stepper_t Type of the Stepper
  ///
  /// Empty call, compiler should optimise that
  template <typename propagator_state_t, typename stepper_t>
  void preStep(propagator_state_t& /*state*/,
               const stepper_t& /*stepper*/) const {}

  /// Navigation call - void
  ///
  /// @tparam propagator_state_t is the type of Propagatgor state
  /// @tparam stepper_t Type of the Stepper
  ///
  /// Empty call, compiler should optimise that
  template <typename propagator_state_t, typename stepper_t>
  void postStep(propagator_state_t& /*state*/,
                const stepper_t& /*stepper*/) const {}
};

}  // namespace Acts
