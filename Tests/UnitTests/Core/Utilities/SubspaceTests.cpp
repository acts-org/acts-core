// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Utilities/AlgebraHelpers.hpp"
#include "Acts/Utilities/detail/Subspace.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <ostream>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using namespace Acts;

// meta-programming type list of scalar type + subspace type combinations
// clang-format off
using ScalarsAndFixedSizeSubspaces = std::tuple<
  std::tuple<float, detail::FixedSizeSubspace<2u, 1u>>,
  std::tuple<float, detail::FixedSizeSubspace<2u, 2u>>,
  std::tuple<float, detail::FixedSizeSubspace<3u, 1u>>,
  std::tuple<float, detail::FixedSizeSubspace<3u, 2u>>,
  std::tuple<float, detail::FixedSizeSubspace<3u, 3u>>,
  std::tuple<float, detail::FixedSizeSubspace<6u, 1u>>,
  std::tuple<float, detail::FixedSizeSubspace<6u, 2u>>,
  std::tuple<float, detail::FixedSizeSubspace<6u, 4u>>,
  std::tuple<float, detail::FixedSizeSubspace<8u, 1u>>,
  std::tuple<float, detail::FixedSizeSubspace<8u, 2u>>,
  std::tuple<float, detail::FixedSizeSubspace<8u, 4u>>,
  std::tuple<double, detail::FixedSizeSubspace<2u, 1u>>,
  std::tuple<double, detail::FixedSizeSubspace<2u, 2u>>,
  std::tuple<double, detail::FixedSizeSubspace<3u, 1u>>,
  std::tuple<double, detail::FixedSizeSubspace<3u, 2u>>,
  std::tuple<double, detail::FixedSizeSubspace<3u, 3u>>,
  std::tuple<double, detail::FixedSizeSubspace<6u, 1u>>,
  std::tuple<double, detail::FixedSizeSubspace<6u, 2u>>,
  std::tuple<double, detail::FixedSizeSubspace<6u, 4u>>,
  std::tuple<double, detail::FixedSizeSubspace<8u, 1u>>,
  std::tuple<double, detail::FixedSizeSubspace<8u, 2u>>,
  std::tuple<double, detail::FixedSizeSubspace<8u, 4u>>
>;
// clang-format on

/// Construct a random vector of the specified size.
template <typename scalar_t, std::size_t kSize>
Eigen::Matrix<scalar_t, kSize, 1> makeRandomVector() {
  Eigen::Matrix<scalar_t, kSize, 1> vec;
  vec.setRandom();
  return vec;
}

/// Construct a vector w/ monotonically inceasing values starting at 0.
std::vector<std::size_t> makeMonotonicIndices(std::size_t n) {
  std::vector<std::size_t> indices(n);
  std::iota(indices.begin(), indices.end(), 0u);
  return indices;
}

