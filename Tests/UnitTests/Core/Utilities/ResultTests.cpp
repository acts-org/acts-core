// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/Result.hpp"

#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

using namespace std::string_literals;

namespace {

enum class MyError {
  Failure = 1,
  SomethingElse,
};

std::error_code make_error_code(MyError e) {
  return {static_cast<int>(e), std::generic_category()};
}

}  // namespace

namespace std {
// register with STL
template <>
struct is_error_code_enum<MyError> : std::true_type {};
}  // namespace std

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Utilities)

BOOST_AUTO_TEST_CASE(TestConstruction) {
  {
    using Result = Result<int, char>;

    Result res = Result::success(42);
    BOOST_CHECK_EQUAL(*res, 42);
    BOOST_CHECK_EQUAL(res.value(), 42);
    BOOST_CHECK(res.ok());
    res = Result::success('e');
    BOOST_CHECK_EQUAL(*res, 'e');
    BOOST_CHECK_EQUAL(res.value(), 'e');
    BOOST_CHECK(res.ok());
    res = Result::failure(42);
    BOOST_CHECK(!res.ok());
    BOOST_CHECK_EQUAL(res.error(), 42);
    BOOST_CHECK_THROW(res.value(), std::runtime_error);
    res = Result::failure('e');
    BOOST_CHECK(!res.ok());
    BOOST_CHECK_EQUAL(res.error(), 'e');
    BOOST_CHECK_THROW(res.value(), std::runtime_error);
  }

  {
    using Result = Result<double, std::string>;

    Result res1("hallo");
    BOOST_CHECK(!res1.ok());
    BOOST_CHECK_EQUAL(res1.error(), "hallo");
    BOOST_CHECK_THROW(res1.value(), std::runtime_error);
    res1 = Result::failure("hallo");
    BOOST_CHECK(!res1.ok());
    BOOST_CHECK_EQUAL(res1.error(), "hallo");
    BOOST_CHECK_THROW(res1.value(), std::runtime_error);

    Result res2(4.5);
    BOOST_CHECK(res2.ok());
    BOOST_CHECK(*res2 == 4.5);
    BOOST_CHECK(res2.value() == 4.5);
    res2 = Result::success(4.5);
    BOOST_CHECK(res2.ok());
    BOOST_CHECK_EQUAL(*res2, 4.5);
    BOOST_CHECK_EQUAL(res2.value(), 4.5);
  }
}

BOOST_AUTO_TEST_CASE(TestErrorCodes) {
  auto err1 = MyError::Failure;

  std::error_code ec = err1;

  {
    using Result = Result<double, MyError>;

    Result res(42.);
    BOOST_CHECK(res.ok());
    BOOST_CHECK_EQUAL(*res, 42.);

    Result res2(err1);
    BOOST_CHECK(!res2.ok());
    BOOST_CHECK_EQUAL(res2.error(), err1);
    BOOST_CHECK_THROW(res2.value(), std::runtime_error);
  }

  {
    using Result = Result<double, std::error_code>;

    Result res(42.);
    BOOST_CHECK(res.ok());
    BOOST_CHECK_EQUAL(*res, 42.);
    BOOST_CHECK_EQUAL(res.value(), 42u);
    res = 46.;
    BOOST_CHECK(res.ok());
    BOOST_CHECK_EQUAL(*res, 46.);
    BOOST_CHECK_EQUAL(res.value(), 46u);

    Result res2(ec);
    BOOST_CHECK(!res2.ok());
    BOOST_CHECK_EQUAL(res2.error(), ec);
    BOOST_CHECK_EQUAL(res2.error(), err1);

    res2 = MyError::SomethingElse;
    BOOST_CHECK(!res2.ok());
    BOOST_CHECK_EQUAL(res2.error(), MyError::SomethingElse);
    BOOST_CHECK_NE(res2.error(), MyError::Failure);
  }

  {
    using Result = Result<double, const char*>;
    Result res{0.};
    BOOST_CHECK(res.ok());
    BOOST_CHECK_EQUAL(*res, 0.0);

    res = 1.;
    BOOST_CHECK(res.ok());
    BOOST_CHECK_EQUAL(*res, 1.0);

    Result res2{"blubb"};
    BOOST_CHECK(!res2.ok());
    BOOST_CHECK_EQUAL(res2.error(), "blubb");
    res2 = "sep";
    BOOST_CHECK(!res2.ok());
    BOOST_CHECK_EQUAL(res2.error(), "sep");
  }

  {
    using Result = Result<const char*, double>;
    Result res{0.};
    BOOST_CHECK(!res.ok());
    BOOST_CHECK_EQUAL(res.error(), 0.0);

    res = "blibb";
    BOOST_CHECK(res.ok());
    BOOST_CHECK_EQUAL(res.value(), "blibb");

    Result res2{"blibb"};
    BOOST_CHECK(res2.ok());
    BOOST_CHECK_EQUAL(res2.value(), "blibb");

    res2 = 0.;
    BOOST_CHECK(!res2.ok());
    BOOST_CHECK_EQUAL(res2.error(), 0.0);
  }

  {
    using Result = Result<double, int>;
    Result res = Result::success(2);
    BOOST_CHECK(res.ok());
    BOOST_CHECK_EQUAL(res.value(), 2.0);

    res = Result::failure(3);
    BOOST_CHECK(!res.ok());
    BOOST_CHECK_EQUAL(res.error(), 3);

    Result res2 = Result::failure(2);
    BOOST_CHECK(!res2.ok());
    BOOST_CHECK_EQUAL(res2.error(), 2);

    res2 = Result::success(3.3);
    BOOST_CHECK(res2.ok());
    BOOST_CHECK_EQUAL(res2.value(), 3.3);
  }

  {
    using Result = Result<double>;

    Result res(42.);
    BOOST_CHECK(res.ok());
    BOOST_CHECK_EQUAL(*res, 42.);
    BOOST_CHECK_EQUAL(res.value(), 42u);
    res = 46.;
    BOOST_CHECK(res.ok());
    BOOST_CHECK_EQUAL(*res, 46.);
    BOOST_CHECK_EQUAL(res.value(), 46u);

    Result res2(ec);
    BOOST_CHECK(!res2.ok());
    BOOST_CHECK_EQUAL(res2.error(), ec);
    BOOST_CHECK_EQUAL(res2.error(), err1);

    res2 = MyError::SomethingElse;
    BOOST_CHECK(!res2.ok());
    BOOST_CHECK_EQUAL(res2.error(), MyError::SomethingElse);
    BOOST_CHECK_NE(res2.error(), MyError::Failure);
  }

  {
    using Result = Result<std::string>;

    Result res("hallo");

    BOOST_CHECK(res.ok());
    BOOST_CHECK_EQUAL(*res, "hallo");
    BOOST_CHECK_EQUAL(res.value(), "hallo");

    res = "something else";
    BOOST_CHECK(res.ok());
    BOOST_CHECK_EQUAL(*res, "something else");
    BOOST_CHECK_EQUAL(res.value(), "something else");

    res = MyError::SomethingElse;
    BOOST_CHECK(!res.ok());
    BOOST_CHECK_EQUAL(res.error(), MyError::SomethingElse);
    BOOST_CHECK_NE(res.error(), MyError::Failure);
  }

  {
    using Result = Result<std::string, int>;

    Result res{"hallo"};
    BOOST_CHECK(res.ok());
    BOOST_CHECK_EQUAL(res.value(), "hallo");

    Result res2{4};
    BOOST_CHECK(!res2.ok());
    BOOST_CHECK_EQUAL(res2.error(), 4);
  }
}

