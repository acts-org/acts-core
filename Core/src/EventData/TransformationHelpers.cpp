// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/EventData/TransformationHelpers.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/UnitVectors.hpp"

#include <algorithm>

Acts::FreeVector Acts::transformBoundToFreeParameters(
    const Acts::Surface& surface, const GeometryContext& geoCtx,
    const Acts::BoundVector& boundParams) {
  // convert angles to global unit direction vector
  Vector3 direction = makeDirectionFromPhiTheta(boundParams[eBoundPhi],
                                                boundParams[eBoundTheta]);

  // convert local position to global position vector
  Vector2 local(boundParams[eBoundLoc0], boundParams[eBoundLoc1]);
  Vector3 position = surface.localToGlobal(geoCtx, local, direction);

  // construct full free-vector. time and q/p stay as-is.
  FreeVector freeParams = FreeVector::Zero();
  freeParams[eFreePos0] = position[ePos0];
  freeParams[eFreePos1] = position[ePos1];
  freeParams[eFreePos2] = position[ePos2];
  freeParams[eFreeTime] = boundParams[eBoundTime];
  freeParams[eFreeDir0] = direction[eMom0];
  freeParams[eFreeDir1] = direction[eMom1];
  freeParams[eFreeDir2] = direction[eMom2];
  freeParams[eFreeQOverP] = boundParams[eBoundQOverP];
  return freeParams;
}

Acts::Result<Acts::BoundVector> Acts::transformFreeToBoundParameters(
    const FreeVector& freeParams, const Surface& surface,
    const GeometryContext& geoCtx, ActsScalar tolerance) {
  // initialize the bound vector
  BoundVector bp = BoundVector::Zero();
  // convert global to local position on the surface
  auto position = freeParams.segment<3>(eFreePos0);
  auto direction = freeParams.segment<3>(eFreeDir0);
  auto result = surface.globalToLocal(geoCtx, position, direction, tolerance);
  if (!result.ok()) {
    return Result<Acts::BoundVector>::failure(result.error());
  }

  auto localPosition = result.value();
  bp[eBoundLoc0] = localPosition[ePos0];
  bp[eBoundLoc1] = localPosition[ePos1];

  bp[eBoundTime] = freeParams[eFreeTime];
  bp[eBoundPhi] = VectorHelpers::phi(direction);
  bp[eBoundTheta] = VectorHelpers::theta(direction);
  bp[eBoundQOverP] = freeParams[eFreeQOverP];
  return Result<Acts::BoundVector>::success(bp);
}

Acts::Result<Acts::BoundVector> Acts::transformFreeToBoundParameters(
    const Acts::Vector3& position, ActsScalar time,
    const Acts::Vector3& direction, ActsScalar qOverP,
    const Acts::Surface& surface, const Acts::GeometryContext& geoCtx,
    ActsScalar tolerance) {
  // initialize the bound vector
  BoundVector bp = BoundVector::Zero();
  // convert global to local position on the surface
  auto result = surface.globalToLocal(geoCtx, position, direction, tolerance);
  if (!result.ok()) {
    return Result<Acts::BoundVector>::failure(result.error());
  }

  auto localPosition = result.value();
  bp[eBoundLoc0] = localPosition[ePos0];
  bp[eBoundLoc1] = localPosition[ePos1];

  bp[eBoundTime] = time;
  bp[eBoundPhi] = VectorHelpers::phi(direction);
  bp[eBoundTheta] = VectorHelpers::theta(direction);
  bp[eBoundQOverP] = qOverP;
  return Result<Acts::BoundVector>::success(bp);
}

Acts::BoundVector Acts::transformFreeToCurvilinearParameters(
    ActsScalar time, ActsScalar phi, ActsScalar theta, ActsScalar qOverP) {
  BoundVector bp = BoundVector::Zero();
  // local coordinates are zero by construction
  bp[eBoundTime] = time;
  bp[eBoundPhi] = phi;
  bp[eBoundTheta] = theta;
  bp[eBoundQOverP] = qOverP;
  return bp;
}

Acts::BoundVector Acts::transformFreeToCurvilinearParameters(
    ActsScalar time, const Vector3& direction, ActsScalar qOverP) {
  BoundVector bp = BoundVector::Zero();
  // local coordinates are zero by construction
  bp[eBoundTime] = time;
  bp[eBoundPhi] = VectorHelpers::phi(direction);
  bp[eBoundTheta] = VectorHelpers::theta(direction);
  bp[eBoundQOverP] = qOverP;
  return bp;
}
