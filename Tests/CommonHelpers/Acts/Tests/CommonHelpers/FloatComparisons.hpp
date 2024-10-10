// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"

#include <algorithm>
#include <limits>

// The following assertions can be seen as an extension of the BOOST_CHECK_XYZ
// macros which also support approximate comparisons of containers of floating-
// point numbers. Containers are compared by size and element-wise.
//
// Relative tolerances are fractional: 2e-4 means +/-0.02% from the reference.
//
// Test failures are reported in detail, from the floating-point comparison
// that failed (and the reason why it failed) to the context in which the
// failure occurred (container contents if applicable, source file & line...).

// Check if "val" and "ref" are within relative tolerance "tol" of each other.
#define CHECK_CLOSE_REL(val, ref, reltol) \
  BOOST_CHECK(Acts::Test::checkCloseRel((val), (ref), (reltol)))

// Check if "val" and "ref" are within absolute tolerance "tol" of each other.
// Equivalent to CHECK_SMALL(val - ref), but does not require an operator-().
#define CHECK_CLOSE_ABS(val, ref, abstol) \
  BOOST_CHECK(Acts::Test::checkCloseAbs((val), (ref), (abstol)))

// Check if "val" is below absolute threshold "small".
// Equivalent to CHECK_CLOSE_ABS(val, 0), but does not require a zero value.
#define CHECK_SMALL(val, small) \
  BOOST_CHECK(Acts::Test::checkSmall((val), (small)))

// Start with a relative comparison, but tolerate failures when both the value
// and the reference are below "small". This assertion is useful when comparing
// containers of values and the reference container has zeroes.
#define CHECK_CLOSE_OR_SMALL(val, ref, reltol, small) \
  BOOST_CHECK(Acts::Test::checkCloseOrSmall((val), (ref), (reltol), (small)))

// Covariance matrices require special logic and care because while relative
// comparisons are perfectly appropriate on diagonal terms, they become
// inappropriate on off-diagonal terms, which represent correlations and are
// therefore best compared with respect to the order of magnitude of the
// corresponding diagonal elements.
#define CHECK_CLOSE_COVARIANCE(val, ref, tol) \
  BOOST_CHECK(Acts::Test::checkCloseCovariance((val), (ref), (tol)))

// The relevant infrastructure is implemented below

