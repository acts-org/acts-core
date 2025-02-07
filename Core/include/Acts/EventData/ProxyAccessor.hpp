// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/TrackProxy.hpp"
#include "Acts/EventData/TrackStateProxy.hpp"
#include "Acts/Utilities/HashedString.hpp"

#include <type_traits>

namespace Acts::detail {

template <typename T>
concept MutableProxyType = requires(T t, HashedString key) {
  requires !T::ReadOnly;

  {
    t.template component<int>(key)
  } -> std::same_as<std::conditional_t<T::ReadOnly, const int&, int&>>;
};

template <typename T>
concept ConstProxyType = requires(T t, HashedString key) {
  requires T::ReadOnly;
  { t.template component<int>(key) } -> std::same_as<const int&>;
};

template <typename T>
concept ProxyType = (MutableProxyType<T> || ConstProxyType<T>) && requires {
  typename T::ConstProxyType;

  requires ConstProxyType<typename T::ConstProxyType>;
};

}  // namespace Acts::detail

namespace Acts {

namespace detail {
template <typename... Args>
struct associatedConstProxy;

template <typename trajectory_t, std::size_t M, bool read_only>
struct associatedConstProxy<TrackStateProxy<trajectory_t, M, read_only>> {
  using type = TrackStateProxy<trajectory_t, M, true>;
};

template <typename track_container_t, typename trajectory_t,
          template <typename> class holder_t, bool read_only>
struct associatedConstProxy<
    TrackProxy<track_container_t, trajectory_t, holder_t, read_only>> {
  using type = TrackProxy<track_container_t, trajectory_t, holder_t, true>;
};

}  // namespace detail

/// Utility class that eases accessing dynamic columns in track and track state
/// containers
/// @tparam T the type of the value to access
/// @tparam ReadOnly true if this is a const accessor
template <typename T, bool ReadOnly>
struct ProxyAccessorBase {
  HashedString key;

  /// Create the accessor from an already-hashed string key
  /// @param _key the key
  constexpr ProxyAccessorBase(HashedString _key) : key{_key} {}

  /// Create the accessor from a string key
  /// @param _key the key
  constexpr ProxyAccessorBase(const std::string& _key)
      : key{hashStringDynamic(_key)} {}

  /// Access the stored key on the proxy given as an argument. Mutable version
  /// @tparam proxy_t the type of the proxy
  /// @param proxy the proxy object to access
  /// @return mutable reference to the column behind the key
  template <detail::MutableProxyType proxy_t>
  T& operator()(proxy_t proxy) const
    requires(!ReadOnly)
  {
    static_assert(!proxy_t::ReadOnly,
                  "Cannot get mutable ref for const track proxy");
    return proxy.template component<T>(key);
  }

  /// Access the stored key on the proxy given as an argument. Const version
  /// @tparam proxy_t the type of the track proxy
  /// @param proxy the proxy to access
  /// @return const reference to the column behind the key
  template <detail::ProxyType proxy_t>
  const T& operator()(proxy_t proxy) const
    requires(ReadOnly)
  {
    if constexpr (proxy_t::ReadOnly) {
      return proxy.template component<T>(key);

    } else {
      using const_proxy_t = typename proxy_t::ConstProxyType;
      const_proxy_t cproxy{proxy};
      return cproxy.template component<T>(key);
    }
  }
};

template <typename T>
using ProxyAccessor = ProxyAccessorBase<T, false>;
template <typename T>
using ConstProxyAccessor = ProxyAccessorBase<T, true>;
}  // namespace Acts
