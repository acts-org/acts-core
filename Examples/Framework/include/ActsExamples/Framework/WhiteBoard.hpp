// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <Acts/Utilities/Concepts.hpp>
#include <Acts/Utilities/Logger.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ActsExamples {

/// A container to store arbitrary objects with ownership transfer.
///
/// This is an append-only container that takes ownership of the objects
/// added to it. Once an object has been added, it can only be read but not
/// be modified. Trying to replace an existing object is considered an error.
/// Its lifetime is bound to the lifetime of the white board.
class WhiteBoard {
 public:
  WhiteBoard(
      std::unique_ptr<const Acts::Logger> logger =
          Acts::getDefaultLogger("WhiteBoard", Acts::Logging::INFO),
      std::unordered_multimap<std::string, std::string> objectAliases = {});

  WhiteBoard(const WhiteBoard& other) = delete;
  WhiteBoard& operator=(const WhiteBoard&) = delete;

  WhiteBoard(WhiteBoard&& other) = default;
  WhiteBoard& operator=(WhiteBoard&& other) = default;

  bool exists(const std::string& name) const;

  /// Copies key from another whiteboard to this whiteboard.
  /// This is a low overhead operation, since the data holders are
  /// shared pointers.
  /// Throws an exception if this whiteboard already contains one of
  /// the keys in the other whiteboard.
  void copyFrom(const WhiteBoard& other);

 private:
  /// Find similar names for suggestions with levenshtein-distance
  std::vector<std::string_view> similarNames(const std::string_view& name,
                                             int distThreshold,
                                             std::size_t maxNumber) const;

  // type-erased value holder for move-constructible types
  struct IHolder {
    virtual ~IHolder() = default;
    virtual const std::type_info& type() const = 0;
  };
  template <Acts::Concepts::nothrow_move_constructible T>
  struct HolderT : public IHolder {
    T value;

    explicit HolderT(T&& v) : value(std::move(v)) {}
    const std::type_info& type() const override { return typeid(T); }
  };

  /// Store a holder on the white board.
  ///
  /// @param name Non-empty identifier to store it under
  /// @param holder The holder to store
  /// @throws std::invalid_argument on empty or duplicate name
  void addHolder(const std::string& name,
                 const std::shared_ptr<IHolder>& holder);

  /// Store an object on the white board and transfer ownership.
  ///
  /// @param name Non-empty identifier to store it under
  /// @param object Movable reference to the transferable object
  template <typename T>
  void add(const std::string& name, T&& object) {
    addHolder(name, std::make_shared<HolderT<T>>(std::forward<T>(object)));
  }

  /// Get access to a stored object.
  ///
  /// @param[in] name Identifier for the object
  /// @return reference to the stored object
  /// @throws std::out_of_range if no object is stored under the requested name
  template <typename T>
  const T& get(const std::string& name) const;

  std::unique_ptr<const Acts::Logger> m_logger;
  std::unordered_map<std::string, std::shared_ptr<IHolder>> m_store;
  std::unordered_multimap<std::string, std::string> m_objectAliases;

  const Acts::Logger& logger() const { return *m_logger; }

  static std::string typeMismatchMessage(const std::string& name,
                                         const char* req, const char* act);

  template <typename T>
  friend class WriteDataHandle;

  template <typename T>
  friend class ReadDataHandle;
};

}  // namespace ActsExamples

inline ActsExamples::WhiteBoard::WhiteBoard(
    std::unique_ptr<const Acts::Logger> logger,
    std::unordered_multimap<std::string, std::string> objectAliases)
    : m_logger(std::move(logger)), m_objectAliases(std::move(objectAliases)) {}

template <typename T>
inline const T& ActsExamples::WhiteBoard::get(const std::string& name) const {
  ACTS_VERBOSE("Attempt to get object '" << name << "' of type "
                                         << typeid(T).name());
  auto it = m_store.find(name);
  if (it == m_store.end()) {
    const auto names = similarNames(name, 10, 3);

    std::stringstream ss;
    if (!names.empty()) {
      ss << ", similar ones are: [ ";
      for (std::size_t i = 0; i < std::min(3ul, names.size()); ++i) {
        ss << "'" << names[i] << "' ";
      }
      ss << "]";
    }

    throw std::out_of_range("Object '" + name + "' does not exists" + ss.str());
  }

  const IHolder* holder = it->second.get();

  const auto* castedHolder = dynamic_cast<const HolderT<T>*>(holder);
  if (castedHolder == nullptr) {
    std::string msg =
        typeMismatchMessage(name, typeid(T).name(), holder->type().name());
    throw std::out_of_range(msg.c_str());
  }

  ACTS_VERBOSE("Retrieved object '" << name << "'");
  return castedHolder->value;
}

inline bool ActsExamples::WhiteBoard::exists(const std::string& name) const {
  // TODO remove this function?
  return m_store.contains(name);
}
