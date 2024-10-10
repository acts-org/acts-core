// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <vector>

#include <torch/torch.h>

namespace Acts::detail {

/// So far this is only needed for integers
template <typename T>
struct TorchTypeMap {};

template <>
struct TorchTypeMap<std::int64_t> {
  constexpr static torch::Dtype type = torch::kInt64;
};

template <>
struct TorchTypeMap<std::int32_t> {
  constexpr static torch::Dtype type = torch::kInt32;
};

template <>
struct TorchTypeMap<std::int16_t> {
  constexpr static torch::Dtype type = torch::kInt16;
};

template <>
struct TorchTypeMap<std::int8_t> {
  constexpr static torch::Dtype type = torch::kInt8;
};

template <>
struct TorchTypeMap<float> {
  constexpr static torch::Dtype type = torch::kFloat32;
};

template <>
struct TorchTypeMap<double> {
  constexpr static torch::Dtype type = torch::kFloat64;
};

/// Converts vector to 2D tensor
/// Make sure your vector has a even number of elements!
/// @Note Input must be mutable, due to torch API.
/// @Note Tensor does not take ownership! `.clone()` afterwards to get
/// ownership of the data
template <typename T>
at::Tensor vectorToTensor2D(std::vector<T> &vec, std::size_t cols) {
  assert(vec.size() % cols == 0);

  auto opts =
      at::TensorOptions().dtype(TorchTypeMap<T>::type).device(torch::kCPU);

  return torch::from_blob(
      vec.data(),
      {static_cast<long>(vec.size() / cols), static_cast<long>(cols)}, opts);
}

/// Converts 2D tensor to vector
/// @Note Automatically converts tensor to target type!
template <typename T>
std::vector<T> tensor2DToVector(const at::Tensor &tensor) {
  assert(tensor.sizes().size() == 2);

  // clone to make sure we own the data
  // bring to CPU
  // convert to requested type
  // ensure the tensor is contiguous (e.g. not the case if indexed with step)

  at::Tensor transformedTensor =
      tensor.to(torch::kCPU).to(TorchTypeMap<T>::type).contiguous();

  std::vector<T> edgeIndex(
      transformedTensor.template data_ptr<T>(),
      transformedTensor.template data_ptr<T>() + transformedTensor.numel());

  return edgeIndex;
}

}  // namespace Acts::detail
