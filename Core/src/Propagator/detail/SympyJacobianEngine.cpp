// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Propagator/detail/SympyJacobianEngine.hpp"

#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include "codegen/sympy_jac_math.hpp"

namespace Acts::detail {

void sympy::boundToBoundTransportJacobian(
    const GeometryContext& geoContext, const Surface& surface,
    const FreeVector& freeParameters,
    const BoundToFreeMatrix& boundToFreeJacobian,
    const FreeMatrix& freeTransportJacobian,
    const FreeVector& freeToPathDerivatives,
    BoundMatrix& fullTransportJacobian) {
  const Vector3 position = freeParameters.segment<3>(eFreePos0);
  const Vector3 direction = freeParameters.segment<3>(eFreeDir0);
  // Calculate the derivative of path length at the final surface or the
  // point-of-closest approach w.r.t. free parameters
  const FreeToPathMatrix freeToPath =
      surface.freeToPathDerivative(geoContext, position, direction);
  // Calculate the jacobian from free to bound at the final surface
  FreeToBoundMatrix freeToBoundJacobian =
      surface.freeToBoundJacobian(geoContext, position, direction);
  // https://acts.readthedocs.io/en/latest/white_papers/correction-for-transport-jacobian.html
  // Calculate the full jacobian from the local/bound parameters at the start
  // surface to local/bound parameters at the final surface
  // @note jac(locA->locB) = jac(gloB->locB)*(1+
  // pathCorrectionFactor(gloB))*jacTransport(gloA->gloB) *jac(locA->gloA)

  boundToBoundTransportJacobianImpl(
      freeToBoundJacobian.data(), freeTransportJacobian.data(),
      boundToFreeJacobian.data(), freeToPathDerivatives.data(),
      freeToPath.data(), fullTransportJacobian.data());
}

void sympy::boundToCurvilinearTransportJacobian(
    const Vector3& direction, const BoundToFreeMatrix& boundToFreeJacobian,
    const FreeMatrix& freeTransportJacobian,
    const FreeVector& freeToPathDerivatives,
    BoundMatrix& fullTransportJacobian) {
  // Calculate the jacobian from global to local at the curvilinear surface
  FreeToBoundMatrix freeToBoundJacobian =
      CurvilinearSurface(direction).freeToBoundJacobian();

  // Calculate the full jocobian from the local parameters at the start surface
  // to curvilinear parameters
  // @note jac(locA->locB) = jac(gloB->locB)*(1+
  // pathCorrectionFactor(gloB))*jacTransport(gloA->gloB) *jac(locA->gloA)

  boundToCurvilinearTransportJacobianImpl(
      freeToBoundJacobian.data(), freeTransportJacobian.data(),
      boundToFreeJacobian.data(), freeToPathDerivatives.data(),
      direction.data(), fullTransportJacobian.data());
}

}  // namespace Acts::detail
