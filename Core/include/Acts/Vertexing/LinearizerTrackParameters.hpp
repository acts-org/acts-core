// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

namespace Acts {

/// Enum to access the components of a track parameter vector.
///
/// Here, we parametrize the track via a 4D point on the track, the momentum
/// angles of the particle at that point, and q/p or 1/p.
///
/// @note It would make sense to rename these parameters if they are used outside of track linearization.
/// @note This must be a regular `enum` and not a scoped `enum class` to allow
/// implicit conversion to an integer. The enum value are thus visible directly
/// in `namespace Acts` and are prefixed to avoid naming collisions.
enum LinIndices : unsigned int {
  // Global spatial position of a point on the track, must be stored as one
  // continuous block.
  eLinPos0 = 0u,
  eLinPos1 = eLinPos0 + 1u,
  eLinPos2 = eLinPos0 + 2u,
  // Global time when particle is at the point
  eLinTime = 3u,
  // Angles of the particle momentum in the global frame at the point
  eLinPhi = 4u,
  eLinTheta = eLinPhi + 1u,
  // Global inverse-momentum-like parameter, i.e. q/p or 1/p, at the point
  // The naming is inconsistent for the case of neutral track parameters where
  // the value is interpreted as 1/p not as q/p. This is intentional to avoid
  // having multiple aliases for the same element and for lack of an acceptable
  // common name.
  eLinQOverP = 6u,
  // Total number of components
  eLinSize = 7u,
  // Number of space-time components (3+1)
  eLinPosSize = 4u,
  // Number of momentum components
  eLinMomSize = 3u,
};

}  // namespace Acts
