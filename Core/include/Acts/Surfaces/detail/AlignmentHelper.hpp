// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <tuple>
#include <utility>
#include <vector>

namespace Acts::detail {

// The container for derivative of local frame axis w.r.t. its
// rotation parameters. The first element is for x axis, second for y axis and
// last for z axis
using RotationToAxes =
    std::tuple<RotationMatrix3, RotationMatrix3, RotationMatrix3>;

/// @brief Evaluate the derivative of local frame axes vector w.r.t.
/// its rotation around local x/y/z axis
/// @Todo: add parameter for rotation axis order
///
/// @param compositeRotation The rotation that help places the composite object being rotated
/// @param relRotation The relative rotation of the surface with respect to the composite object being rotated
///
/// @return Derivative of local frame x/y/z axis vector w.r.t. its
/// rotation angles (extrinsic Euler angles) around local x/y/z axis
RotationToAxes rotationToLocalAxesDerivative(
    const RotationMatrix3& compositeRotation,
    const RotationMatrix3& relRotation = RotationMatrix3::Identity());

}  // namespace Acts::detail
