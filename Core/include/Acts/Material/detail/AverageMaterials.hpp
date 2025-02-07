// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Material/MaterialSlab.hpp"

namespace Acts::detail {

/// Compute the average properties for a combined slab of two materials.
///
/// @params slab1 Properties of the first material slab
/// @params slab2 Properties of the second material slab
/// @returns Material slab with the combined thickness and average parameters
///
/// The averaged material slab has the combined thickness of the two input slabs
/// and assumes the two input materials are homogeneously and continuously mixed
/// throughout the slab.
MaterialSlab combineSlabs(const MaterialSlab& slab1, const MaterialSlab& slab2);

}  // namespace Acts::detail
