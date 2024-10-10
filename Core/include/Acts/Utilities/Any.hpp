// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <any>
#include <array>
#include <cassert>
#include <cstddef>
#include <utility>

// #define _ACTS_ANY_ENABLE_VERBOSE
// #define _ACTS_ANY_ENABLE_DEBUG
// #define _ACTS_ANY_ENABLE_TRACK_ALLOCATIONS

#if defined(_ACTS_ANY_ENABLE_TRACK_ALLOCATIONS)
#include <iostream>
#include <mutex>
#include <set>
#include <typeindex>
#include <typeinfo>
#endif

#if defined(_ACTS_ANY_ENABLE_VERBOSE) || defined(_ACTS_ANY_ENABLE_DEBUG)
#include <iomanip>
#include <iostream>
#endif

#if defined(_ACTS_ANY_ENABLE_DEBUG)
#define _ACTS_ANY_DEBUG(x) std::cout << x << std::endl;
#else
#define _ACTS_ANY_DEBUG(x)
#endif

#if defined(_ACTS_ANY_ENABLE_VERBOSE)
#define _ACTS_ANY_VERBOSE(x) std::cout << x << std::endl;
#define _ACTS_ANY_VERBOSE_BUFFER(s, b)              \
  do {                                              \
    std::cout << "" << s << ": 0x";                 \
    for (char c : b) {                              \
      std::cout << std::hex << static_cast<int>(c); \
    }                                               \
    std::cout << std::endl;                         \
  } while (0)
#else
#define _ACTS_ANY_VERBOSE(x)
#define _ACTS_ANY_VERBOSE_BUFFER(s, b)
#endif

namespace Acts {

#if defined(_ACTS_ANY_ENABLE_TRACK_ALLOCATIONS)
static std::mutex _s_any_mutex;
static std::set<std::pair<std::type_index, void*>> _s_any_allocations;

#define _ACTS_ANY_TRACK_ALLOCATION(T, heap)                                  \
  do {                                                                       \
    std::lock_guard guard{_s_any_mutex};                                     \
    _s_any_allocations.emplace(std::type_index(typeid(T)), heap);            \
    _ACTS_ANY_DEBUG("Allocate type: " << typeid(T).name() << " at " << heap) \
  } while (0)

#define _ACTS_ANY_TRACK_DEALLOCATION(T, heap)                                 \
  do {                                                                        \
    std::lock_guard guard{_s_any_mutex};                                      \
    auto it =                                                                 \
        _s_any_allocations.find(std::pair{std::type_index(typeid(T)), heap}); \
    if (it == _s_any_allocations.end()) {                                     \
      throw std::runtime_error{                                               \
          "Trying to deallocate heap address that we didn't allocate"};       \
    }                                                                         \
    _s_any_allocations.erase(it);                                             \
  } while (0)

struct _AnyAllocationReporter {
  static void checkAllocations() {
    std::lock_guard guard{_s_any_mutex};

    if (!_s_any_allocations.empty()) {
      std::cout << "Not all allocations have been released" << std::endl;
      for (const auto& [idx, addr] : _s_any_allocations) {
        std::cout << "- " << idx.name() << ": " << addr << std::endl;
      }
      throw std::runtime_error{"AnyCheckAllocations failed"};
    }
  }

  ~_AnyAllocationReporter() noexcept { checkAllocations(); }
};
static _AnyAllocationReporter s_reporter;
#else
#define _ACTS_ANY_TRACK_ALLOCATION(T, heap) \
  do {                                      \
  } while (0)
#define _ACTS_ANY_TRACK_DEALLOCATION(T, heap) \
  do {                                        \
  } while (0)
#endif

class AnyBaseAll {};

/// Small opaque cache type which uses small buffer optimization
template <std::size_t SIZE>
class AnyBase : public AnyBaseAll {
  static_assert(sizeof(void*) <= SIZE, "Size is too small for a pointer");

