// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Propagator/ActorConcepts.hpp"

#include <tuple>
#include <utility>

namespace Acts::detail {

namespace {

struct actor_caller {
  template <typename actor_t, typename propagator_state_t, typename stepper_t,
            typename navigator_t, typename... Args>
  static void act(const actor_t& actor, propagator_state_t& state,
                  const stepper_t& stepper, const navigator_t& navigator,
                  Args&&... args)
    requires(
        Actor<actor_t, propagator_state_t, stepper_t, navigator_t, Args...>)
  {
    if constexpr (ActorHasActWithoutResult<actor_t, propagator_state_t,
                                           stepper_t, navigator_t, Args...>) {
      actor.act(state, stepper, navigator, std::forward<Args>(args)...);
      return;
    }

    if constexpr (ActorHasActWithResult<actor_t, propagator_state_t, stepper_t,
                                        navigator_t, Args...>) {
      actor.act(state, stepper, navigator,
                state.template get<typename actor_t::result_type>(),
                std::forward<Args>(args)...);
      return;
    }
  }

  template <typename actor_t, typename propagator_state_t, typename stepper_t,
            typename navigator_t, typename... Args>
  static bool checkAbort(const actor_t& actor, propagator_state_t& state,
                         const stepper_t& stepper, const navigator_t& navigator,
                         Args&&... args)
    requires(
        Actor<actor_t, propagator_state_t, stepper_t, navigator_t, Args...>)
  {
    if constexpr (ActorHasAbortWithoutResult<actor_t, propagator_state_t,
                                             stepper_t, navigator_t, Args...>) {
      return actor.checkAbort(state, stepper, navigator,
                              std::forward<Args>(args)...);
    }

    if constexpr (ActorHasAbortWithResult<actor_t, propagator_state_t,
                                          stepper_t, navigator_t, Args...>) {
      return actor.checkAbort(
          state, stepper, navigator,
          state.template get<typename actor_t::result_type>(),
          std::forward<Args>(args)...);
    }

    return false;
  }
};

}  // namespace

template <typename... actors_t>
struct actor_list_impl {
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t, typename... Args>
  static void act(const std::tuple<actors_t...>& actor_tuple,
                  propagator_state_t& state, const stepper_t& stepper,
                  const navigator_t& navigator, Args&&... args) {
    std::apply(
        [&](const actors_t&... actor) {
          (actor_caller::act(actor, state, stepper, navigator,
                             std::forward<Args>(args)...),
           ...);
        },
        actor_tuple);
  }

  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t, typename... Args>
  static bool checkAbort(const std::tuple<actors_t...>& actor_tuple,
                         propagator_state_t& state, const stepper_t& stepper,
                         const navigator_t& navigator, Args&&... args) {
    return std::apply(
        [&](const actors_t&... actor) {
          return (actor_caller::checkAbort(actor, state, stepper, navigator,
                                           std::forward<Args>(args)...) ||
                  ...);
        },
        actor_tuple);
  }
};

}  // namespace Acts::detail
