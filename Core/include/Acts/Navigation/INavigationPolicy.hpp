// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Navigation/NavigationDelegate.hpp"
#include "Acts/Navigation/NavigationStream.hpp"
#include "Acts/Utilities/DelegateChainBuilder.hpp"

#include <type_traits>

namespace Acts {

class TrackingVolume;
class INavigationPolicy;

/// Concept for a navigation policy
/// This exists so `updateState` can be a non-virtual method and we still have a
/// way to enforce it exists.
template <typename T>
concept NavigationPolicyConcept = requires {
  requires std::is_base_of_v<INavigationPolicy, T>;
  // Has a conforming update method
  requires requires(T policy, const NavigationArguments& args) {
    policy.initializeCandidates(args,
                                std::declval<AppendOnlyNavigationStream&>(),
                                std::declval<const Logger&>());
  };
};

/// Base class for all navigation policies. The policy needs to be *connected*
/// to a delegate via a virtual method for it to become active. The update
/// method is not part of the class interface. The conventional `updateState`
/// method is only required for use with the navigation policy factory,
/// otherwise `connect` is free to connect any function.
class INavigationPolicy {
 public:
  /// Noop update function that is suitable as a default for default navigation
  /// delegates.
  static void noopInitializeCandidates(
      const NavigationArguments& /*unused*/,
      const AppendOnlyNavigationStream& /*unused*/, const Logger& /*unused*/) {
    // This is a noop
  }

  /// Virtual destructor so policies can be held through this base class.
  virtual ~INavigationPolicy() = default;

  /// Connect a policy with a delegate (usually a member of a volume).
  /// This method exists to allow a policy to ensure a non-virtual function is
  /// registered with the delegate.
  /// @param delegate The delegate to connect to
  virtual void connect(NavigationDelegate& delegate) const = 0;

 protected:
  /// Internal helper function for derived classes that conform to the concept
  /// and have a conventional `updateState` method. Mainly used to save some
  /// boilerplate.
  /// @tparam T The type of the navigation policy
  /// @param delegate The delegate to connect to
  template <NavigationPolicyConcept T>
  void connectDefault(NavigationDelegate& delegate) const {
    // This cannot be a concept because we use it in CRTP below
    const auto* self = static_cast<const T*>(this);
    DelegateChainBuilder{delegate}.add<&T::initializeCandidates>(self).store(
        delegate);
  }
};

}  // namespace Acts
