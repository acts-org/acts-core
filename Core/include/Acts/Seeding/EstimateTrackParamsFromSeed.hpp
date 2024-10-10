// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/MathHelpers.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <iterator>
#include <optional>

namespace Acts {
/// @todo:
/// 1) Implement the simple Line and Circle fit based on Taubin Circle fit
/// 2) Implement the simple Line and Parabola fit (from HPS reconstruction by
/// Robert Johnson)

/// Estimate the track parameters on the xy plane from at least three space
/// points. It assumes the trajectory projection on the xy plane is a circle,
/// i.e. the magnetic field is along global z-axis.
///
/// The method is based on V. Karimaki NIM A305 (1991) 187-191:
/// https://doi.org/10.1016/0168-9002(91)90533-V
/// - no weights are used in Karimaki's fit; d0 is the distance of the point of
/// closest approach to the origin, 1/R is the curvature, phi is the angle of
/// the direction propagation (counter clockwise as positive) at the point of
/// cloest approach.
///
/// @tparam spacepoint_iterator_t The type of space point iterator
///
/// @param spBegin is the begin iterator for the space points
/// @param spEnd is the end iterator for the space points
/// @param logger A logger instance
///
/// @return optional bound track parameters with the estimated d0, phi and 1/R
/// stored with the indices, eBoundLoc0, eBoundPhi and eBoundQOverP,
/// respectively. The bound parameters with other indices are set to zero.
template <typename spacepoint_iterator_t>
std::optional<BoundVector> estimateTrackParamsFromSeed(
    spacepoint_iterator_t spBegin, spacepoint_iterator_t spEnd,
    const Logger& logger = getDummyLogger()) {
  // Check the number of provided space points
  std::size_t numSP = std::distance(spBegin, spEnd);
  if (numSP < 3) {
    ACTS_ERROR("At least three space points are required.");
    return std::nullopt;
  }

  ActsScalar x2m = 0., xm = 0.;
  ActsScalar xym = 0.;
  ActsScalar y2m = 0., ym = 0.;
  ActsScalar r2m = 0., r4m = 0.;
  ActsScalar xr2m = 0., yr2m = 0.;

  for (spacepoint_iterator_t it = spBegin; it != spEnd; it++) {
    if (*it == nullptr) {
      ACTS_ERROR("Empty space point found. This should not happen.");
      return std::nullopt;
    }

    const auto& sp = *it;

    ActsScalar x = sp->x();
    ActsScalar y = sp->y();
    ActsScalar r2 = x * x + y * y;
    x2m += x * x;
    xm += x;
    xym += x * y;
    y2m += y * y;
    ym += y;
    r2m += r2;
    r4m += r2 * r2;
    xr2m += x * r2;
    yr2m += y * r2;
    numSP++;
  }
  x2m = x2m / numSP;
  xm = xm / numSP;
  xym = xym / numSP;
  y2m = y2m / numSP;
  ym = ym / numSP;
  r2m = r2m / numSP;
  r4m = r4m / numSP;
  xr2m = xr2m / numSP;
  yr2m = yr2m / numSP;

  ActsScalar Cxx = x2m - xm * xm;
  ActsScalar Cxy = xym - xm * ym;
  ActsScalar Cyy = y2m - ym * ym;
  ActsScalar Cxr2 = xr2m - xm * r2m;
  ActsScalar Cyr2 = yr2m - ym * r2m;
  ActsScalar Cr2r2 = r4m - r2m * r2m;

  ActsScalar q1 = Cr2r2 * Cxy - Cxr2 * Cyr2;
  ActsScalar q2 = Cr2r2 * (Cxx - Cyy) - Cxr2 * Cxr2 + Cyr2 * Cyr2;

  ActsScalar phi = 0.5 * std::atan(2 * q1 / q2);
  ActsScalar k = (std::sin(phi) * Cxr2 - std::cos(phi) * Cyr2) * (1. / Cr2r2);
  ActsScalar delta = -k * r2m + std::sin(phi) * xm - std::cos(phi) * ym;

  ActsScalar rho = (2 * k) / (std::sqrt(1 - 4 * delta * k));
  ActsScalar d0 = (2 * delta) / (1 + std::sqrt(1 - 4 * delta * k));

  // Initialize the bound parameters vector
  BoundVector params = BoundVector::Zero();
  params[eBoundLoc0] = d0;
  params[eBoundPhi] = phi;
  params[eBoundQOverP] = rho;

  return params;
}

/// Estimate the full track parameters from three space points
///
/// This method is based on the conformal map transformation. It estimates the
/// full bound track parameters, i.e. (loc0, loc1, phi, theta, q/p, t) at the
/// bottom space point. The bottom space is assumed to be the first element
/// in the range defined by the iterators. It must lie on the surface
/// provided for the representation of the bound track parameters. The magnetic
/// field (which might be along any direction) is also necessary for the
/// momentum estimation.
///
/// It resembles the method used in ATLAS for the track parameters
/// estimated from seed, i.e. the function InDet::SiTrackMaker_xk::getAtaPlane
/// here:
/// https://acode-browser.usatlas.bnl.gov/lxr/source/athena/InnerDetector/InDetRecTools/SiTrackMakerTool_xk/src/SiTrackMaker_xk.cxx
///
/// @tparam spacepoint_iterator_t  The type of space point iterator
///
/// @param gctx is the geometry context
/// @param spBegin is the begin iterator for the space points
/// @param spEnd is the end iterator for the space points
/// @param surface is the surface of the bottom space point. The estimated bound
/// track parameters will be represented also at this surface
/// @param bField is the magnetic field vector
/// @param bFieldMin is the minimum magnetic field required to trigger the
/// estimation of q/pt
/// @param logger A logger instance
///
/// @return optional bound parameters
template <typename spacepoint_iterator_t>
std::optional<BoundVector> estimateTrackParamsFromSeed(
    const GeometryContext& gctx, spacepoint_iterator_t spBegin,
    spacepoint_iterator_t spEnd, const Surface& surface, const Vector3& bField,
    ActsScalar bFieldMin, const Acts::Logger& logger = getDummyLogger()) {
  // Check the number of provided space points
  std::size_t numSP = std::distance(spBegin, spEnd);
  if (numSP != 3) {
    ACTS_ERROR("There should be exactly three space points provided.");
    return std::nullopt;
  }

  // Convert bField to Tesla
  ActsScalar bFieldInTesla = bField.norm() / UnitConstants::T;
  ActsScalar bFieldMinInTesla = bFieldMin / UnitConstants::T;
  // Check if magnetic field is too small
  if (bFieldInTesla < bFieldMinInTesla) {
    // @todo shall we use straight-line estimation and use default q/pt in such
    // case?
    ACTS_WARNING("The magnetic field at the bottom space point: B = "
                 << bFieldInTesla << " T is smaller than |B|_min = "
                 << bFieldMinInTesla << " T. Estimation is not performed.");
    return std::nullopt;
  }

  // The global positions of the bottom, middle and space points
  std::array<Vector3, 3> spGlobalPositions = {Vector3::Zero(), Vector3::Zero(),
                                              Vector3::Zero()};
  std::array<std::optional<float>, 3> spGlobalTimes = {
      std::nullopt, std::nullopt, std::nullopt};
  // The first, second and third space point are assumed to be bottom, middle
  // and top space point, respectively
  for (std::size_t isp = 0; isp < 3; ++isp) {
    spacepoint_iterator_t it = std::next(spBegin, isp);
    if (*it == nullptr) {
      ACTS_ERROR("Empty space point found. This should not happen.");
      return std::nullopt;
    }
    const auto& sp = *it;
    spGlobalPositions[isp] = Vector3(sp->x(), sp->y(), sp->z());
    spGlobalTimes[isp] = sp->t();
  }

  // Define a new coordinate frame with its origin at the bottom space point, z
  // axis long the magnetic field direction and y axis perpendicular to vector
  // from the bottom to middle space point. Hence, the projection of the middle
  // space point on the transverse plane will be located at the x axis of the
  // new frame.
  Vector3 relVec = spGlobalPositions[1] - spGlobalPositions[0];
  Vector3 newZAxis = bField.normalized();
  Vector3 newYAxis = newZAxis.cross(relVec).normalized();
  Vector3 newXAxis = newYAxis.cross(newZAxis);
  RotationMatrix3 rotation;
  rotation.col(0) = newXAxis;
  rotation.col(1) = newYAxis;
  rotation.col(2) = newZAxis;
  // The center of the new frame is at the bottom space point
  Translation3 trans(spGlobalPositions[0]);
  // The transform which constructs the new frame
  Transform3 transform(trans * rotation);

  // The coordinate of the middle and top space point in the new frame
  Vector3 local1 = transform.inverse() * spGlobalPositions[1];
  Vector3 local2 = transform.inverse() * spGlobalPositions[2];

  // In the new frame the bottom sp is at the origin, while the middle
  // sp in along the x axis. As such, the x-coordinate of the circle is
  // at: x-middle / 2.
  // The y coordinate can be found by using the straight line passing
  // between the mid point between the middle and top sp and perpendicular to
  // the line connecting them
  Vector2 circleCenter;
  circleCenter(0) = 0.5 * local1(0);

  ActsScalar deltaX21 = local2(0) - local1(0);
  ActsScalar sumX21 = local2(0) + local1(0);
  // straight line connecting the two points
  // y = a * x + c (we don't care about c right now)
  // we simply need the slope
  // we compute 1./a since this is what we need for the following computation
  ActsScalar ia = deltaX21 / local2(1);
  // Perpendicular line is then y = -1/a *x + b
  // we can evaluate b given we know a already by imposing
  // the line passes through P = (0.5 * (x2 + x1), 0.5 * y2)
  ActsScalar b = 0.5 * (local2(1) + ia * sumX21);
  circleCenter(1) = -ia * circleCenter(0) + b;
  // Radius is a signed distance between circleCenter and first sp, which is at
  // (0, 0) in the new frame. Sign depends on the slope a (positive vs negative)
  int sign = ia > 0 ? -1 : 1;
  const ActsScalar R = circleCenter.norm();
  ActsScalar invTanTheta =
      local2.z() / (2.f * R * std::asin(local2.head<2>().norm() / (2.f * R)));
  // The momentum direction in the new frame (the center of the circle has the
  // coordinate (-1.*A/(2*B), 1./(2*B)))
  ActsScalar A = -circleCenter(0) / circleCenter(1);
  Vector3 transDirection(1., A, fastHypot(1, A) * invTanTheta);
  // Transform it back to the original frame
  Vector3 direction = rotation * transDirection.normalized();

  // Initialize the bound parameters vector
  BoundVector params = BoundVector::Zero();

  // The estimated phi and theta
  params[eBoundPhi] = VectorHelpers::phi(direction);
  params[eBoundTheta] = VectorHelpers::theta(direction);

  // Transform the bottom space point to local coordinates of the provided
  // surface
  auto lpResult = surface.globalToLocal(gctx, spGlobalPositions[0], direction);
  if (!lpResult.ok()) {
    ACTS_ERROR(
        "Global to local transformation did not succeed. Please make sure the "
        "bottom space point lies on the provided surface.");
    return std::nullopt;
  }
  Vector2 bottomLocalPos = lpResult.value();
  // The estimated loc0 and loc1
  params[eBoundLoc0] = bottomLocalPos.x();
  params[eBoundLoc1] = bottomLocalPos.y();
  params[eBoundTime] = spGlobalTimes[0].value_or(0.);

  // The estimated q/pt in [GeV/c]^-1 (note that the pt is the projection of
  // momentum on the transverse plane of the new frame)
  ActsScalar qOverPt = sign * (UnitConstants::m) / (0.3 * bFieldInTesla * R);
  // The estimated q/p in [GeV/c]^-1
  params[eBoundQOverP] = qOverPt / fastHypot(1., invTanTheta);

  if (params.hasNaN()) {
    ACTS_ERROR(
        "The NaN value exists at the estimated track parameters from seed with"
        << "\nbottom sp: " << spGlobalPositions[0] << "\nmiddle sp: "
        << spGlobalPositions[1] << "\ntop sp: " << spGlobalPositions[2]);
    return std::nullopt;
  }
  return params;
}

}  // namespace Acts