/// Build a sorted array from the first kSize indices.
template <std::size_t kSize>
std::array<std::size_t, kSize> selectFixedIndices(
    const std::vector<std::size_t>& fullIndices) {
  std::array<std::size_t, kSize> indices{};
  for (auto i = 0u; i < kSize; ++i) {
    indices[i] = fullIndices[i];
  }
  std::ranges::sort(indices);
  return indices;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(UtilitiesSubspace)

// all these cases should lead to compile-time errors
// BOOST_AUTO_TEST_CASE(ConstructFixedInvalid) {
//   {
//     using Subspace = detail::FixedSizeSubspace<6u, 7u>;
//     Subspace subspace(0u, 1u, 2u, 3u, 4u, 5u, 6u);
//   }
//   {
//     using Subspace = detail::FixedSizeSubspace<8u, 9u>;
//     Subspace subspace(0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u);
//   }
// }

BOOST_AUTO_TEST_CASE_TEMPLATE(FixedSizeSubspace, ScalarAndSubspace,
                              ScalarsAndFixedSizeSubspaces) {
  // extract the test types
  using Scalar = std::tuple_element_t<0, ScalarAndSubspace>;
  using Subspace = std::tuple_element_t<1, ScalarAndSubspace>;

  auto x = makeRandomVector<Scalar, Subspace::fullSize()>();
  auto fullIndices = makeMonotonicIndices(Subspace::fullSize());

  // in principle, we would like to iterate over all possible ordered subsets
  // from the full space indices with a size identical to the subspace. since i
  // do not know how to do that in a simple manner, we are iterating over all
  // permutations of the full indices and pick the first n elements. this should
  // give a reasonable set of different subspace configurations.
  do {
    auto indices = selectFixedIndices<Subspace::size()>(fullIndices);
    Subspace subspace(indices);

    // verify projector/expander consistency
    BOOST_CHECK_EQUAL(subspace.template projector<Scalar>().transpose(),
                      subspace.template expander<Scalar>());
    BOOST_CHECK_EQUAL(subspace.template expander<Scalar>().transpose(),
                      subspace.template projector<Scalar>());
    // project into the subspace
    auto s0 = subspace.projectVector(x);
    auto s1 = (subspace.template projector<Scalar>() * x).eval();
    for (auto i = 0u; i < subspace.size(); ++i) {
      BOOST_TEST_INFO("Checking projected subspace component " << i);
      BOOST_CHECK_EQUAL(s0[i], x[indices[i]]);
      BOOST_CHECK_EQUAL(s1[i], x[indices[i]]);
    }
    // expand from the subspace back into the full space
    auto y0 = subspace.expandVector(s1);
    auto y1 = (subspace.template expander<Scalar>() * s0).eval();
    for (auto i = 0u; i < subspace.fullSize(); ++i) {
      BOOST_TEST_INFO("Checking expanded fullspace component " << i);
      BOOST_CHECK_EQUAL(y0[i], subspace.contains(i) ? x[i] : 0);
      BOOST_CHECK_EQUAL(y1[i], subspace.contains(i) ? x[i] : 0);
    }
  } while (std::next_permutation(fullIndices.begin(), fullIndices.end()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(VariableSizeSubspace, ScalarAndSubspace,
                              ScalarsAndFixedSizeSubspaces) {
  // extract the test types
  using Scalar = std::tuple_element_t<0, ScalarAndSubspace>;
  using FixedSubspace = std::tuple_element_t<1, ScalarAndSubspace>;
  using VariableSubspace =
      detail::VariableSizeSubspace<FixedSubspace::fullSize()>;

  auto fullIndices = makeMonotonicIndices(FixedSubspace::fullSize());

  // in principle, we would like to iterate over all possible ordered subsets
  // from the full space indices with a size identical to the subspace. since i
  // do not know how to do that in a simple manner, we are iterating over all
  // permutations of the full indices and pick the first n elements. this should
  // give a reasonable set of different subspace configurations.
  do {
    auto indices = selectFixedIndices<FixedSubspace::size()>(fullIndices);
    FixedSubspace fixedSubspace(indices);
    VariableSubspace variableSubspace(indices);

    BOOST_CHECK_EQUAL(variableSubspace.size(), fixedSubspace.size());
    BOOST_CHECK_EQUAL(variableSubspace.fullSize(), fixedSubspace.fullSize());

    auto fixedProjector = fixedSubspace.template projector<Scalar>();

    Eigen::Matrix<Scalar, FixedSubspace::fullSize(), FixedSubspace::fullSize()>
        fixedFullProjector;
    fixedFullProjector.setZero();
    fixedFullProjector.template topLeftCorner<FixedSubspace::size(),
                                              FixedSubspace::fullSize()>() =
        fixedProjector;

    auto variableFullProjector =
        variableSubspace.template fullProjector<Scalar>();

    BOOST_CHECK_EQUAL(variableFullProjector, fixedFullProjector);
  } while (std::next_permutation(fullIndices.begin(), fullIndices.end()));
}

BOOST_AUTO_TEST_SUITE_END()
