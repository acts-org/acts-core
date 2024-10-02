// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Delegate.hpp"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace Acts;

BOOST_AUTO_TEST_SUITE(DelegateTests)

int sumImpl(int a, int b) {
  return a + b;
}

BOOST_AUTO_TEST_CASE(ConnectConstexprLambda) {
  Delegate<int(int, int)> sum;
  BOOST_CHECK(!sum);
  BOOST_CHECK(!sum.connected());

  sum.connect<&sumImpl>();

  BOOST_CHECK_EQUAL(sum(2, 5), 7);
  BOOST_CHECK_NE(sum(2, 3), 7);

  sum.connect([](const void*, int a, int b) -> int { return a + b; });

  BOOST_CHECK(sum);
  BOOST_CHECK(sum.connected());

  BOOST_CHECK_EQUAL(sum(2, 5), 7);
  BOOST_CHECK_NE(sum(2, 3), 7);
}

float multiply(float a, float b) {
  return a * b;
}

BOOST_AUTO_TEST_CASE(ConnectFunctionPointer) {
  Delegate<float(float, float)> mult;

  BOOST_CHECK(!mult);
  BOOST_CHECK(!mult.connected());

  mult.connect<multiply>();

  BOOST_CHECK(mult);
  BOOST_CHECK(mult.connected());

  CHECK_CLOSE_REL(mult(2, 5.9), 2 * 5.9, 1e-6);
  BOOST_CHECK_NE(mult(2, 3.2), 58.9);
}

struct Subtractor {
  int v;
  int execute(int a) const { return a - v; }
};

BOOST_AUTO_TEST_CASE(ConnectStruct) {
  Delegate<int(int)> sub;

  BOOST_CHECK(!sub);
  BOOST_CHECK(!sub.connected());

  Subtractor s{18};
  sub.connect<&Subtractor::execute>(&s);

  BOOST_CHECK(sub);
  BOOST_CHECK(sub.connected());

  BOOST_CHECK_EQUAL(sub(7), 7 - 18);
}

int addition(const void* /*payload*/, int a, int b) {
  return a + b;
}

BOOST_AUTO_TEST_CASE(ConnectRuntime) {
  {
    Delegate<int(int, int)> add;
    BOOST_CHECK(!add);
    BOOST_CHECK(!add.connected());

    add.connect(&addition);
    BOOST_CHECK(add);
    BOOST_CHECK(add.connected());

    BOOST_CHECK_EQUAL(add(4, 4), 8);
  }

  {
    Delegate<int(int, int)> add{&addition};

    BOOST_CHECK(add);
    BOOST_CHECK(add.connected());

    BOOST_CHECK_EQUAL(add(4, 4), 8);
  }

  {
    Delegate<int(int, int)> add;
    BOOST_CHECK(!add);
    BOOST_CHECK(!add.connected());

    add = &addition;
    BOOST_CHECK(add);
    BOOST_CHECK(add.connected());

    BOOST_CHECK_EQUAL(add(4, 4), 8);
  }
}

BOOST_AUTO_TEST_CASE(ConnectConstructFuncPtr) {
  Delegate<int(int, int)> add{DelegateFuncTag<&sumImpl>{}};
  BOOST_CHECK(add);
  BOOST_CHECK(add.connected());
  BOOST_CHECK_EQUAL(add(4, 4), 8);

  Subtractor s{18};
  Delegate<int(int)> sub{DelegateFuncTag<&Subtractor::execute>{}, &s};

  BOOST_CHECK(sub);
  BOOST_CHECK(sub.connected());

  BOOST_CHECK_EQUAL(sub(7), 7 - 18);
}

void modify(int& v, int a) {
  v = a;
}

void noModify(int v, int a) {
  (void)v;
  v = a;
}

BOOST_AUTO_TEST_CASE(DelegateReference) {
  Delegate<void(int&, int)> d;
  d.connect<&modify>();

  int v = 0;
  d(v, 42);
  BOOST_CHECK_EQUAL(v, 42);

  // This should not compile since the signature is not exactly matched
  // d.connect<&noModify>();
}

struct SignatureTest {
  void modify(int& v, int a) const { v = a; }

  void noModify(int v, int a) const {
    (void)v;
    v = a;
  }
};

BOOST_AUTO_TEST_CASE(DelegateReferenceMember) {
  SignatureTest s;
  Delegate<void(int&, int)> d;
  d.connect<&SignatureTest::modify>(&s);

  int v = 0;
  d(v, 42);
  BOOST_CHECK_EQUAL(v, 42);

  // This should not compile since the signature is not exactly matched
  // d.connect<&SignatureTest::noModify>(&s);
}

