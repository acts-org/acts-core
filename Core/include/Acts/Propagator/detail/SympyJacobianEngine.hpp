// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// Workaround for building on clang+libstdc++
#include "Acts/Utilities/detail/ReferenceWrapperAnyCompat.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryContext.hpp"

namespace Acts {
class Surface;
}  // namespace Acts

namespace Acts::detail::sympy {

/// @brief These functions perform the calculation of the Jacobians for the
/// the covariance transport. This is a purely algebraic problem the
/// calculations are identical for @c StraightLineStepper and @c EigenStepper.
/// As a consequence the methods can be located in a separate file.

/// @brief This function calculates the full transport jacobian from a bound
///        curvilinear representation to a new bound representation
///
/// @note Modifications of the jacobian related to the projection onto a surface is
/// considered. Since a variation of the start parameters within a given
/// uncertainty would lead to a variation of the end parameters, these need to
/// be propagated onto the target surface. This an approximated approach to
/// treat the (assumed) small change.
///
/// @param [in] geoContext The geometry Context
/// @param [in] surface Target surface
/// @param [in] freeParameters Free, nominal parametrisation
/// @param [in] boundToFreeJacobian Jacobian from bound to free at start
/// @param [in] freeTransportJacobian Transport jacobian free to free
/// @param [in] freeToPathDerivatives Path length derivatives for free parameters
/// @param [out] fullTransportJacobian A 6x6 transport jacobian from bound to bound
///
/// @note jac(locA->locB) = jac(gloB->locB)*(1+
///       pathCorrectionFactor(gloB))*jacTransport(gloA->gloB) *jac(locA->gloA)
void boundToBoundTransportJacobian(const GeometryContext& geoContext,
                                   const Surface& surface,
                                   const FreeVector& freeParameters,
                                   const BoundToFreeMatrix& boundToFreeJacobian,
                                   const FreeMatrix& freeTransportJacobian,
                                   const FreeVector& freeToPathDerivatives,
                                   BoundMatrix& fullTransportJacobian);

/// @brief This function calculates the full jacobian from a given
/// bound/curvilinear parameterisation from a surface to new curvilinear
/// parameterisation.
///
/// @note Modifications of the jacobian related to the
/// projection onto a curvilinear surface is considered. Since a variation of
/// the start parameters within a given uncertainty would lead to a variation of
/// the end parameters, these need to be propagated onto the target surface.
/// This is an approximated approach to treat the (assumed) small change.
///
/// @param [in] direction Normalised direction vector
/// @param [in] boundToFreeJacobian Jacobian from bound to free at start
/// @param [in] freeTransportJacobian Transport jacobian free to free
/// @param [in] freeToPathDerivatives Path length derivatives for free parameters
/// @param [out] fullTransportJacobian A 6x6 transport jacobian from curilinear to bound
///
/// @note The parameter @p surface is only required if projected to bound
/// parameters. In the case of curvilinear parameters the geometry and the
/// position is known and the calculation can be simplified
void boundToCurvilinearTransportJacobian(
    const Vector3& direction, const BoundToFreeMatrix& boundToFreeJacobian,
    const FreeMatrix& freeTransportJacobian,
    const FreeVector& freeToPathDerivatives,
    BoundMatrix& fullTransportJacobian);

}  // namespace Acts::detail::sympy
