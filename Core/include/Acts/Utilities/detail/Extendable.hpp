// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <tuple>
#include <type_traits>

namespace Acts::detail {

/// This sctruct defines an extendable std::tuple
///
/// all Extensions have to :
///   - default constructible
///   - copy constructible
///   - move constructible
///
/// This is needed in order to allow custom construction of objects
template <typename... extensions_t>
struct Extendable {
  // clang-format off
    static_assert((std::is_default_constructible_v<extensions_t> && ...),
                  "all extensions must be default constructible");
    static_assert((std::is_copy_constructible_v<extensions_t> && ...),
                  "all extensions must be copy constructible");
    static_assert((std::is_move_constructible_v<extensions_t> && ...),
                  "all extensions must be move constructible");
  // clang-format on

  /// Default constructor
  Extendable() = default;

  /// Default copy constructor
  Extendable(const Extendable<extensions_t...>& extendable) = default;

  // Default move constructor
  Extendable(Extendable<extensions_t...>&& extendable) noexcept = default;

  /// Constructor from tuple
  ///
  /// @param extensions Source extensions tuple
  Extendable(const std::tuple<extensions_t...>& extensions)
      : m_extensions(extensions) {}

  /// Constructor from tuple move
  ///
  /// @param extensions source extensions tuple
  Extendable(std::tuple<extensions_t...>&& extensions)
      : m_extensions(std::move(extensions)) {}

  /// Default move assignment operator
  ///
  /// @param extendable The source Extendable list
  Extendable<extensions_t...>& operator=(
      const Extendable<extensions_t...>& extendable) = default;

  /// Default move assignment operator
  ///
  /// @param extendable The source Extendable list
  Extendable<extensions_t...>& operator=(
      Extendable<extensions_t...>&& extendable) noexcept = default;

  /// Append new entries and return a new condition
  ///
  /// @tparam appendices_t Types of appended entries to the tuple
  ///
  /// @param aps The extensions to be appended to the new Extendable
  template <typename... appendices_t>
  Extendable<extensions_t..., appendices_t...> append(
      appendices_t... aps) const {
    auto catTuple =
        std::tuple_cat(m_extensions, std::tuple<appendices_t...>(aps...));
    return Extendable<extensions_t..., appendices_t...>(std::move(catTuple));
  }

  /// Const retrieval of an extension of a specific type
  ///
  /// @tparam extension_t Type of the Extension to be retrieved
  template <typename extension_t>
  const extension_t& get() const {
    return std::get<extension_t>(m_extensions);
  }

  /// Non-const retrieval of an extension of a specific type
  ///
  /// @tparam extension_t Type of the Extension to be retrieved
  template <typename extension_t>
  extension_t& get() {
    return std::get<extension_t>(m_extensions);
  }

  /// Const retrieval of the extension tuype
  ///
  /// @tparam extension_t Type of the Extension to be retrieved
  const std::tuple<extensions_t...>& tuple() const { return m_extensions; }

  /// Non-Const retrieval of the extendsion tuype
  ///
  /// @tparam extension_t Type of the Extension to be retrieved
  std::tuple<extensions_t...>& tuple() { return m_extensions; }

 private:
  std::tuple<extensions_t...> m_extensions;
};

}  // namespace Acts::detail
