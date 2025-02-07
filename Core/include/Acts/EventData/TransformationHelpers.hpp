// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <numbers>

namespace Acts {

class Surface;

/// Reflect bound track parameters.
///
/// @param boundParams Bound track parameters vector
/// @return Reflected bound track parameters vector
inline BoundVector reflectBoundParameters(const BoundVector& boundParams) {
  BoundVector reflected = boundParams;
  auto [phi, theta] =
      detail::normalizePhiTheta(boundParams[eBoundPhi] - std::numbers::pi,
                                std::numbers::pi - boundParams[eBoundTheta]);
  reflected[eBoundPhi] = phi;
  reflected[eBoundTheta] = theta;
  reflected[eBoundQOverP] = -boundParams[eBoundQOverP];
  return reflected;
}

/// Reflect free track parameters.
///
/// @param freeParams Free track parameters vector
/// @return Reflected free track parameters vector
inline FreeVector reflectFreeParameters(const FreeVector& freeParams) {
  FreeVector reflected = freeParams;
  reflected[eFreeDir0] = -freeParams[eFreeDir0];
  reflected[eFreeDir1] = -freeParams[eFreeDir1];
  reflected[eFreeDir2] = -freeParams[eFreeDir2];
  reflected[eFreeQOverP] = -freeParams[eFreeQOverP];
  return reflected;
}

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
    const GeometryContext& geoCtx, double tolerance = s_onSurfaceTolerance);

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
    const Vector3& position, double time, const Vector3& direction,
    double qOverP, const Surface& surface, const GeometryContext& geoCtx,
    double tolerance = s_onSurfaceTolerance);

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
BoundVector transformFreeToCurvilinearParameters(double time,
                                                 const Vector3& direction,
                                                 double qOverP);

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
BoundVector transformFreeToCurvilinearParameters(double time, double phi,
                                                 double theta, double qOverP);

}  // namespace Acts