struct NoCopy {
  NoCopy(int i) : num(i) {}
  NoCopy(const NoCopy&) = delete;
  NoCopy& operator=(const NoCopy&) = delete;
  NoCopy(NoCopy&&) = default;
  NoCopy& operator=(NoCopy&&) = default;

  int num;
};

Result<NoCopy> make_nocopy(int i, bool v = true) {
  if (!v) {
    return MyError::Failure;
  }
  return i;
}

BOOST_AUTO_TEST_CASE(CopyBehaviour) {
  using Result = Result<NoCopy>;

  NoCopy n(5);
  Result res = std::move(n);
  BOOST_CHECK(res.ok());
  BOOST_CHECK_EQUAL((*res).num, res.value().num);

  res = make_nocopy(3);
  BOOST_CHECK(res.ok());
  BOOST_CHECK_EQUAL((*res).num, res.value().num);
  BOOST_CHECK_EQUAL((*res).num, 3);

  res = NoCopy(-4);
  BOOST_CHECK(res.ok());
  BOOST_CHECK_EQUAL((*res).num, res.value().num);
  BOOST_CHECK_EQUAL((*res).num, -4.);

  NoCopy n2 = make_nocopy(7).value();
  BOOST_CHECK_EQUAL(n2.num, 7);
  BOOST_REQUIRE_THROW(make_nocopy(6, false).value();, std::runtime_error);

  Result n4r = make_nocopy(8);
  BOOST_CHECK(n4r.ok());
  BOOST_CHECK_EQUAL((*n4r).num, 8);
  NoCopy n4 = std::move(n4r.value());
  BOOST_CHECK_EQUAL(n4.num, 8);
}

Result<void> void_res_func(int b) {
  if (b > 5) {
    return MyError::SomethingElse;
  }
  return {};
}

BOOST_AUTO_TEST_CASE(VoidResult) {
  using Result = Result<void>;

  Result res;
  BOOST_CHECK(res.ok());

  Result res2 = Result::success();
  BOOST_CHECK(res2.ok());

  res = MyError::Failure;
  BOOST_CHECK(!res.ok());
  BOOST_CHECK_EQUAL(res.error(), MyError::Failure);

  Result res3 = Result::failure(MyError::SomethingElse);
  BOOST_CHECK(!res3.ok());
  BOOST_CHECK_EQUAL(res3.error(), MyError::SomethingElse);

  Result res4 = void_res_func(4);
  BOOST_CHECK(res4.ok());

  Result res5 = void_res_func(42);
  BOOST_CHECK(!res5.ok());
  BOOST_CHECK_EQUAL(res5.error(), MyError::SomethingElse);
}

BOOST_AUTO_TEST_CASE(BoolResult) {
  using Result = Result<bool>;

  Result res = Result::success(false);
  BOOST_CHECK(res.ok());
  BOOST_CHECK_EQUAL(*res, false);

  res = Result::success(true);
  BOOST_CHECK(res.ok());
  BOOST_CHECK_EQUAL(*res, true);

  res = Result::failure(MyError::Failure);
  BOOST_CHECK(!res.ok());
  BOOST_CHECK_EQUAL(res.error(), MyError::Failure);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
