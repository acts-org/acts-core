// This file is part of the Acts project.
//
// Copyright (C) 2018-2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Propagator/ActorConcepts.hpp"
#include "Acts/Utilities/detail/MPL/type_collector.hpp"

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
    if constexpr (ActorHasAct<actor_t, propagator_state_t, stepper_t,
                              navigator_t, Args...>) {
      if constexpr (ActorHasResult<actor_t>) {
        actor.act(state, stepper, navigator,
                  state.template get<detail::result_type_t<actor_t>>(),
                  std::forward<Args>(args)...);
      } else {
        actor.act(state, stepper, navigator, std::forward<Args>(args)...);
      }
    }
  }

  template <typename actor_t, typename propagator_state_t, typename stepper_t,
            typename navigator_t, typename... Args>
  static bool check(const actor_t& actor, propagator_state_t& state,
                    const stepper_t& stepper, const navigator_t& navigator,
                    Args&&... args)
    requires(
        Actor<actor_t, propagator_state_t, stepper_t, navigator_t, Args...>)
  {
    if constexpr (ActorHasAbort<actor_t, propagator_state_t, stepper_t,
                                navigator_t, Args...>) {
      if constexpr (ActorHasResult<actor_t>) {
        return actor.check(state, stepper, navigator,
                           state.template get<detail::result_type_t<actor_t>>(),
                           std::forward<Args>(args)...);
      } else {
        return actor.check(state, stepper, navigator,
                           std::forward<Args>(args)...);
      }
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
  static bool check(const std::tuple<actors_t...>& actor_tuple,
                    propagator_state_t& state, const stepper_t& stepper,
                    const navigator_t& navigator, Args&&... args) {
    return std::apply(
        [&](const actors_t&... actor) {
          return (actor_caller::check(actor, state, stepper, navigator,
                                      std::forward<Args>(args)...) ||
                  ...);
        },
        actor_tuple);
  }
};

}  // namespace Acts::detail
