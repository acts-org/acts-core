// This file is part of the Acts project.
//
// Copyright (C) 2016-2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Propagator/detail/actor_list_implementation.hpp"
#include "Acts/Utilities/detail/Extendable.hpp"
#include "Acts/Utilities/detail/MPL/all_of.hpp"
#include "Acts/Utilities/detail/MPL/has_duplicates.hpp"
#include "Acts/Utilities/detail/MPL/type_collector.hpp"

#include <boost/hana/type.hpp>
#include <boost/hana/unpack.hpp>

namespace hana = boost::hana;

namespace Acts {

/// @brief ActorList implementation to be used with the propagator
///
/// This is the ActorList struct that is used in the propagator
/// to define a list of different actors_t that are each
/// executed during the stepping procedure
template <typename... actors_t>
struct ActorList : public detail::Extendable<actors_t...> {
 private:
  static_assert(!detail::has_duplicates_v<actors_t...>,
                "same action type specified several times");

  using detail::Extendable<actors_t...>::tuple;

 public:
  /// @cond
  // This uses the type collector and unpacks using the `R` meta function
  template <template <typename...> class R>
  using result_type = typename decltype(hana::unpack(
      detail::type_collector_t<detail::result_type_extractor, actors_t...>,
      hana::template_<R>))::type;
  /// @endcond

  using detail::Extendable<actors_t...>::get;

  /// Default constructor
  ActorList() = default;

  /// Default copy constructor
  ///
  /// @param actors The source action list
  ActorList(const ActorList<actors_t...>& actors) = default;

  /// Default move constructor
  ///
  /// @param actors The source action list
  ActorList(ActorList<actors_t...>&& actors) = default;

  /// Default move assignment operator
  ///
  /// @param actors The source action list
  ActorList<actors_t...>& operator=(const ActorList<actors_t...>& actors) =
      default;

  /// Default move assignment operator
  ///
  /// @param actors The source action list
  ActorList<actors_t...>& operator=(ActorList<actors_t...>&& actors) = default;

  /// Constructor from tuple
  ///
  /// @param actors Source extensions tuple
  ActorList(const std::tuple<actors_t...>& actors)
      : detail::Extendable<actors_t...>(actors) {}

  /// Constructor from tuple move
  ///
  /// @param actors Source extensions tuple
  ActorList(std::tuple<actors_t...>&& actors)
      : detail::Extendable<actors_t...>(std::move(actors)) {}

  /// Append new entries and return a new condition
  template <typename... appendices_t>
  ActorList<actors_t..., appendices_t...> append(appendices_t... aps) const {
    auto catTuple =
        std::tuple_cat(tuple(), std::tuple<appendices_t...>(aps...));
    return ActorList<actors_t..., appendices_t...>(std::move(catTuple));
  }

  /// Act call which broadcasts the call to the tuple() members of the list
  ///
  /// @tparam propagator_state_t is the state type of the propagator
  /// @tparam stepper_t Type of the stepper used for the propagation
  /// @tparam navigator_t Type of the navigator used for the propagation
  ///
  /// @param [in,out] state This is the propagator state object
  /// @param [in] stepper The stepper in use
  /// @param [in] navigator The navigator in use
  /// @param [in] args The arguments to be passed to the actions
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t, typename... Args>
  void act(propagator_state_t& state, const stepper_t& stepper,
           const navigator_t& navigator, Args&&... args) const {
    using impl = detail::actor_list_impl<actors_t...>;
    impl::act(tuple(), state, stepper, navigator, std::forward<Args>(args)...);
  }

  /// Check call which broadcasts the call to the tuple() members of the list
  ///
  /// @tparam propagator_state_t is the state type of the propagator
  /// @tparam stepper_t Type of the stepper
  /// @tparam navigator_t Type of the navigator
  ///
  /// @param [in,out] state is the state object from the propagator
  /// @param [in] stepper Stepper used for the propagation
  /// @param [in] navigator Navigator used for the propagation
  /// @param [in] args are the arguments to be passed to the aborters
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t, typename... Args>
  bool check(propagator_state_t& state, const stepper_t& stepper,
             const navigator_t& navigator, Args&&... args) const {
    using impl = detail::actor_list_impl<actors_t...>;
    return impl::check(tuple(), state, stepper, navigator,
                       std::forward<Args>(args)...);
  }
};

}  // namespace Acts