 public:
  template <typename T, typename... Args>
  explicit AnyBase(std::in_place_type_t<T> /*unused*/, Args&&... args) {
    using U = std::decay_t<T>;
    static_assert(
        std::is_move_assignable_v<U> && std::is_move_constructible_v<U>,
        "Type needs to be move assignable and move constructible");
    static_assert(
        std::is_copy_assignable_v<U> && std::is_copy_constructible_v<U>,
        "Type needs to be copy assignable and copy constructible");

    m_handler = makeHandler<U>();
    if constexpr (!heapAllocated<U>()) {
      // construct into local buffer
      /*U* ptr =*/new (m_data.data()) U(std::forward<Args>(args)...);
      _ACTS_ANY_VERBOSE("Construct local (this="
                        << this
                        << ") at: " << static_cast<void*>(m_data.data()));
    } else {
      // too large, heap allocate
      U* heap = new U(std::forward<Args>(args)...);
      _ACTS_ANY_DEBUG("Allocate type: " << typeid(U).name() << " at " << heap);
      _ACTS_ANY_TRACK_ALLOCATION(T, heap);
      setDataPtr(heap);
    }
  }

#if defined(_ACTS_ANY_ENABLE_VERBOSE)
  AnyBase() { _ACTS_ANY_VERBOSE("Default construct this=" << this); };
#else
  AnyBase() = default;
#endif

  template <typename T>
  explicit AnyBase(T&& value)
    requires(!std::same_as<std::decay_t<T>, AnyBase<SIZE>>)
      : AnyBase{std::in_place_type<T>, std::forward<T>(value)} {}

  template <typename T>
  T& as() {
    static_assert(std::is_same_v<T, std::decay_t<T>>,
                  "Please pass the raw type, no const or ref");
    if (makeHandler<T>() != m_handler) {
      throw std::bad_any_cast{};
    }

    _ACTS_ANY_VERBOSE("Get as "
                      << (m_handler->heapAllocated ? "heap" : "local"));

    return *reinterpret_cast<T*>(dataPtr());
  }

  template <typename T>
  const T& as() const {
    static_assert(std::is_same_v<T, std::decay_t<T>>,
                  "Please pass the raw type, no const or ref");
    if (makeHandler<T>() != m_handler) {
      throw std::bad_any_cast{};
    }

    _ACTS_ANY_VERBOSE("Get as " << (m_handler->heap ? "heap" : "local"));

    return *reinterpret_cast<const T*>(dataPtr());
  }

  ~AnyBase() { destroy(); }

  AnyBase(const AnyBase& other) {
    if (m_handler == nullptr && other.m_handler == nullptr) {
      // both are empty, noop
      return;
    }

    _ACTS_ANY_VERBOSE("Copy construct (this="
                      << this << ") at: " << static_cast<void*>(m_data.data()));

    m_handler = other.m_handler;
    copyConstruct(other);
  }

  AnyBase& operator=(const AnyBase& other) {
    _ACTS_ANY_VERBOSE("Copy assign (this="
                      << this << ") at: " << static_cast<void*>(m_data.data()));

    if (m_handler == nullptr && other.m_handler == nullptr) {
      // both are empty, noop
      return *this;
    }

    if (m_handler == other.m_handler) {
      // same type, but checked before they're not both nullptr
      copy(std::move(other));
    } else {
      if (m_handler != nullptr) {
        // this object is not empty, but have different types => destroy
        destroy();
      }
      assert(m_handler == nullptr);
      m_handler = other.m_handler;
      copyConstruct(std::move(other));
    }
    return *this;
  }

  AnyBase(AnyBase&& other) {
    _ACTS_ANY_VERBOSE("Move construct (this="
                      << this << ") at: " << static_cast<void*>(m_data.data()));
    if (m_handler == nullptr && other.m_handler == nullptr) {
      // both are empty, noop
      return;
    }

    m_handler = other.m_handler;
    moveConstruct(std::move(other));
  }

  AnyBase& operator=(AnyBase&& other) {
    _ACTS_ANY_VERBOSE("Move assign (this="
                      << this << ") at: " << static_cast<void*>(m_data.data()));
    if (m_handler == nullptr && other.m_handler == nullptr) {
      // both are empty, noop
      return *this;
    }

    if (m_handler == other.m_handler) {
      // same type, but checked before they're not both nullptr
      move(std::move(other));
    } else {
      if (m_handler != nullptr) {
        // this object is not empty, but have different types => destroy
        destroy();
      }
      assert(m_handler == nullptr);
      m_handler = other.m_handler;
      moveConstruct(std::move(other));
    }

    return *this;
  }

  operator bool() const { return m_handler != nullptr; }

