// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/ExaTrkX/detail/TensorVectorConversion.hpp"

#include <iostream>

#include <torch/torch.h>

BOOST_AUTO_TEST_CASE(test_vector_tensor_conversion_int_2cols) {
  std::vector<std::int64_t> start_vec = {
      // clang-format off
    0, 1,
    1, 2,
    2, 3,
    3, 4
      // clang-format on
  };

  auto tensor = Acts::detail::vectorToTensor2D(start_vec, 2).clone();

  BOOST_CHECK_EQUAL(tensor.options().dtype(), torch::kInt64);
  BOOST_CHECK_EQUAL(tensor.sizes().size(), 2);
  BOOST_CHECK_EQUAL(tensor.size(0), 4);
  BOOST_CHECK_EQUAL(tensor.size(1), 2);

  BOOST_CHECK_EQUAL(tensor[0][0].item<std::int64_t>(), 0);
  BOOST_CHECK_EQUAL(tensor[0][1].item<std::int64_t>(), 1);

  BOOST_CHECK_EQUAL(tensor[1][0].item<std::int64_t>(), 1);
  BOOST_CHECK_EQUAL(tensor[1][1].item<std::int64_t>(), 2);

  BOOST_CHECK_EQUAL(tensor[2][0].item<std::int64_t>(), 2);
  BOOST_CHECK_EQUAL(tensor[2][1].item<std::int64_t>(), 3);

  BOOST_CHECK_EQUAL(tensor[3][0].item<std::int64_t>(), 3);
  BOOST_CHECK_EQUAL(tensor[3][1].item<std::int64_t>(), 4);

  auto test_vec = Acts::detail::tensor2DToVector<std::int64_t>(tensor);

  BOOST_CHECK_EQUAL(test_vec, start_vec);
}

BOOST_AUTO_TEST_CASE(test_vector_tensor_conversion_float_3cols) {
  std::vector<float> start_vec = {
      // clang-format off
    0.f, 0.f, 0.f,
    1.f, 1.f, 1.f,
    2.f, 2.f, 2.f,
    3.f, 3.f, 3.f
      // clang-format on
  };

  auto tensor = Acts::detail::vectorToTensor2D(start_vec, 3).clone();

  BOOST_CHECK_EQUAL(tensor.options().dtype(), torch::kFloat32);
  BOOST_CHECK_EQUAL(tensor.sizes().size(), 2);
  BOOST_CHECK_EQUAL(tensor.size(0), 4);
  BOOST_CHECK_EQUAL(tensor.size(1), 3);

  for (auto i : {0, 1, 2, 3}) {
    BOOST_CHECK_EQUAL(tensor[i][0].item<std::int64_t>(), static_cast<float>(i));
    BOOST_CHECK_EQUAL(tensor[i][1].item<std::int64_t>(), static_cast<float>(i));
    BOOST_CHECK_EQUAL(tensor[i][2].item<std::int64_t>(), static_cast<float>(i));
  }

  auto test_vec = Acts::detail::tensor2DToVector<float>(tensor);

  BOOST_CHECK_EQUAL(test_vec, start_vec);
}

BOOST_AUTO_TEST_CASE(test_slicing) {
  std::vector<float> start_vec = {
      // clang-format off
    0.f, 4.f, 0.f,
    1.f, 5.f, 1.f,
    2.f, 6.f, 2.f,
    3.f, 7.f, 3.f
      // clang-format on
  };

  auto tensor = Acts::detail::vectorToTensor2D(start_vec, 3).clone();

  using namespace torch::indexing;
  tensor = tensor.index({Slice{}, Slice{0, None, 2}});

  BOOST_CHECK_EQUAL(tensor.size(0), 4);
  BOOST_CHECK_EQUAL(tensor.size(1), 2);

  const std::vector<float> ref_vec = {
      // clang-format off
    0.f, 0.f,
    1.f, 1.f,
    2.f, 2.f,
    3.f, 3.f,
      // clang-format on
  };

  const auto test_vec = Acts::detail::tensor2DToVector<float>(tensor);

  BOOST_CHECK_EQUAL(test_vec, ref_vec);
}