namespace Acts::Test {
namespace float_compare_internal {

// Under the hood, various scalar comparison logics may be used

using predicate_result = boost::test_tools::predicate_result;

using ScalarComparison = std::function<predicate_result(double, double)>;

ScalarComparison closeOrSmall(double reltol, double small) {
  return [=](double val, double ref) -> predicate_result {
    // Perform the comparison, exit on success
    if (std::abs(ref) >= small) {
      // Reference is large enough for a relative comparison
      if (std::abs(val - ref) < reltol * std::abs(ref)) {
        return true;
      }
    } else if (std::abs(val) < small) {
      // Reference is small and value is small too
      return true;
    }

    // Comparison failed, report why
    predicate_result res(false);
    res.message() << "The floating point value " << val;
    if ((std::abs(ref) < small) || (reltol == 0.)) {
      res.message() << " is above small-ness threshold " << small;
    } else {
      res.message() << " is not within relative tolerance " << reltol
                    << " of reference " << ref;
    }
    res.message() << '.';
    return res;
  };
}

ScalarComparison closeAbs(double abstol) {
  return [=](double val, double ref) -> predicate_result {
    // Perform the comparison, exit on success
    if (std::abs(ref - val) <= abstol) {
      return true;
    }

    // Comparison failed, report why
    predicate_result res(false);
    res.message() << "The floating point value " << val
                  << " is not within absolute tolerance " << abstol
                  << " of reference " << ref << '.';
    return res;
  };
}

// Container comparison is then implemented on top of scalar comparison

// Matrix comparison backend (called by Eigen-related compare() overloads)
template <typename Derived1, typename Derived2>
predicate_result matrixCompare(const Eigen::DenseBase<Derived1>& val,
                               const Eigen::DenseBase<Derived2>& ref,
                               ScalarComparison&& compareImpl) {
  constexpr int rows1 = Eigen::DenseBase<Derived1>::RowsAtCompileTime;
  constexpr int rows2 = Eigen::DenseBase<Derived2>::RowsAtCompileTime;
  constexpr int cols1 = Eigen::DenseBase<Derived1>::ColsAtCompileTime;
  constexpr int cols2 = Eigen::DenseBase<Derived2>::ColsAtCompileTime;

  if constexpr (rows1 != Eigen::Dynamic && rows2 != Eigen::Dynamic &&
                cols1 != Eigen::Dynamic && cols2 != Eigen::Dynamic) {
    // All dimensions on both are static. Static assert compatibility.
    static_assert(rows1 == rows2,
                  "Input matrices do not have the same number of rows");
    static_assert(cols1 == cols2,
                  "Input matrices do not have the same number of columns");
  } else {
    // some are dynamic, do runtime check
    if (val.rows() != ref.rows() || val.cols() != ref.cols()) {
      predicate_result res{false};
      res.message() << "Mismatch in matrix dimensions:\n" << val << "\n" << ref;
      return res;
    }
  }

  // for looping, always use runtime values
  for (int col = 0; col < val.cols(); ++col) {
    for (int row = 0; row < val.rows(); ++row) {
      predicate_result res = compareImpl(val(row, col), ref(row, col));
      if (!res) {
        res.message() << " The failure occurred during a matrix comparison,"
                      << " at index (" << row << ", " << col << ")."
                      << " The value was\n"
                      << val << '\n'
                      << "and the reference was\n"
                      << ref << '\n';
        return res;
      }
    }
  }
  return true;
}

template <typename T>
concept is_eigen_type = requires(const T& t) {
  { t.eval() };
};

// STL container frontend
//
// FIXME: The algorithm only supports ordered containers, so the API should
//        only accept them. Does someone know a clean way to do that in C++?
//
template <typename Container>
predicate_result compare(const Container& val, const Container& ref,
                         ScalarComparison&& compareImpl)
  requires(!is_eigen_type<Container>) && requires(const Container& t) {
    { t.cbegin() };
    { t.cend() };
  }
{
  // Make sure that the two input containers have the same number of items
  // (in order to provide better error reporting when they don't)
  std::size_t numVals = std::distance(std::cbegin(val), std::cend(val));
  std::size_t numRefs = std::distance(std::cbegin(ref), std::cend(ref));
  if (numVals != numRefs) {
    predicate_result res(false);
    res.message() << "The container size does not match (value has " << numVals
                  << " elements, reference has " << numRefs << " elements).";
    return res;
  }

  // Compare the container's contents, bubbling assertion results up. Sadly,
  // this means that we cannot use std::equal.
  auto valBeg = std::cbegin(val);
  auto valIter = valBeg;
  auto valEnd = std::cend(val);
  auto refIter = std::cbegin(ref);
  while (valIter != valEnd) {
    predicate_result res = compareImpl(*valIter, *refIter);
    if (!res) {
      // If content comparison failed, report the container's contents
      res.message() << " The failure occurred during a container comparison,"
                    << " at index " << std::distance(valBeg, valIter) << '.'
                    << " The value contained {";
      for (const auto& item : val) {
        res.message() << ' ' << item << ' ';
      }
      res.message() << "} and the reference contained {";
      for (const auto& item : ref) {
        res.message() << ' ' << item << ' ';
      }
      res.message() << "}.";
      return res;
    }
    ++valIter;
    ++refIter;
  }

  // If the size and contents match, we're good
  return true;
}

// Eigen expression template frontend
template <typename T, typename U>
predicate_result compare(const Eigen::DenseBase<T>& val,
                         const Eigen::DenseBase<U>& ref,
                         ScalarComparison&& compareImpl) {
  return matrixCompare(val.eval(), ref.eval(), std::move(compareImpl));
}

// Eigen transform frontend
predicate_result compare(const Transform3& val, const Transform3& ref,
                         ScalarComparison&& compareImpl) {
  return matrixCompare(val.matrix(), ref.matrix(), std::move(compareImpl));
}

// Scalar frontend
predicate_result compare(double val, double ref,
                         ScalarComparison&& compareImpl) {
  return compareImpl(val, ref);
}
}  // namespace float_compare_internal

// ...and with all that, we can implement the CHECK_XYZ macros

template <typename T, typename U>
boost::test_tools::predicate_result checkCloseRel(const T& val, const U& ref,
                                                  double reltol) {
  using namespace float_compare_internal;
  return compare(val, ref, closeOrSmall(reltol, 0.));
}

template <typename T, typename U>
boost::test_tools::predicate_result checkCloseAbs(const T& val, const U& ref,
                                                  double abstol) {
  using namespace float_compare_internal;
  return compare(val, ref, closeAbs(abstol));
}

template <typename T>
boost::test_tools::predicate_result checkSmall(const T& val, double small) {
  using namespace float_compare_internal;
  return compare(val, val, closeOrSmall(0., small));
}

template <typename T, typename U>
boost::test_tools::predicate_result checkCloseOrSmall(const T& val,
                                                      const U& ref,
                                                      double reltol,
                                                      double small) {
  using namespace float_compare_internal;
  return compare(val, ref, closeOrSmall(reltol, small));
}

template <typename val_t, typename ref_t>
boost::test_tools::predicate_result checkCloseCovariance(
    const Eigen::MatrixBase<val_t>& val, const Eigen::MatrixBase<ref_t>& ref,
    double tol) {
  EIGEN_STATIC_ASSERT_FIXED_SIZE(val_t);
  EIGEN_STATIC_ASSERT_FIXED_SIZE(ref_t);
  EIGEN_STATIC_ASSERT_SAME_MATRIX_SIZE(val_t, ref_t);
  assert(val.cols() == val.rows());
  assert(ref.cols() == ref.rows());

  for (int col = 0; col < val.cols(); ++col) {
    for (int row = col; row < val.rows(); ++row) {
      // For diagonal elements, this is just a regular relative comparison.
      // But for off-diagonal correlation terms, the tolerance scales with the
      // geometric mean of the variance terms that are being correlated.
      //
      // This accounts for the fact that a relatively large correlation
      // difference means little if the overall correlation has a tiny weight
      // with respect to the diagonal variance elements anyway.
      //
      auto orderOfMagnitude = std::sqrt(ref(row, row) * ref(col, col));
      if (std::abs(val(row, col) - ref(row, col)) >= tol * orderOfMagnitude) {
        boost::test_tools::predicate_result res(false);
        res.message() << "The difference between the covariance matrix term "
                      << val(row, col) << " and its reference " << ref(row, col)
                      << ","
                      << " at index (" << row << ", " << col << "),"
                      << " is not within tolerance " << tol * orderOfMagnitude
                      << '.' << " The covariance matrix being tested was\n"
                      << val << '\n'
                      << "and the reference covariance matrix was\n"
                      << ref << '\n';
        return res;
      }
    }
  }
  return true;
}
}  // namespace Acts::Test