BOOST_AUTO_TEST_CASE(StatefullLambdas) {
  std::vector<int> v;

  auto lambda = [&](int n) -> int {
    v.push_back(n);
    return v.size();
  };

  Delegate<int(int)> d(lambda);

  BOOST_CHECK(d);
  BOOST_CHECK(d.connected());
  BOOST_CHECK_EQUAL(d(2), 1);

  d.disconnect();
  d = lambda;

  BOOST_CHECK(d);
  BOOST_CHECK(d.connected());
  BOOST_CHECK_EQUAL(d(2), 2);

  d.disconnect();
  d.connect(lambda);

  BOOST_CHECK(d);
  BOOST_CHECK(d.connected());
  BOOST_CHECK_EQUAL(d(2), 3);

  // This should not compile because of deleted && overloads
  // d.connect([&](int a){ v.push_back(a); return v.size(); });
}

struct CheckDestructor {
  CheckDestructor(bool* _out) : destructorCalled{_out} {}

  bool* destructorCalled;

  int func() const { return 4; }

  ~CheckDestructor() { (*destructorCalled) = true; }
};

int owningTest() {
  return 8;
}

int owningTest2(const void* /*payload*/) {
  return 8;
}

BOOST_AUTO_TEST_CASE(OwningDelegateTest) {
  {
    auto s = std::make_unique<const SignatureTest>();
    Delegate<void(int&, int)> d;
    (void)d;
    // This should not compile, as it would be a memory leak
    // d.connect<&SignatureTest::modify>(std::move(s));
  }

  {
    bool destructorCalled = false;
    auto s = std::make_unique<const CheckDestructor>(&destructorCalled);
    {
      BOOST_CHECK_EQUAL(destructorCalled, false);
      Delegate<int(), void, DelegateType::NonOwning> d;
      BOOST_CHECK_EQUAL(destructorCalled, false);
      d.connect<&CheckDestructor::func>(s.get());
      BOOST_CHECK_EQUAL(destructorCalled, false);
      Delegate<int(), void, DelegateType::NonOwning> dCopy{d};
      BOOST_CHECK_EQUAL(d(), 4);
      BOOST_CHECK_EQUAL(dCopy(), 4);
      BOOST_CHECK_EQUAL(destructorCalled, false);
    }
    // destructor not called after non-owning delegate goes out of scope
    BOOST_CHECK_EQUAL(destructorCalled, false);

    {
      BOOST_CHECK_EQUAL(destructorCalled, false);
      Delegate<int(), void, DelegateType::Owning> d;
      // This doesn't compile: owning delegate is not copyable
      // Delegate<int(), DelegateType::Owning> dCopy = d;
      BOOST_CHECK_EQUAL(destructorCalled, false);
      // This doesn't compile: owning delegate cannot accept raw pointer
      // instance
      // d.connect<&CheckDestructor::func>(s.get());
      d.connect<&CheckDestructor::func>(std::move(s));
      BOOST_CHECK_EQUAL(destructorCalled, false);
      BOOST_CHECK_EQUAL(d(), 4);
      BOOST_CHECK_EQUAL(destructorCalled, false);
    }
    // destructor called after owning delegate goes out of scope
    BOOST_CHECK_EQUAL(destructorCalled, true);

    destructorCalled = false;
    s = std::make_unique<const CheckDestructor>(&destructorCalled);
    {
      BOOST_CHECK_EQUAL(destructorCalled, false);
      OwningDelegate<int()> d;
      // This doesn't compile: owning delegate is not copyable
      // OwningDelegate<int()> dCopy = d;
      BOOST_CHECK_EQUAL(destructorCalled, false);
      d.connect<&CheckDestructor::func>(std::move(s));
      BOOST_CHECK_EQUAL(destructorCalled, false);
      BOOST_CHECK_EQUAL(d(), 4);
      BOOST_CHECK_EQUAL(destructorCalled, false);
    }
    // destructor called after owning delegate goes out of scope
    BOOST_CHECK_EQUAL(destructorCalled, true);
  }

  {
    bool destructorCalled = false;
    auto s = std::make_unique<const CheckDestructor>(&destructorCalled);
    {
      BOOST_CHECK_EQUAL(destructorCalled, false);
      Delegate<int(), void, DelegateType::NonOwning> d;
      BOOST_CHECK_EQUAL(destructorCalled, false);
      d.connect<&CheckDestructor::func>(s.get());
      Delegate<int(), void, DelegateType::NonOwning> dCopy{d};
      BOOST_CHECK_EQUAL(destructorCalled, false);
      BOOST_CHECK_EQUAL(d(), 4);
      BOOST_CHECK_EQUAL(dCopy(), 4);
      BOOST_CHECK_EQUAL(destructorCalled, false);
      d.disconnect();
      BOOST_CHECK_EQUAL(destructorCalled, false);
    }

    {
      BOOST_CHECK_EQUAL(destructorCalled, false);
      Delegate<int(), void, DelegateType::Owning> d;
      // This doesn't compile: owning delegate is not copyable
      // Delegate<int(), DelegateType::Owning> dCopy = d;
      BOOST_CHECK_EQUAL(destructorCalled, false);
      // This doesn't compile: owning delegate cannot accept raw pointer
      // instance
      // d.connect<&CheckDestructor::func>(s.get());
      d.connect<&CheckDestructor::func>(std::move(s));
      BOOST_CHECK_EQUAL(destructorCalled, false);
      BOOST_CHECK_EQUAL(d(), 4);
      BOOST_CHECK_EQUAL(destructorCalled, false);
      d.disconnect();
      BOOST_CHECK_EQUAL(destructorCalled, true);
    }
    // destructor called after owning delegate goes out of scope
    BOOST_CHECK_EQUAL(destructorCalled, true);
  }

  {
    OwningDelegate<int()> d;
    d.connect<&owningTest>();
    BOOST_CHECK_EQUAL(d(), 8);

    d.disconnect();
    d.connect<&owningTest>();
    BOOST_CHECK_EQUAL(d(), 8);

    d.disconnect();
    d.connect(owningTest2);
    BOOST_CHECK_EQUAL(d(), 8);
  }
}