 private:
  void* dataPtr() {
    if (m_handler->heapAllocated) {
      return *reinterpret_cast<void**>(m_data.data());
    } else {
      return reinterpret_cast<void*>(m_data.data());
    }
  }

  void setDataPtr(void* ptr) { *reinterpret_cast<void**>(m_data.data()) = ptr; }

  const void* dataPtr() const {
    if (m_handler->heapAllocated) {
      return *reinterpret_cast<void* const*>(m_data.data());
    } else {
      return reinterpret_cast<const void*>(m_data.data());
    }
  }

  struct Handler {
    void (*destroy)(void* ptr) = nullptr;
    void (*moveConstruct)(void* from, void* to) = nullptr;
    void (*move)(void* from, void* to) = nullptr;
    void* (*copyConstruct)(const void* from, void* to) = nullptr;
    void (*copy)(const void* from, void* to) = nullptr;
    bool heapAllocated{false};
  };

  template <typename T>
  static const Handler* makeHandler() {
    static_assert(!std::is_same_v<T, AnyBase<SIZE>>, "Cannot wrap any in any");
    static const Handler static_handler = []() {
      Handler h;
      h.heapAllocated = heapAllocated<T>();
      if constexpr (!std::is_trivially_destructible_v<T> ||
                    heapAllocated<T>()) {
        h.destroy = &destroyImpl<T>;
      }
      if constexpr (!std::is_trivially_move_constructible_v<T> ||
                    heapAllocated<T>()) {
        h.moveConstruct = &moveConstructImpl<T>;
      }
      if constexpr (!std::is_trivially_move_assignable_v<T> ||
                    heapAllocated<T>()) {
        h.move = &moveImpl<T>;
      }
      if constexpr (!std::is_trivially_copy_constructible_v<T> ||
                    heapAllocated<T>()) {
        h.copyConstruct = &copyConstructImpl<T>;
      }
      if constexpr (!std::is_trivially_copy_assignable_v<T> ||
                    heapAllocated<T>()) {
        h.copy = &copyImpl<T>;
      }

      _ACTS_ANY_DEBUG("Type: " << typeid(T).name());
      _ACTS_ANY_DEBUG(" -> destroy: " << h.destroy);
      _ACTS_ANY_DEBUG(" -> moveConstruct: " << h.moveConstruct);
      _ACTS_ANY_DEBUG(" -> move: " << h.move);
      _ACTS_ANY_DEBUG(" -> copyConstruct: " << h.copyConstruct);
      _ACTS_ANY_DEBUG(" -> copy: " << h.copy);
      _ACTS_ANY_DEBUG(
          " -> heapAllocated: " << (h.heapAllocated ? "yes" : "no"));

      return h;
    }();
    return &static_handler;
  }

  template <typename T>
  static constexpr bool heapAllocated() {
    return sizeof(T) > SIZE;
  }

  void destroy() {
    _ACTS_ANY_VERBOSE("Destructor this=" << this << " handler: " << m_handler);
    if (m_handler != nullptr && m_handler->destroy != nullptr) {
      _ACTS_ANY_VERBOSE("Non-trivial destruction");
      m_handler->destroy(dataPtr());
    }
    m_handler = nullptr;
  }

  void moveConstruct(AnyBase&& fromAny) {
    if (m_handler == nullptr) {
      return;
    }

    void* to = dataPtr();
    void* from = fromAny.dataPtr();
    if (m_handler->heapAllocated) {
      // stored on heap: just copy the pointer
      setDataPtr(fromAny.dataPtr());
      // do not delete in moved-from any
      fromAny.m_handler = nullptr;
      return;
    }

    if (m_handler->moveConstruct == nullptr) {
      _ACTS_ANY_VERBOSE("Trivially move construct");
      // trivially move constructible
      m_data = std::move(fromAny.m_data);
    } else {
      m_handler->moveConstruct(from, to);
    }
  }

  void move(AnyBase&& fromAny) {
    if (m_handler == nullptr) {
      return;
    }

    void* to = dataPtr();
    void* from = fromAny.dataPtr();
    if (m_handler->heapAllocated) {
      // stored on heap: just copy the pointer
      // need to delete existing pointer
      m_handler->destroy(dataPtr());
      setDataPtr(fromAny.dataPtr());
      // do not delete in moved-from any
      fromAny.m_handler = nullptr;
      return;
    }

    if (m_handler->move == nullptr) {
      _ACTS_ANY_VERBOSE("Trivially move");
      // trivially movable
      m_data = std::move(fromAny.m_data);
    } else {
      m_handler->move(from, to);
    }
  }

