// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <ostream>

#include <torch/torch.h>

namespace Acts::detail {

struct TensorDetails {
  const torch::Tensor &tensor;
  TensorDetails(const torch::Tensor &t) : tensor(t) {}
};

inline std::ostream &operator<<(std::ostream &os, const TensorDetails &t) {
  os << t.tensor.dtype() << ", " << t.tensor.sizes();
  if (at::isnan(t.tensor).any().item<bool>()) {
    os << ", contains NaNs";
  } else {
    os << ", no NaNs";
  }
  return os;
}

template <typename It>
struct RangePrinter {
  It begin;
  It end;

  RangePrinter(It a, It b) : begin(a), end(b) {}
};

template <class It>
RangePrinter(It b, It e) -> RangePrinter<It>;

template <typename It>
inline std::ostream &operator<<(std::ostream &os, const RangePrinter<It> &r) {
  for (auto it = r.begin; it != r.end; ++it) {
    os << *it << " ";
  }
  return os;
}

}  // namespace Acts::detail
