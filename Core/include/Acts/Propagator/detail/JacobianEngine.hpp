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

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"

namespace Acts::detail {

/// @brief These functions perform the calculation of the Jacobians for the
/// the covariance transport. This is a purely algebraic problem the
/// calculations are identical for @c StraightLineStepper and @c EigenStepper.
/// As a consequence the methods can be located in a separate file.

/// @brief Evaluate the projection Jacobian from free to curvilinear parameters
///        without transport jacobian.
///
/// @param [in] direction Normalised direction vector
///
/// @return Projection Jacobian
FreeToBoundMatrix freeToCurvilinearJacobian(const Vector3& direction);

/// @brief Evaluate the projection Jacobian from curvilinear to free parameters
///        without transport jacobian.
///
/// @param [in] direction Normalised direction vector
///
/// @return Projection Jacobian
BoundToFreeMatrix curvilinearToFreeJacobian(const Vector3& direction);

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
/// @param [in] freeParameters Free, nominal parametrisation
/// @param [in] boundToFreeJacobian Jacobian from bound to free at start
/// @param [in] freeTransportJacobian Transport jacobian free to free
/// @param [in] freeToPathDerivatives Path length derivatives for free parameters
/// @param [in] surface Target surface
///
/// @note jac(locA->locB) = jac(gloB->locB)*(1+
/// pathCorrectionFactor(gloB))*jacTransport(gloA->gloB) *jac(locA->gloA)
///
/// @return a 6x6 transport jacobian from bound to bound
BoundMatrix boundToBoundTransportJacobian(
    const GeometryContext& geoContext, const FreeVector& freeParameters,
    const BoundToFreeMatrix& boundToFreeJacobian,
    const FreeMatrix& freeTransportJacobian,
    const FreeVector& freeToPathDerivatives, const Surface& surface);

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

/// @brief This function calculates the full jacobian from a given
/// bound/curvilinear parameterisation from a new free parameterisation.
///
/// @param [in] boundToFreeJacobian Jacobian from bound to free at start
/// @param [in] freeTransportJacobian Transport jacobian free to free
///
/// @return a 8x6 transport jacobian from bound to free
BoundToFreeMatrix boundToFreeTransportJacobian(
    const BoundToFreeMatrix& boundToFreeJacobian,
    const FreeMatrix& freeTransportJacobian);

/// @brief This function calculates the full jacobian from a given
/// free parametrisation to a new curvilinear bound parametrisation.
///
/// @note Modifications of the jacobian related to the
/// projection onto the target surface is considered. Since a variation of
/// the start parameters within a given uncertainty would lead to a variation of
/// the end parameters, these need to be propagated onto the target surface.
/// This is an approximated approach to treat the (assumed) small change.
///
/// @param [in] geoContext The geometry Context
/// @param [in] surface The target surface
/// @param [in] freeParameters Free, nominal parametrisation
/// @param [in] freeTransportJacobian Transport jacobian free to free
/// @param [in] freeToPathDerivatives Path length derivatives for free parameters
/// @param [out] fullTransportJacobian The 6x8 transport jacobian from bound to free
///
void freeToBoundTransportJacobian(const GeometryContext& geoContext,
                                  const Surface& surface,
                                  const FreeVector& freeParameters,
                                  const FreeMatrix& freeTransportJacobian,
                                  const FreeVector& freeToPathDerivatives,
                                  FreeToBoundMatrix& fullTransportJacobian);

/// @brief This function calculates the full transport jacobian from a free
/// parameterisation to a bound one. Since a variation of the start
/// parameters within a given uncertainty would lead to a variation of the end
/// parameters, these need to be propagated onto the target surface. This an
/// approximated approach to treat the (assumed) small change.
///
/// @param [in] direction Normalised direction vector
/// @param [in] freeTransportJacobian Transport jacobian free to free
/// @param [in] freeToPathDerivatives Path length derivatives for free
///
/// @return a 6x8 transport jacobian from curvilinear to free
FreeToBoundMatrix freeToCurvilinearTransportJacobian(
    const Vector3& direction, const FreeMatrix& freeTransportJacobian,
    const FreeVector& freeToPathDerivatives);

/// @brief This function reinitialises the state members required for the
///        covariance transport for usual surfaces
///
/// Reinitialize jacobian components:
/// ->The transportJacobian is reinitialized to Identity
/// ->The derivatives is reinitialized to Zero
/// ->The boundToFreeJacobian is initialized to that at the current surface
///
/// @param [in] geoContext The geometry context
/// @param [in] surface The reference surface of the local parametrisation
/// @param [in, out] freeTransportJacobian The transport jacobian from start
///        free to final free parameters
/// @param [in, out] freeToPathDerivatives Path length derivatives of the free,
///        nominal parameters
/// @param [in, out] boundToFreeJacobian Projection jacobian of the last bound
///        parametrisation to free parameters
/// @param [in] freeParameters Free, nominal parametrisation
Result<void> reinitializeJacobians(const GeometryContext& geoContext,
                                   const Surface& surface,
                                   FreeMatrix& freeTransportJacobian,
                                   FreeVector& freeToPathDerivatives,
                                   BoundToFreeMatrix& boundToFreeJacobian,
                                   const FreeVector& freeParameters);

/// @brief This function reinitialises the state members required for the
///        covariance transport for curvilinear surfaces
///
/// Reinitialize jacobian components:
/// ->The free transportJacobian is reinitialized to Identity
/// ->The path derivatives is reinitialized to Zero
/// ->The boundToFreeJacobian is reinitialized to that at the current
/// curvilinear surface
///
/// @param [in, out] freeTransportJacobian The transport jacobian from start
///        free to final free parameters
/// @param [in, out] derivatives Path length derivatives of the free, nominal
///        parameters
/// @param [in, out] boundToFreeJacobian Projection jacobian of the last bound
///        parametrisation to free parameters
/// @param [in] direction Normalised direction vector
void reinitializeJacobians(FreeMatrix& freeTransportJacobian,
                           FreeVector& freeToPathDerivatives,
                           BoundToFreeMatrix& boundToFreeJacobian,
                           const Vector3& direction);

}  // namespace Acts::detail
