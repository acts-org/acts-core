// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/Any.hpp"

#include <any>
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

using namespace Acts;

#if defined(_ACTS_ANY_ENABLE_TRACK_ALLOCATIONS)
#define CHECK_ANY_ALLOCATIONS()                 \
  do {                                          \
    _AnyAllocationReporter::checkAllocations(); \
  } while (0)
#else
#define CHECK_ANY_ALLOCATIONS() \
  do {                          \
  } while (0)
#endif

BOOST_AUTO_TEST_SUITE(AnyTests)

BOOST_AUTO_TEST_CASE(AnyConstructPrimitive) {
  {
    // small type
    Any a;
    BOOST_CHECK(!a);

    int v = 5;
    a = Any{v};
    BOOST_CHECK(!!a);

    BOOST_CHECK_EQUAL(a.as<int>(), v);
    BOOST_CHECK_NE(a.as<int>(), v + 1);

    BOOST_CHECK_THROW(a.as<float>(), std::bad_any_cast);
  }
  CHECK_ANY_ALLOCATIONS();

  {
    // type that is large
    Any a;
    BOOST_CHECK(!a);

    std::array<int, 2> v{1, 2};
    a = Any{v};
    BOOST_CHECK(!!a);

    BOOST_CHECK_EQUAL_COLLECTIONS(a.as<decltype(v)>().begin(),
                                  a.as<decltype(v)>().end(), v.begin(),
                                  v.end());
    BOOST_CHECK_THROW(a.as<float>(), std::bad_any_cast);
  }
  CHECK_ANY_ALLOCATIONS();

  {
    // type that is large
    Any a;
    BOOST_CHECK(!a);

    std::array<unsigned long, 5> v{1, 2, 3, 4, 5};
    a = Any{v};
    BOOST_CHECK(!!a);

    BOOST_CHECK_EQUAL_COLLECTIONS(a.as<decltype(v)>().begin(),
                                  a.as<decltype(v)>().end(), v.begin(),
                                  v.end());
    BOOST_CHECK_THROW(a.as<float>(), std::bad_any_cast);
  }
  CHECK_ANY_ALLOCATIONS();
}

BOOST_AUTO_TEST_CASE(AnyAssignConstructEmpty) {
  Any a;
  Any b;
  a = b;
  Any c{a};
  a = std::move(b);
  Any d{std::move(a)};

  BOOST_CHECK(!a);
  BOOST_CHECK(!b);
  BOOST_CHECK(!c);
  BOOST_CHECK(!d);

  CHECK_ANY_ALLOCATIONS();
}

BOOST_AUTO_TEST_CASE(AnyConstructCustom) {
  struct A {
    int value;
    A() { value = 76; }
  };

  Any a;
  BOOST_CHECK(!a);
  a = Any{A{}};

  BOOST_CHECK(!!a);

  BOOST_CHECK_EQUAL(a.as<A>().value, 76);

  CHECK_ANY_ALLOCATIONS();
}

BOOST_AUTO_TEST_CASE(AnyConstructCustomInPlace) {
  struct A {
    int value;
    A(int v) { value = v; }
  };

  Any a{std::in_place_type<A>, 42};
  BOOST_CHECK(!!a);
  BOOST_CHECK_EQUAL(a.as<A>().value, 42);

  CHECK_ANY_ALLOCATIONS();
}

BOOST_AUTO_TEST_CASE(AnyMove) {
  {
    // small type
    Any a;
    BOOST_CHECK(!a);

    int v = 5;
    a = Any{v};
    BOOST_CHECK(!!a);

    Any b = std::move(a);
    BOOST_CHECK(!!b);
    BOOST_CHECK_EQUAL(b.as<int>(), 5);

    Any c;
    c = std::move(b);
    BOOST_CHECK(!!c);
    BOOST_CHECK_EQUAL(c.as<int>(), 5);
  }

  CHECK_ANY_ALLOCATIONS();
}

BOOST_AUTO_TEST_CASE(AnyCopy) {
  {
    // small type
    Any a;
    BOOST_CHECK(!a);

    int v = 5;
    a = Any{v};
    BOOST_CHECK(!!a);

    Any b = a;
    BOOST_CHECK(!!b);
    BOOST_CHECK_EQUAL(b.as<int>(), 5);

    Any c;
    c = a;
    BOOST_CHECK(!!c);
    BOOST_CHECK_EQUAL(c.as<int>(), 5);
  }
  CHECK_ANY_ALLOCATIONS();
}

struct D {
  bool* destroyed;
  D(bool* d) : destroyed{d} {}
  ~D() { *destroyed = true; }
};

struct D2 {
  bool* destroyed{nullptr};
  std::array<char, 512> blob{};

  D2(bool* d) : destroyed{d} {}

  ~D2() { *destroyed = true; }
};

