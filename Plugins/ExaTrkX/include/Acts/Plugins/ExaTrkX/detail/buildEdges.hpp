// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>

namespace at {
class Tensor;
}

namespace Acts {
namespace detail {

/// Post process edges
at::Tensor postprocessEdgeTensor(at::Tensor edges, bool removeSelfLoops = true,
                                 bool removeDuplicates = true,
                                 bool flipDirections = false);

/// Edge building using FRNN and CUDA.
/// Raises an exception if not built with CUDA
at::Tensor buildEdgesFRNN(at::Tensor& embedFeatures, float rVal, int kVal,
                          bool flipDirections = false);

/// Edge building using the Acts KD-Tree implementation
/// Note that this implementation has no maximum number of neighbours
/// in the NN search. kVal is only a hint for reserving memory
at::Tensor buildEdgesKDTree(at::Tensor& embedFeatures, float rVal, int kVal,
                            bool flipDirections = false);

/// Dispatches either to FRNN or KD-Tree based edge building
///
/// @param embedFeatures Tensor of shape (n_nodes, embedding_dim)
/// @param rVal radius for NN search
/// @param kVal max number of neighbours in NN search
/// @param flipDirections if we want to randomly flip directions of the
/// edges after the edge building
at::Tensor buildEdges(at::Tensor& embedFeatures, float rVal, int kVal,
                      bool flipDirections = false);

}  // namespace detail
}  // namespace Acts
