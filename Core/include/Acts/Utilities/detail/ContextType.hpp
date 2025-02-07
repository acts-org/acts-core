// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <any>

namespace Acts {

/// Strong type wrapper around std::any. This has all the flexibility of
/// std::any, but it does not convert-construct from anything. You have to call
/// one of the explicit constructors manually to populate
/// the internal std::any. You can then access and modify the any as desired.
///
/// @note This is used for the context types, and should probably not be used
/// outside of this use-case.
class ContextType {
 public:
  /// Default constructor, does nothing
  ///
  ContextType() = default;

  /// Move construct a new Context Type object from anything. Must be explicit.
  ///
  /// @tparam T The type of the value to construct from
  /// @param value The value to construct from
  template <typename T>
  explicit ContextType(T&& value) : m_data{std::forward<T>(value)} {}

  /// Copy construct a new Context Type object from anything. Must be explicit.
  ///
  /// @tparam T The type of the value to construct from
  /// @param value The value to construct from
  template <typename T>
  explicit ContextType(const T& value) : m_data{value} {}

  /// Move assignment of anything to this object is allowed.
  ///
  /// @tparam T The type of the value to assign
  /// @param value The value to assign
  /// @return ContextType&
  template <typename T>
  ContextType& operator=(T&& value) {
    m_data = std::forward<T>(value);
    return *this;
  }

  /// Copy assignment of anything to this object is allowed.
  ///
  /// @tparam T The type of the value to assign
  /// @param value The value to assign
  /// @return ContextType&
  template <typename T>
  ContextType& operator=(const T& value) {
    m_data = value;
    return *this;
  }

  /// Retrieve a reference to the contained type
  ///
  /// @tparam T The type to attempt to retrieve the value as
  /// @return Reference to the contained value
  template <typename T>
  std::decay_t<T>& get() {
    return std::any_cast<std::decay_t<T>&>(m_data);
  }

  /// Retrieve a reference to the contained type
  ///
  /// @tparam T The type to attempt to retrieve the value as
  /// @return Reference to the contained value
  template <typename T>
  const std::decay_t<T>& get() const {
    return std::any_cast<const std::decay_t<T>&>(m_data);
  }

  /// Retrieve a pointer to the contained type
  ///
  /// @note Returns `nullptr` if @p is not the contained type.
  ///
  /// @tparam T The type to attempt to retrieve the value as
  /// @return Pointer to the contained value, may be null
  template <typename T>
  std::decay_t<T>* maybeGet() {
    return std::any_cast<std::decay_t<T>>(&m_data);
  }

  /// Retrieve a pointer to the contained type
  ///
  /// @note Returns `nullptr` if @p is not the contained type.
  ///
  /// @tparam T The type to attempt to retrieve the value as
  /// @return Pointer to the contained value, may be null
  template <typename T>
  const std::decay_t<T>* maybeGet() const {
    return std::any_cast<const std::decay_t<T>>(&m_data);
  }

  /// Check if the contained type is initialized.
  /// @return Boolean indicating whether a type is present
  bool hasValue() const { return m_data.has_value(); }

 private:
  std::any m_data;
};

}  // namespace Acts
