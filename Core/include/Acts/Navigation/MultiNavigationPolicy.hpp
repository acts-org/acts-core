// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Navigation/INavigationPolicy.hpp"

namespace Acts {

/// Base class for multi navigation policies
class MultiNavigationPolicyBase : public INavigationPolicy {};

/// Combined navigation policy that calls all contained other navigation
/// policies. This class only works with policies complying with
/// `NavigationPolicyConcept`, which means that they have a conventional
/// `updateState` method.
///
/// Internally, this uses a delegate chain factory to produce an unrolled
/// delegate chain.
///
/// @tparam Policies The navigation policies to be combined
template <NavigationPolicyConcept... Policies>
  requires(sizeof...(Policies) > 0)
class MultiNavigationPolicy final : public MultiNavigationPolicyBase {
 public:
  /// Constructor from a set of child policies.
  /// @param policies The child policies
  explicit MultiNavigationPolicy(Policies&&... policies)
      : m_policies{std::move(policies)...} {}

  /// Implementation of the connection to a navigation delegate.
  /// It uses the delegate chain factory to produce a delegate chain and stores
  /// that chain in the owning navigation delegate.
  /// @param delegate The navigation delegate to connect to
  void connect(NavigationDelegate& delegate) const override {
    auto factory = add(DelegateChainBuilder{delegate},
                       std::index_sequence_for<Policies...>{});

    factory.store(delegate);
  }

  /// Access the contained policies
  /// @return The contained policies
  const std::tuple<Policies...>& policies() const { return m_policies; }

 private:
  /// Internal helper to build the delegate chain
  template <std::size_t... Is>
  constexpr auto add(
      auto factory,
      std::integer_sequence<std::size_t, Is...> /*unused*/) const {
    return add<Is...>(factory);
  }

  /// Internal helper to build the delegate chain
  template <std::size_t I, std::size_t... Is>
  constexpr auto add(auto factory) const {
    using policy_type = std::tuple_element_t<I, decltype(m_policies)>;
    auto* policy = &std::get<I>(m_policies);

    auto added =
        factory.template add<&policy_type::initializeCandidates>(policy);

    if constexpr (sizeof...(Is) > 0) {
      return add<Is...>(added);
    } else {
      return added;
    }
  }

  std::tuple<Policies...> m_policies;
};
}  // namespace Acts