BOOST_AUTO_TEST_CASE(AnyMoveTypeChange) {
  BOOST_TEST_CONTEXT("Small type") {
    bool destroyed = false;
    D d{&destroyed};
    Any a{std::move(d)};
    BOOST_CHECK(!destroyed);

    int value = 5;
    Any b{value};
    a = std::move(b);
    BOOST_CHECK(destroyed);
    BOOST_CHECK_EQUAL(a.as<int>(), value);
  }

  bool destroyed = false;
  BOOST_TEST_CONTEXT("Large type") {
    D2 d{&destroyed};
    Any a{std::move(d)};
    BOOST_CHECK(!destroyed);

    int value = 5;
    Any b{value};
    a = std::move(b);
    BOOST_CHECK(destroyed);
    BOOST_CHECK_EQUAL(a.as<int>(), value);
  }
}

BOOST_AUTO_TEST_CASE(AnyCopyTypeChange) {
  BOOST_TEST_CONTEXT("Small type") {
    bool destroyed = false;
    D d{&destroyed};
    Any a{std::move(d)};
    BOOST_CHECK(!destroyed);

    int value = 5;
    Any b{value};
    a = b;
    BOOST_CHECK(destroyed);
    BOOST_CHECK_EQUAL(a.as<int>(), value);
  }

  bool destroyed = false;
  BOOST_TEST_CONTEXT("Large type") {
    D2 d{&destroyed};
    Any a{std::move(d)};
    BOOST_CHECK(!destroyed);

    int value = 5;
    Any b{value};
    a = b;
    BOOST_CHECK(destroyed);
    BOOST_CHECK_EQUAL(a.as<int>(), value);
  }
}

BOOST_AUTO_TEST_CASE(AnyDestroy) {
  {  // small type
    bool destroyed = false;
    D d{&destroyed};
    BOOST_CHECK(!destroyed);
    {
      Any a{std::move(d)};
      BOOST_CHECK(!destroyed);
    }
    BOOST_CHECK(destroyed);
  }
  CHECK_ANY_ALLOCATIONS();

  {  // large type
    bool destroyed = false;
    D2 d{&destroyed};
    BOOST_CHECK(!destroyed);
    {
      Any a{std::move(d)};
      BOOST_CHECK(!destroyed);
    }
    BOOST_CHECK(destroyed);
  }
  CHECK_ANY_ALLOCATIONS();
}

BOOST_AUTO_TEST_CASE(AnyDestroyCopy) {
  {  // small type
    bool destroyed = false;

    {
      Any b;
      {
        Any a{std::in_place_type<D>, &destroyed};
        BOOST_CHECK(!destroyed);
        b = a;
        BOOST_CHECK(!destroyed);
      }
      BOOST_CHECK(destroyed);  // a destroyed, should be true
      destroyed = false;
      BOOST_CHECK(!destroyed);
    }
    BOOST_CHECK(destroyed);  // b destroyed, should be true again
  }
  CHECK_ANY_ALLOCATIONS();

  {  // large type
    bool destroyed = false;

    {
      Any b;
      {
        Any a{std::in_place_type<D2>, &destroyed};
        BOOST_CHECK(!destroyed);
        b = a;
        BOOST_CHECK(!destroyed);
      }
      BOOST_CHECK(destroyed);  // a destroyed, should be true
      destroyed = false;
      BOOST_CHECK(!destroyed);
    }
    BOOST_CHECK(destroyed);  // b destroyed, should be true again
  }
  CHECK_ANY_ALLOCATIONS();
}

BOOST_AUTO_TEST_CASE(AnyDestroyInPlace) {
  {  // small type
    bool destroyed = false;
    BOOST_CHECK(!destroyed);
    {
      Any a{std::in_place_type<D>, &destroyed};
      BOOST_CHECK(!destroyed);
    }
    BOOST_CHECK(destroyed);
  }
  CHECK_ANY_ALLOCATIONS();

  {  // large type
    bool destroyed = false;
    BOOST_CHECK(!destroyed);
    {
      Any a{std::in_place_type<D2>, &destroyed};
      BOOST_CHECK(!destroyed);
    }
    BOOST_CHECK(destroyed);
  }
  CHECK_ANY_ALLOCATIONS();
}

struct D3 {
  std::size_t* destroyed{nullptr};
  std::array<char, 512> blob{};

  D3(std::size_t* d) : destroyed{d} {}

  ~D3() { (*destroyed)++; }
};

BOOST_AUTO_TEST_CASE(LeakCheck) {
  std::size_t destroyed = 0;
  for (std::size_t i = 0; i < 10000; i++) {
    {
      BOOST_CHECK_EQUAL(destroyed, i);
      Any a;
      BOOST_CHECK_EQUAL(destroyed, i);
      a = Any{std::in_place_type<D3>, &destroyed};
      BOOST_CHECK_EQUAL(destroyed, i);
    }
    BOOST_CHECK_EQUAL(destroyed, i + 1);
  }
  CHECK_ANY_ALLOCATIONS();
}