  void copyConstruct(const AnyBase& fromAny) {
    if (m_handler == nullptr) {
      return;
    }

    void* to = dataPtr();
    const void* from = fromAny.dataPtr();

    if (m_handler->copyConstruct == nullptr) {
      _ACTS_ANY_VERBOSE("Trivially copy construct");
      // trivially copy constructible
      m_data = fromAny.m_data;
    } else {
      void* copyAt = m_handler->copyConstruct(from, to);
      if (to == nullptr) {
        assert(copyAt != nullptr);
        // copy allocated, store pointer
        setDataPtr(copyAt);
      }
    }
  }

  void copy(const AnyBase& fromAny) {
    if (m_handler == nullptr) {
      return;
    }

    void* to = dataPtr();
    const void* from = fromAny.dataPtr();

    if (m_handler->copy == nullptr) {
      _ACTS_ANY_VERBOSE("Trivially copy");
      // trivially copyable
      m_data = fromAny.m_data;
    } else {
      m_handler->copy(from, to);
    }
  }

  template <typename T>
  static void destroyImpl(void* ptr) {
    assert(ptr != nullptr && "Address to destroy is nullptr");
    T* obj = static_cast<T*>(ptr);
    if constexpr (!heapAllocated<T>()) {
      // stored in place: just call the destructor
      _ACTS_ANY_VERBOSE("Destroy local at: " << ptr);
      obj->~T();
    } else {
      // stored on heap: delete
      _ACTS_ANY_DEBUG("Delete type: " << typeid(T).name()
                                      << " heap at: " << obj);
      _ACTS_ANY_TRACK_DEALLOCATION(T, obj);
      delete obj;
    }
  }

  template <typename T>
  static void moveConstructImpl(void* from, void* to) {
    _ACTS_ANY_VERBOSE("move const: " << from << " -> " << to);
    assert(from != nullptr && "Source is null");
    assert(to != nullptr && "Target is null");
    T* _from = static_cast<T*>(from);
    /*T* ptr =*/new (to) T(std::move(*_from));
  }

  template <typename T>
  static void moveImpl(void* from, void* to) {
    _ACTS_ANY_VERBOSE("move: " << from << " -> " << to);
    assert(from != nullptr && "Source is null");
    assert(to != nullptr && "Target is null");

    T* _from = static_cast<T*>(from);
    T* _to = static_cast<T*>(to);

    (*_to) = std::move(*_from);
  }

  template <typename T>
  static void* copyConstructImpl(const void* from, void* to) {
    _ACTS_ANY_VERBOSE("copy const: " << from << " -> " << to);
    assert(from != nullptr && "Source is null");
    const T* _from = static_cast<const T*>(from);
    if (to == nullptr) {
      assert(heapAllocated<T>() && "Received nullptr in local buffer case");
      to = new T(*_from);
      _ACTS_ANY_TRACK_ALLOCATION(T, to);

    } else {
      assert(!heapAllocated<T>() && "Received non-nullptr in heap case");
      /*T* ptr =*/new (to) T(*_from);
    }
    return to;
  }

  template <typename T>
  static void copyImpl(const void* from, void* to) {
    _ACTS_ANY_VERBOSE("copy: " << from << " -> " << to);
    assert(from != nullptr && "Source is null");
    assert(to != nullptr && "Target is null");

    const T* _from = static_cast<const T*>(from);
    T* _to = static_cast<T*>(to);

    (*_to) = *_from;
  }

  static constexpr std::size_t kMaxAlignment =
      std::max(alignof(std::max_align_t),
#if defined(__AVX512F__)
               std::size_t{64}
#elif defined(__AVX__)
               std::size_t{32}
#elif defined(__SSE__)
               std::size_t{16}
#else
               std::size_t{0}
  // Neutral element
  // for maximum
#endif
      );

  alignas(kMaxAlignment) std::array<std::byte, SIZE> m_data{};
  const Handler* m_handler{nullptr};
};

using Any = AnyBase<sizeof(void*)>;

#undef _ACTS_ANY_VERBOSE
#undef _ACTS_ANY_VERBOSE_BUFFER
#undef _ACTS_ANY_ENABLE_VERBOSE

}  // namespace Acts
