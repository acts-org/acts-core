// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Navigation/INavigationPolicy.hpp"
#include "Acts/Navigation/MultiNavigationPolicy.hpp"

#include <concepts>
#include <memory>
namespace Acts {

class TrackingVolume;
class GeometryContext;
class Logger;
class INavigationPolicy;

namespace detail {
template <typename... Factories>
class NavigationPolicyFactoryImpl;
}

/// Base class for navigation policy factories. The factory can be assembled
/// iteratively by using `make` followed by a number of calls to the `add`
/// function of the helper type. Example:
///
/// ```cpp
/// auto factory = NavigationPolicyFactory::make()
///  .add<NavigationPolicy1>(arg1, arg2)
///  .add<NavigationPolicy2>(/*no args*/)
///  .asUniquePtr();
/// ```
class NavigationPolicyFactory {
 public:
  virtual ~NavigationPolicyFactory() = default;

  // This needs to be listed here, but the return type cannot be spelled out
  // yet.
  static auto make();

  // This will potentially get serialization interface and deserialization
  // functionality

  virtual std::unique_ptr<INavigationPolicy> build(
      const GeometryContext& gctx, const TrackingVolume& volume,
      const Logger& logger) const = 0;
};

namespace detail {

template <typename F, typename... Args>
concept NavigationPolicyIsolatedFactoryConcept = requires(
    F f, const GeometryContext& gctx, const TrackingVolume& volume,
    const Logger& logger, Args&&... args) {
  { f(gctx, volume, logger, args...) } -> std::derived_from<INavigationPolicy>;

  requires NavigationPolicyConcept<decltype(f(gctx, volume, logger, args...))>;

  requires(std::is_copy_constructible_v<Args> && ...);
};

template <>
class NavigationPolicyFactoryImpl<> {
 public:
  template <typename...>
  friend class NavigationPolicyFactoryImpl;
  NavigationPolicyFactoryImpl() = default;

  /// Create a factory with the specified policy added
  /// @tparam P The policy type to add
  /// @param args The arguments to pass to the policy constructor
  /// @note Arguments need to be copy constructible because the factory must be
  ///       able to execute multiple times.
  /// @return A new policy factory including the @c P policy.
  template <NavigationPolicyConcept P, typename... Args>
    requires(std::is_constructible_v<P, const GeometryContext&,
                                     const TrackingVolume&, const Logger&,
                                     Args...> &&
             (std::is_copy_constructible_v<Args> && ...))
  constexpr auto add(Args&&... args) && {
    auto factory = [=](const GeometryContext& gctx,
                       const TrackingVolume& volume, const Logger& logger) {
      return P{gctx, volume, logger, args...};
    };

    return NavigationPolicyFactoryImpl<decltype(factory)>{
        std::make_tuple(std::move(factory))};
  }

  /// Create a factory with a policy returned by a factory function
  /// @tparam Fn The type of the function to construct the policy
  /// @param args The arguments to pass to the policy factory
  /// @note Arguments need to be copy constructible because the factory must be
  ///       able to execute multiple times.
  /// @return A new policy factory including the function
  template <typename Fn, typename... Args>
    requires(NavigationPolicyIsolatedFactoryConcept<Fn, Args...>)
  constexpr auto add(Fn&& fn, Args&&... args) {
    auto factory = [=](const GeometryContext& gctx,
                       const TrackingVolume& volume, const Logger& logger) {
      return fn(gctx, volume, logger, args...);
    };

    return NavigationPolicyFactoryImpl<decltype(factory)>{
        std::make_tuple(std::move(factory))};
  }
};

template <typename F, typename... Fs>
class NavigationPolicyFactoryImpl<F, Fs...> : public NavigationPolicyFactory {
 public:
  /// Create a factory with the specified policy added
  /// @tparam P The policy type to add
  /// @param args The arguments to pass to the policy constructor
  /// @note Arguments need to be copy constructible because the factory must be
  ///       able to execute multiple times.
  /// @return A new policy factory including the @c P policy.
  template <NavigationPolicyConcept P, typename... Args>
    requires(std::is_constructible_v<P, const GeometryContext&,
                                     const TrackingVolume&, const Logger&,
                                     Args...> &&
             (std::is_copy_constructible_v<Args> && ...))
  constexpr auto add(Args&&... args) && {
    auto factory = [=](const GeometryContext& gctx,
                       const TrackingVolume& volume, const Logger& logger) {
      return P{gctx, volume, logger, args...};
    };

    return NavigationPolicyFactoryImpl<F, Fs..., decltype(factory)>{
        std::tuple_cat(std::move(m_factories),
                       std::make_tuple(std::move(factory)))};
  }

  /// Create a factory with a policy returned by a factory function
  /// @tparam Fn The type of the function to construct the policy
  /// @param args The arguments to pass to the policy factory
  /// @note Arguments need to be copy constructible because the factory must be
  ///       able to execute multiple times.
  /// @return A new policy factory including the function
  template <typename Fn, typename... Args>
    requires(NavigationPolicyIsolatedFactoryConcept<Fn, Args...>)
  constexpr auto add(Fn&& fn, Args&&... args) && {
    auto factory = [=](const GeometryContext& gctx,
                       const TrackingVolume& volume, const Logger& logger) {
      return fn(gctx, volume, logger, args...);
    };

    return NavigationPolicyFactoryImpl<F, Fs..., decltype(factory)>{
        std::tuple_cat(std::move(m_factories),
                       std::make_tuple(std::move(factory)))};
  }

  /// Move the factory into a unique pointer
  /// @note Only callable on rvalue references
  /// @return A unique pointer to the factory
  constexpr std::unique_ptr<NavigationPolicyFactoryImpl<F, Fs...>>
  asUniquePtr() && {
    return std::make_unique<NavigationPolicyFactoryImpl<F, Fs...>>(
        std::move(*this));
  }

  /// Construct a navigation policy using the factories
  /// @param gctx The geometry context
  /// @param volume The tracking volume
  /// @param logger The logger
  auto operator()(const GeometryContext& gctx, const TrackingVolume& volume,
                  const Logger& logger) const {
    return std::apply(
        [&](auto&&... factories) {
          // Deduce policy type explicitly here...
          using policy_type = decltype(MultiNavigationPolicy{
              std::invoke(factories, std::declval<const GeometryContext&>(),
                          std::declval<const TrackingVolume&>(),
                          std::declval<const Logger&>())...});

          // ... so we can create a unique_ptr of the concrete type here rather
          // than the base. (`make_unique` can't do type deduction)
          return std::make_unique<policy_type>(
              std::invoke(factories, gctx, volume, logger)...);
        },
        m_factories);
  }

  /// Construct a navigation policy using the factories
  /// @param gctx The geometry context
  /// @param volume The tracking volume
  /// @param logger The logger
  std::unique_ptr<INavigationPolicy> build(
      const GeometryContext& gctx, const TrackingVolume& volume,
      const Logger& logger) const override {
    return operator()(gctx, volume, logger);
  }

 private:
  template <typename...>
  friend class NavigationPolicyFactoryImpl;

  explicit NavigationPolicyFactoryImpl(std::tuple<F, Fs...>&& factories)
      : m_factories(std::move(factories)) {}

  std::tuple<F, Fs...> m_factories;
};

}  // namespace detail

inline auto NavigationPolicyFactory::make() {
  return detail::NavigationPolicyFactoryImpl<>{};
}

}  // namespace Acts
