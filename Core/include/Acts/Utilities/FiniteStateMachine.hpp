// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <optional>
#include <string_view>
#include <variant>

namespace Acts {

/// Implementation of a finite state machine engine
///
/// Allows setting up a system of states and transitions between them. States
/// are definedd as empty structs (footprint: 1 byte). Transitions call
/// functions using overload resolution. This works by subclassing this class,
/// providing the deriving type as the first template argument (CRTP) and
/// providing methods like
///
/// ```cpp
/// event_return on_event(const S&, const E&);
/// ```
///
/// The arguments are the state `S` and the triggered event `E`. Their values
/// can be discarded (you can attach values to events of course, if you like)
/// The return type of these functions is effectively `std::optional<State>`, so
/// you can either return `std::nullopt` to remain in the same state, or an
/// instance of another state. That state will then become active.
///
/// You can also define a method template, which will serve as a catch-all
/// handler (due to the fact that it will match any state/event combination):
///
/// ```cpp
/// template <typename State, typename Event>
///   event_return on_event(const State&, const Event&) const {
///   return Terminated{};
/// }
/// ```
///
/// If for a given state and event no suitable overload of `on_event` (and you
/// also haven't defined a catch-all as described above), a transition to
/// `Terminated` will be triggered. This is essentially equivalent to the method
/// template above.
///
/// If this triggers, it will switch to the `Terminated` state (which is always
/// included in the FSM).
///
/// Additionally, the FSM will attempt to call functions like
/// ```cpp
/// void on_enter(const State&);
/// void on_exit(const State&);
/// ```
/// when entering/exiting a state. This can be used to
/// perform actions regardless of the source or destination state in a
/// transition to a given state. This is also fired in case a transition to
/// `Terminated` occurs.
///
/// The base class also calls
/// ```cpp
/// void on_process(const Event&);
/// void on_process(const State&, const Event&);
/// void on_process(const State1& const Event&, const State2&);
/// ```
/// during event processing, and allow for things like event and
/// transition logging.
///
/// The `on_event`, `on_enter`, `on_exit` and `on_process` methods need to be
/// implemented exhaustively, i.e. for all state/event combinations. This might
/// require you to add catch-all no-op functions like
/// ```cpp
/// template <typename...Args>
/// event_return on_event(Args&&...args) {} // noop
/// ```
/// and so on.
///
/// The public interface for the user of the FSM are the
/// ```cpp
/// template <typename... Args>
/// void setState(StateVariant state, Args&&... args);
///
/// template <typename Event, typename... Args>
/// void dispatch(Event&& event, Args&&... args) {
/// ```
///
/// `setState` triggers a transition to a given state, `dispatch` triggers
/// processing on an event from the given state. Both will call the appropriate
/// `on_exit` and `on_enter` overloads. Both also accept an arbitrary number of
/// additional arguments that are passed to the `on_event`, `on_exit` and
/// `on_enter` overloads.
///
/// @tparam Derived Class deriving from the FSM
/// @tparam States Argument pack with the state types that the FSM can be
///         handled.
template <typename Derived, typename... States>
class FiniteStateMachine {
 public:
  /// Contractual termination state. Is transitioned to if State+Event do not
  /// have a transition defined.
  struct Terminated {
    /// Name of this state (useful for logging)
    constexpr static std::string_view name = "Terminated";
  };

  /// Variant type allowing tagged type erased storage of the current state of
  /// the FSM.
  using StateVariant = std::variant<Terminated, States...>;

 protected:
  using fsm_base = FiniteStateMachine<Derived, States...>;

  using event_return = std::optional<StateVariant>;

 public:
  /// Default constructor. The default state is taken to be the first in the
  /// `States` template arguments
  FiniteStateMachine()
      : m_state(typename std::tuple_element_t<0, std::tuple<States...>>{}) {}

  /// Constructor from an explicit state. The FSM is initialized to this state.
  /// @param state Initial state for the FSM.
  FiniteStateMachine(StateVariant state) : m_state(std::move(state)) {}

  /// Get the current state of the FSM (as a variant).
  /// @return StateVariant The current state of the FSM.
  const StateVariant& getState() const noexcept { return m_state; }

 public:
  /// Sets the state to a given one. Triggers `on_exit` and `on_enter` for the
  /// given states.
  /// @tparam State Type of the target state
  /// @tparam Args Additional arguments passed through callback overloads.
  /// @param state Instance of the target state
  /// @param args The additional arguments
  template <typename State, typename... Args>
  void setState(State state, Args&&... args) {
    Derived& child = static_cast<Derived&>(*this);

    // call on exit function
    std::visit([&](auto& s) { child.on_exit(s, std::forward<Args>(args)...); },
               m_state);

    m_state = std::move(state);

    // call on enter function, the type is known from the template argument.
    child.on_enter(std::get<State>(m_state), std::forward<Args>(args)...);
  }

  /// Returns whether the FSM is in the specified state
  /// @tparam State type to check against
  /// @return Whether the FSM is in the given state.
  template <typename S>
  bool is(const S& /*state*/) const noexcept {
    return is<S>();
  }

  /// Returns whether the FSM is in the specified state. Alternative version
  /// directly taking only the template argument.
  /// @tparam State type to check against
  /// @return Whether the FSM is in the given state.
  template <typename S>
  bool is() const noexcept {
    if (std::get_if<S>(&m_state)) {
      return true;
    }
    return false;
  }

  /// Returns whether the FSM is in the terminated state.
  /// @return Whether the FSM is in the terminated state.
  bool terminated() const noexcept { return is<Terminated>(); }

 protected:
  /// Handles processing of an event.
  /// @note This should only be called from inside the class Deriving from FSM.
  /// @tparam Event Type of the event being processed
  /// @tparam Args Arguments being passed to the overload handlers.
  /// @param event Instance of the event
  /// @param args Additional arguments
  /// @return Variant state type, signifying if a transition is supposed to
  ///         happen.
  template <typename Event, typename... Args>
  event_return process_event(Event&& event, Args&&... args) {
    Derived& child = static_cast<Derived&>(*this);

    child.on_process(event);

    auto new_state = std::visit(
        [&](auto& s) -> std::optional<StateVariant> {
          auto s2 = child.on_event(s, std::forward<Event>(event),
                                   std::forward<Args>(args)...);

          if (s2) {
            std::visit([&](auto& s2_) { child.on_process(s, event, s2_); },
                       *s2);
          } else {
            child.on_process(s, event);
          }
          return s2;
        },
        m_state);
    return new_state;
  }

 public:
  /// Public interface to handle an event. Will call the appropriate event
  /// handlers and perform any required transitions.
  /// @tparam Event Type of the event being triggered
  /// @tparam Args Additional arguments being passed to overload handlers.
  /// @param event Instance of the event being triggere
  /// @param args Additional arguments
  template <typename Event, typename... Args>
  void dispatch(Event&& event, Args&&... args) {
    auto new_state = process_event(std::forward<Event>(event), args...);
    if (new_state) {
      std::visit(
          [&](auto& s) { setState(std::move(s), std::forward<Args>(args)...); },
          *new_state);
    }
  }

 private:
  StateVariant m_state;
};

}  // namespace Acts
