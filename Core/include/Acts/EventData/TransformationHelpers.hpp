// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/Result.hpp"

namespace Acts {

class Surface;

/// Transform bound track parameters into equivalent free track parameters.
///
/// @param surface Surface onto which the input parameters are bound
/// @param geoCtx Geometry context for the local-to-global transformation
/// @param boundParams Bound track parameters vector
/// @return Equivalent free trackparameters vector
FreeVector transformBoundToFreeParameters(const Surface& surface,
                                          const GeometryContext& geoCtx,
                                          const BoundVector& boundParams);

/// Convert free track parameters to bound track parameters.
///
/// @param freeParams Free track parameters vector
/// @param surface Surface onto which the parameters are bound
/// @param geoCtx Geometry context for the global-to-local transformation
/// @param tolerance Tolerance used for globalToLocal
///
/// @return Bound track parameters vector on the given surface
Result<BoundVector> transformFreeToBoundParameters(
    const FreeVector& freeParams, const Surface& surface,
    const GeometryContext& geoCtx, ActsScalar tolerance = s_onSurfaceTolerance);

/// Convert position and direction to bound track parameters.
///
/// @param position Global track three-position
/// @param time Global track time
/// @param direction Global direction three-vector; normalization is ignored.
/// @param qOverP Charge-over-momentum-like parameter
/// @param surface Surface onto which the parameters are bound
/// @param geoCtx Geometry context for the global-to-local transformation
/// @param tolerance Tolerance used for globalToLocal
///
/// @return Equivalent bound parameters vector on the given surface
Result<BoundVector> transformFreeToBoundParameters(
    const Vector3& position, ActsScalar time, const Vector3& direction,
    ActsScalar qOverP, const Surface& surface, const GeometryContext& geoCtx,
    ActsScalar tolerance = s_onSurfaceTolerance);

/// Convert direction to curvilinear track parameters.
///
/// @param time Global track time
/// @param direction Global direction three-vector; normalization is ignored.
/// @param qOverP Charge-over-momentum-like parameter
/// @return Equivalent bound parameters vector on the curvilinear surface
///
/// @note The parameters are assumed to be defined at the origin of the
///       curvilinear frame derived from the direction vector. The local
///       coordinates are zero by construction.
BoundVector transformFreeToCurvilinearParameters(ActsScalar time,
                                                 const Vector3& direction,
                                                 ActsScalar qOverP);

/// Convert direction angles to curvilinear track parameters.
///
/// @param time Global track time
/// @param phi Global transverse direction angle
/// @param theta Global longitudinal direction angle
/// @param qOverP Charge-over-momentum-like parameter
/// @return Equivalent bound parameters vector on the curvilinear surface
///
/// @note The parameters are assumed to be defined at the origin of the
///       curvilinear frame derived from the direction angles. The local
///       coordinates are zero by construction.
BoundVector transformFreeToCurvilinearParameters(ActsScalar time,
                                                 ActsScalar phi,
                                                 ActsScalar theta,
                                                 ActsScalar qOverP);

}  // namespace Acts