struct LifecycleCounters {
  std::size_t nDestroy = 0;
  std::size_t nCopyConstruct = 0;
  std::size_t nCopy = 0;
  std::size_t nMoveConstruct = 0;
  std::size_t nMove = 0;
};

template <std::size_t PADDING>
struct Lifecycle;

template <>
struct Lifecycle<0> {
  LifecycleCounters* counters;

  Lifecycle(LifecycleCounters* _counters) : counters{_counters} {}

  Lifecycle(Lifecycle&& o) {
    counters = o.counters;
    counters->nMoveConstruct++;
  }

  Lifecycle& operator=(Lifecycle&& o) {
    counters = o.counters;
    counters->nMove++;
    return *this;
  }

  Lifecycle(const Lifecycle& o) {
    counters = o.counters;
    counters->nCopyConstruct++;
  }

  Lifecycle& operator=(const Lifecycle& o) {
    counters = o.counters;
    counters->nCopy++;
    return *this;
  }

  ~Lifecycle() { counters->nDestroy++; }
};

template <std::size_t PADDING>
struct Lifecycle : public Lifecycle<0> {
  std::array<char, PADDING> m_padding{};

  Lifecycle(LifecycleCounters* _counters) : Lifecycle<0>(_counters) {}
};

template <std::size_t PADDING>
struct LifecycleHandle {
  LifecycleCounters counters;
  Lifecycle<PADDING> inner;

  LifecycleHandle() : counters{}, inner{&counters} {}
};

#define checkCounters()                                                      \
  do {                                                                       \
    BOOST_REQUIRE_EQUAL(l.counters.nCopy, counters.nCopy);                   \
    BOOST_REQUIRE_EQUAL(l.counters.nCopyConstruct, counters.nCopyConstruct); \
    BOOST_REQUIRE_EQUAL(l.counters.nMove, counters.nMove);                   \
    BOOST_REQUIRE_EQUAL(l.counters.nMoveConstruct, counters.nMoveConstruct); \
    BOOST_REQUIRE_EQUAL(l.counters.nDestroy, counters.nDestroy);             \
  } while (0)

#define makeCounter(counter, n) \
  do {                          \
    counter += n;               \
    checkCounters();            \
  } while (0)

#define incCopyConstruct(n) makeCounter(counters.nCopyConstruct, n)
#define incCopy(n) makeCounter(counters.nCopy, n)
#define incMoveConstruct(n) makeCounter(counters.nMoveConstruct, n)
#define incMove(n) makeCounter(counters.nMove, n)
#define incDestroy(n) makeCounter(counters.nDestroy, n)

BOOST_AUTO_TEST_CASE(LifeCycleSmall) {
  LifecycleCounters counters;
  LifecycleHandle<0> l;

  checkCounters();

  {
    const auto& o = l.inner;  // force copy
    Any a{o};
    incCopyConstruct(1);

    const auto& _a = a;  // force copy later
    {
      Any b{_a};
      incCopyConstruct(1);
    }
    incDestroy(1);

    {
      Any b;
      b = _a;
      incCopyConstruct(1);
      b = _a;
      incCopy(1);
    }
    incDestroy(1);

    {
      Any b{a};
      incCopyConstruct(1);
      b = a;
      incCopy(1);
    }
    incDestroy(1);

    {
      auto _a2 = a;
      incCopyConstruct(1);
      Any b;
      b = std::move(_a2);
      incMoveConstruct(1);
      auto _a3 = a;
      incCopyConstruct(1);
      b = std::move(_a3);
      incMove(1);
    }
    incDestroy(3);
  }
  incDestroy(1);

  checkCounters();

  CHECK_ANY_ALLOCATIONS();
}

BOOST_AUTO_TEST_CASE(LifeCycleHeap) {
  LifecycleCounters counters;
  LifecycleHandle<512> l;

  checkCounters();

  {
    const auto& o = l.inner;  // force copy
    Any a{o};
    incCopyConstruct(1);

    const auto& _a = a;  // force copy later
    {
      Any b{_a};
      incCopyConstruct(1);
    }
    incDestroy(1);

    {
      Any b;
      b = _a;
      incCopyConstruct(1);
      b = _a;
      incCopy(1);
    }
    incDestroy(1);

    {
      Any b{a};
      incCopyConstruct(1);
      b = a;
      incCopy(1);
    }
    incDestroy(1);

    {
      Any _a2 = a;
      incCopyConstruct(1);
      Any b;
      b = std::move(_a2);
      // no actual move

      Any _a3 = a;
      incCopyConstruct(1);
      b = std::move(_a3);
      // no actual move
      incDestroy(1);
    }
    incDestroy(1);
  }
  incDestroy(1);

  checkCounters();

  CHECK_ANY_ALLOCATIONS();
}

BOOST_AUTO_TEST_SUITE_END()
