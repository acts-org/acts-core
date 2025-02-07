// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

// CUDA plugin include(s).
#include "Acts/Plugins/Cuda/Seeding2/Details/Types.hpp"

namespace Acts {
namespace Cuda {

/// Structure holding pointers to the user defined filter functions
struct TripletFilterConfig {
  /// Type for the seed weighting functions
  using seedWeightFunc_t = float (*)(const Details::SpacePoint &,
                                     const Details::SpacePoint &,
                                     const Details::SpacePoint &);

  /// Pointer to a function assigning weights to seed candidates
  ///
  /// The function receives the bottom, middle and top spacepoints (in this
  /// order), and needs to return a float weight for the combination.
  ///
  /// Note that you can not set this pointer directly. You must use
  /// @c cudaMemcpyFromSymbol to set it from a global @c __device__ function
  /// pointer.
  ///
  seedWeightFunc_t seedWeight = nullptr;

  /// Type for the seed filtering functions
  using singleSeedCutFunc_t = bool (*)(float, const Details::SpacePoint &,
                                       const Details::SpacePoint &,
                                       const Details::SpacePoint &);

  /// Pointer to a function filtering seed candidates
  ///
  /// The function receives a previously assigned "seed weight", and references
  /// to the bottom, middle and top spacepoints (in this order). It needs to
  /// return an accept/reject decision for the combination.
  ///
  /// Note that you can not set this pointer directly. You must use
  /// @c cudaMemcpyFromSymbol to set it from a global @c __device__ function
  /// pointer.
  ///
  singleSeedCutFunc_t singleSeedCut = nullptr;

};  // struct TripletFinderConfig

}  // namespace Cuda
}  // namespace Acts