struct DelegateInterface {
  DelegateInterface() = default;
  virtual ~DelegateInterface() = 0;

  virtual std::string func() const { return "base"; }
};
inline DelegateInterface::~DelegateInterface() = default;

struct ConcreteDelegate : public DelegateInterface {
  std::string func() const final { return "derived"; }
};

struct SeparateDelegate {
  std::string func() const { return "separate"; }
};

BOOST_AUTO_TEST_CASE(NonVoidDelegateTest) {
  // check void behavior with virtuals
  {
    Delegate<std::string(), void> d;
    ConcreteDelegate c;
    d.connect<&ConcreteDelegate::func>(&c);
    BOOST_CHECK_EQUAL(d(), "derived");

    // does not compile: delegate won't hand out void pointer
    // d.instance();
  }
  {
    Delegate<std::string(), void> d;
    ConcreteDelegate c;
    d.connect<&DelegateInterface::func>(&c);
    BOOST_CHECK_EQUAL(
        d(), "derived");  // <- even if you plug in the base class member
                          // pointer you get the derived class call
  }

  {
    Delegate<std::string(), DelegateInterface> d;
    ConcreteDelegate c;
    d.connect<&ConcreteDelegate::func>(&c);
    BOOST_CHECK_EQUAL(d(), "derived");

    const auto* instance = d.instance();
    static_assert(
        std::is_same_v<
            std::remove_const_t<std::remove_pointer_t<decltype(instance)>>,
            DelegateInterface>,
        "Did not get correct instance pointer");
    BOOST_CHECK_NE(dynamic_cast<const DelegateInterface*>(instance), nullptr);
    BOOST_CHECK_NE(dynamic_cast<const ConcreteDelegate*>(instance), nullptr);
  }

  {
    Delegate<std::string(), ConcreteDelegate> d;
    ConcreteDelegate c;
    d.connect<&ConcreteDelegate::func>(&c);
    BOOST_CHECK_EQUAL(d(), "derived");

    const auto* instance = d.instance();
    static_assert(
        std::is_same_v<
            std::remove_const_t<std::remove_pointer_t<decltype(instance)>>,
            ConcreteDelegate>,
        "Did not get correct instance pointer");
    BOOST_CHECK_NE(dynamic_cast<const DelegateInterface*>(instance), nullptr);
    BOOST_CHECK_NE(dynamic_cast<const ConcreteDelegate*>(instance), nullptr);
  }

  {
    OwningDelegate<std::string(), DelegateInterface> d;
    d.connect<&ConcreteDelegate::func>(
        std::make_unique<const ConcreteDelegate>());
    BOOST_CHECK_EQUAL(d(), "derived");

    const auto* instance = d.instance();
    static_assert(
        std::is_same_v<
            std::remove_const_t<std::remove_pointer_t<decltype(instance)>>,
            DelegateInterface>,
        "Did not get correct instance pointer");
    BOOST_CHECK_NE(dynamic_cast<const DelegateInterface*>(instance), nullptr);
    BOOST_CHECK_NE(dynamic_cast<const ConcreteDelegate*>(instance), nullptr);
  }

  {
    OwningDelegate<std::string(), ConcreteDelegate> d;
    ConcreteDelegate c;
    d.connect<&ConcreteDelegate::func>(
        std::make_unique<const ConcreteDelegate>());
    BOOST_CHECK_EQUAL(d(), "derived");

    const auto* instance = d.instance();
    static_assert(
        std::is_same_v<
            std::remove_const_t<std::remove_pointer_t<decltype(instance)>>,
            ConcreteDelegate>,
        "Did not get correct instance pointer");
    BOOST_CHECK_NE(dynamic_cast<const DelegateInterface*>(instance), nullptr);
    BOOST_CHECK_NE(dynamic_cast<const ConcreteDelegate*>(instance), nullptr);
  }

  {
    Delegate<std::string(), DelegateInterface> d;
    SeparateDelegate c;
    // Does not compile: cannot assign unrelated type
    // d.connect<&SeparateDelegate::func>(&c);
    (void)d;
    (void)c;
  }

  { OwningDelegate<std::string(), DelegateInterface> d; }
}

BOOST_AUTO_TEST_SUITE_END()
