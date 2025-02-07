// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <vector>

namespace ActsExamples {

/// Lightweight graph representation for GNN debugging
///
struct Graph {
  /// The edges-vector contains flattened edge-pairs. Usually, the indices
  /// refer to a spacepoint collection.
  ///
  /// Structure: [ source0, dest0, source1, dest1, ..., sourceN, destN ]
  std::vector<std::int64_t> edges;

  /// The weight-vector should have half the size of the edges-vector (or
  /// be empty if missing).
  ///
  /// Structure: [ weight0, weight1, ..., weightN ]
  std::vector<float> weights;
};

}  // namespace ActsExamples
