// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <iosfwd>

namespace Acts {

///  This is a steering enum to tell which material update stage:
/// - PreUpdate  : update on approach of a surface
/// - FullUpdate : update when passing a surface
/// - PostUpdate : update when leaving a surface
enum class MaterialUpdateStage : int {
  PreUpdate = -1,
  FullUpdate = 0,
  PostUpdate = 1
};

std::ostream& operator<<(std::ostream& os, MaterialUpdateStage matUpdate);

/// @enum NoiseUpdateMode to tell how to deal with noise term in covariance
/// transport
/// - removeNoise: subtract noise term
/// - addNoise: add noise term
enum NoiseUpdateMode : int { removeNoise = -1, addNoise = 1 };

/// Components of coordinate vectors.
///
/// To be used to access coordinate components by named indices instead of magic
/// numbers. This must be a regular `enum` and not a scoped `enum class` to
/// allow implicit conversion to an integer. The enum value are thus visible
/// directly in `namespace Acts`.
///
/// This index enum is not user-configurable (in contrast e.g. to the track
/// parameter index enums) since it must be compatible with varying
/// dimensionality (2d-4d) and other access methods (`.{x,y,z}()` accessors).
enum CoordinateIndices : unsigned int {
  // generic position-like access
  ePos0 = 0,
  ePos1 = 1,
  ePos2 = 2,
  eTime = 3,
  // generic momentum-like access
  eMom0 = ePos0,
  eMom1 = ePos1,
  eMom2 = ePos2,
  eEnergy = eTime,
  // Cartesian spatial coordinates
  eX = ePos0,
  eY = ePos1,
  eZ = ePos2,
};

}  // namespace Acts
