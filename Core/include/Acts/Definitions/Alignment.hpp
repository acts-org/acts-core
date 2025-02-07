// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"

namespace Acts {

/// Components of alignment parameters vector.
///
/// To be used to access components by named indices instead of just numbers.
/// This must be a regular `enum` and not a scoped `enum class` to allow
/// implicit conversion to an integer. The enum value are thus visible directly
/// in `namespace Acts` and are prefixed to avoid naming collisions.
enum AlignmentIndices : unsigned int {
  // Center of geometry object in global 3D cartesian coordinates
  eAlignmentCenter0 = 0u,
  eAlignmentCenter1 = eAlignmentCenter0 + 1u,
  eAlignmentCenter2 = eAlignmentCenter0 + 2u,
  // Rotation angle around local x/y/z axis of geometry object
  eAlignmentRotation0 = 3u,
  eAlignmentRotation1 = eAlignmentRotation0 + 1u,
  eAlignmentRotation2 = eAlignmentRotation0 + 2u,
  // Last uninitialized value contains the total number of components
  eAlignmentSize,
};

// Matrix and vector types related to alignment parameters.
using AlignmentVector = ActsVector<eAlignmentSize>;
using AlignmentRowVector = ActsMatrix<1, eAlignmentSize>;
using AlignmentMatrix = ActsMatrix<eAlignmentSize, eAlignmentSize>;
using AlignmentToPositionMatrix = ActsMatrix<3, eAlignmentSize>;
using AlignmentToBoundMatrix = ActsMatrix<eBoundSize, eAlignmentSize>;
using AlignmentToPathMatrix = ActsMatrix<1, eAlignmentSize>;

}  // namespace Acts
