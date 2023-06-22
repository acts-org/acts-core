// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>

namespace at {
class Tensor;
}

namespace Acts {
namespace detail {
    
/// Edge building using FRNN and CUDA.
/// Raises an exception if not built with CUDA
at::Tensor buildEdgesFRNN(at::Tensor& embedFeatures, float rVal, int kVal,
                          bool flipDirections = false);

/// Edge building using the Acts KD-Tree implementation
at::Tensor buildEdgesKDTree(at::Tensor &embedFeatures, float rVal, int kVal, bool flipDirections = false);

}

/// Dispatches either to FRNN or KD-Tree based edge building
///
/// @param embedFeatures Tensor of shape (n_nodes, embedding_dim)
/// @param rVal radius for NN search
/// @param kVal max number of neighbours in NN search
/// @param flipDirections if we want to randomly flip directions of the
/// edges after the edge building
at::Tensor buildEdges(at::Tensor& embedFeatures, float rVal, int kVal,
                      bool flipDirections = false);



}  // namespace Acts
