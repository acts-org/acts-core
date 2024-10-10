// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace Acts {

template <typename actor_t>
concept ActorHasResult = requires { typename actor_t::result_type; };

template <typename actor_t>
struct ActorHasResultStruct {
  static constexpr bool value = ActorHasResult<actor_t>;
};

template <typename actor_t, typename propagator_state_t, typename stepper_t,
          typename navigator_t, typename... Args>
concept ActorHasActWithoutResult = requires(
    const actor_t& a, propagator_state_t& state, const stepper_t& stepper,
    const navigator_t& navigator, Args&&... args) {
  {
    a.act(state, stepper, navigator, std::move<Args>(args)...)
  } -> std::same_as<void>;
};

template <typename actor_t, typename propagator_state_t, typename stepper_t,
          typename navigator_t, typename... Args>
concept ActorHasActWithResult =
    ActorHasResult<actor_t> &&
    requires(const actor_t& a, propagator_state_t& state,
             const stepper_t& stepper, const navigator_t& navigator,
             typename actor_t::result_type& result, Args&&... args) {
      {
        a.act(state, stepper, navigator, result, std::move<Args>(args)...)
      } -> std::same_as<void>;
    };

template <typename actor_t, typename propagator_state_t, typename stepper_t,
          typename navigator_t, typename... Args>
concept ActorHasAct =
    ActorHasActWithoutResult<actor_t, propagator_state_t, stepper_t,
                             navigator_t, Args...> ||
    ActorHasActWithResult<actor_t, propagator_state_t, stepper_t, navigator_t,
                          Args...>;

template <typename actor_t, typename propagator_state_t, typename stepper_t,
          typename navigator_t, typename... Args>
concept ActorHasAbortWithoutResult = requires(
    const actor_t& a, propagator_state_t& state, const stepper_t& stepper,
    const navigator_t& navigator, Args&&... args) {
  {
    a.checkAbort(state, stepper, navigator, std::move<Args>(args)...)
  } -> std::same_as<bool>;
};

template <typename actor_t, typename propagator_state_t, typename stepper_t,
          typename navigator_t, typename... Args>
concept ActorHasAbortWithResult =
    ActorHasResult<actor_t> &&
    requires(const actor_t& a, propagator_state_t& state,
             const stepper_t& stepper, const navigator_t& navigator,
             typename actor_t::result_type& result, Args&&... args) {
      {
        a.checkAbort(state, stepper, navigator, result,
                     std::move<Args>(args)...)
      } -> std::same_as<bool>;
    };

template <typename actor_t, typename propagator_state_t, typename stepper_t,
          typename navigator_t, typename... Args>
concept ActorHasAbort =
    ActorHasAbortWithoutResult<actor_t, propagator_state_t, stepper_t,
                               navigator_t, Args...> ||
    ActorHasAbortWithResult<actor_t, propagator_state_t, stepper_t, navigator_t,
                            Args...>;

template <typename actor_t, typename propagator_state_t, typename stepper_t,
          typename navigator_t, typename... Args>
concept Actor =
    ActorHasAct<actor_t, propagator_state_t, stepper_t, navigator_t, Args...> ||
    ActorHasAbort<actor_t, propagator_state_t, stepper_t, navigator_t, Args...>;

}  // namespace Acts
