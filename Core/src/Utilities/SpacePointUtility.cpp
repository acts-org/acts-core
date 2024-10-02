// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Utilities/SpacePointUtility.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/SpacePointFormation/SpacePointBuilderOptions.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

namespace Acts {

Result<double> SpacePointUtility::differenceOfMeasurementsChecked(
    const Vector3& pos1, const Vector3& pos2, const Vector3& posVertex,
    const double maxDistance, const double maxAngleTheta2,
    const double maxAnglePhi2) const {
  // Check if measurements are close enough to each other
  if ((pos1 - pos2).norm() > maxDistance) {
    return Result<double>::failure(m_error);
  }

  // Calculate the angles of the vectors
  double phi1 = VectorHelpers::phi(pos1 - posVertex);
  double theta1 = VectorHelpers::theta(pos1 - posVertex);
  double phi2 = VectorHelpers::phi(pos2 - posVertex);
  double theta2 = VectorHelpers::theta(pos2 - posVertex);
  // Calculate the squared difference between the theta angles
  double diffTheta2 = (theta1 - theta2) * (theta1 - theta2);
  if (diffTheta2 > maxAngleTheta2) {
    return Result<double>::failure(m_error);
  }
  // Calculate the squared difference between the phi angles
  double diffPhi2 = (phi1 - phi2) * (phi1 - phi2);
  if (diffPhi2 > maxAnglePhi2) {
    return Result<double>::failure(m_error);
  }
  // Return the squared distance between both vector
  return Result<double>::success(diffTheta2 + diffPhi2);
}

std::tuple<Vector3, std::optional<ActsScalar>, Vector2,
           std::optional<ActsScalar>>
SpacePointUtility::globalCoords(
    const GeometryContext& gctx, const SourceLink& slink,
    const SourceLinkSurfaceAccessor& surfaceAccessor, const BoundVector& par,
    const BoundSquareMatrix& cov) const {
  const Surface* surface = surfaceAccessor(slink);
  Vector2 localPos(par[eBoundLoc0], par[eBoundLoc1]);
  SquareMatrix2 localCov = cov.block<2, 2>(eBoundLoc0, eBoundLoc0);
  Vector3 globalPos = surface->localToGlobal(gctx, localPos, Vector3());
  RotationMatrix3 rotLocalToGlobal =
      surface->referenceFrame(gctx, globalPos, Vector3());

  // the space point requires only the variance of the transverse and
  // longitudinal position. reduce computations by transforming the
  // covariance directly from local to rho/z.
  //
  // compute Jacobian from global coordinates to rho/z
  //
  //         rho = sqrt(x² + y²)
  // drho/d{x,y} = (1 / sqrt(x² + y²)) * 2 * {x,y}
  //             = 2 * {x,y} / r
  //       dz/dz = 1
  //
  auto x = globalPos[ePos0];
  auto y = globalPos[ePos1];
  auto scale = 2 / std::hypot(x, y);
  ActsMatrix<2, 3> jacXyzToRhoZ = ActsMatrix<2, 3>::Zero();
  jacXyzToRhoZ(0, ePos0) = scale * x;
  jacXyzToRhoZ(0, ePos1) = scale * y;
  jacXyzToRhoZ(1, ePos2) = 1;
  // compute Jacobian from local coordinates to rho/z
  ActsMatrix<2, 2> jac =
      jacXyzToRhoZ * rotLocalToGlobal.block<3, 2>(ePos0, ePos0);
  // compute rho/z variance
  ActsVector<2> var = (jac * localCov * jac.transpose()).diagonal();

  auto gcov = Vector2(var[0], var[1]);

  // optionally set time
  // TODO the current condition of checking the covariance is not optional but
  // should do for now
  std::optional<ActsScalar> globalTime = par[eBoundTime];
  std::optional<ActsScalar> tcov = cov(eBoundTime, eBoundTime);
  if (tcov.value() <= 0) {
    globalTime = std::nullopt;
    tcov = std::nullopt;
  }

  return std::make_tuple(globalPos, globalTime, gcov, tcov);
}

Vector2 SpacePointUtility::calcRhoZVars(
    const GeometryContext& gctx, const SourceLink& slinkFront,
    const SourceLink& slinkBack,
    const SourceLinkSurfaceAccessor& surfaceAccessor,
    const ParamCovAccessor& paramCovAccessor, const Vector3& globalPos,
    const double theta) const {
  const auto var1 = paramCovAccessor(slinkFront).second(0, 0);
  const auto var2 = paramCovAccessor(slinkBack).second(0, 0);

  // strip1 and strip2 are tilted at +/- theta/2
  double sigma_x = std::hypot(var1, var2) / (2 * sin(theta * 0.5));
  double sigma_y = std::hypot(var1, var2) / (2 * cos(theta * 0.5));

  // projection to the surface with strip1.
  double sig_x1 = sigma_x * cos(0.5 * theta) + sigma_y * sin(0.5 * theta);
  double sig_y1 = sigma_y * cos(0.5 * theta) + sigma_x * sin(0.5 * theta);
  SquareMatrix2 lcov;
  lcov << sig_x1, 0, 0, sig_y1;

  const Surface& surface = *surfaceAccessor(slinkFront);

  auto gcov = rhoZCovariance(gctx, surface, globalPos, lcov);
  return gcov;
}

Vector2 SpacePointUtility::rhoZCovariance(const GeometryContext& gctx,
                                          const Surface& surface,
                                          const Vector3& globalPos,
                                          const SquareMatrix2& localCov) const {
  Vector3 globalFakeMom(1, 1, 1);

  RotationMatrix3 rotLocalToGlobal =
      surface.referenceFrame(gctx, globalPos, globalFakeMom);

  auto x = globalPos[ePos0];
  auto y = globalPos[ePos1];
  auto scale = 2 / std::hypot(x, y);
  ActsMatrix<2, 3> jacXyzToRhoZ = ActsMatrix<2, 3>::Zero();
  jacXyzToRhoZ(0, ePos0) = scale * x;
  jacXyzToRhoZ(0, ePos1) = scale * y;
  jacXyzToRhoZ(1, ePos2) = 1;
  // compute Jacobian from local coordinates to rho/z
  ActsMatrix<2, 2> jac =
      jacXyzToRhoZ * rotLocalToGlobal.block<3, 2>(ePos0, ePos0);
  // compute rho/z variance
  ActsVector<2> var = (jac * localCov * jac.transpose()).diagonal();

  auto gcov = Vector2(var[0], var[1]);

  return gcov;
}

Result<void> SpacePointUtility::calculateStripSPPosition(
    const std::pair<Vector3, Vector3>& stripEnds1,
    const std::pair<Vector3, Vector3>& stripEnds2, const Vector3& posVertex,
    SpacePointParameters& spParams, const double stripLengthTolerance) const {
  /// The following algorithm is meant for finding the position on the first
  /// strip if there is a corresponding Measurement on the second strip. The
  /// resulting point is a point x on the first surfaces. This point is
  /// along a line between the points a (top end of the strip)
  /// and b (bottom end of the strip). The location can be parametrized as
  /// 	2 * x = (1 + m) a + (1 - m) b
  /// as function of the scalar m. m is a parameter in the interval
  /// -1 < m < 1 since the hit was on the strip. Furthermore, the vector
  /// from the vertex to the Measurement on the second strip y is needed to be a
  /// multiple k of the vector from vertex to the hit on the first strip x.
  /// As a consequence of this demand y = k * x needs to be on the
  /// connecting line between the top (c) and bottom (d) end of
  /// the second strip. If both measurements correspond to each other, the
  /// condition
  /// 	y * (c X d) = k * x (c X d) = 0 ("X" represents a cross product)
  /// needs to be fulfilled. Inserting the first equation into this
  /// equation leads to the condition for m as given in the following
  /// algorithm and therefore to the calculation of x.
  /// The same calculation can be repeated for y. Its corresponding
  /// parameter will be named n.

  spParams.firstBtmToTop = stripEnds1.first - stripEnds1.second;
  spParams.secondBtmToTop = stripEnds2.first - stripEnds2.second;
  spParams.vtxToFirstMid2 =
      stripEnds1.first + stripEnds1.second - 2 * posVertex;
  spParams.vtxToSecondMid2 =
      stripEnds2.first + stripEnds2.second - 2 * posVertex;
  spParams.firstBtmToTopXvtxToFirstMid2 =
      spParams.firstBtmToTop.cross(spParams.vtxToFirstMid2);
  spParams.secondBtmToTopXvtxToSecondMid2 =
      spParams.secondBtmToTop.cross(spParams.vtxToSecondMid2);
  spParams.m =
      -spParams.vtxToFirstMid2.dot(spParams.secondBtmToTopXvtxToSecondMid2) /
      spParams.firstBtmToTop.dot(spParams.secondBtmToTopXvtxToSecondMid2);
  spParams.n =
      -spParams.vtxToSecondMid2.dot(spParams.firstBtmToTopXvtxToFirstMid2) /
      spParams.secondBtmToTop.dot(spParams.firstBtmToTopXvtxToFirstMid2);

  // Set the limit for the parameter
  if (spParams.limit == 1. && stripLengthTolerance != 0.) {
    spParams.limit = 1. + stripLengthTolerance;
  }

  // Check if m and n can be resolved in the interval (-1, 1)
  if (fabs(spParams.m) <= spParams.limit &&
      fabs(spParams.n) <= spParams.limit) {
    return Result<void>::success();
  }
  return Result<void>::failure(m_error);
}

Result<void> SpacePointUtility::recoverSpacePoint(
    SpacePointParameters& spParams, double stripLengthGapTolerance) const {
  // Consider some cases that would allow an easy exit
  // Check if the limits are allowed to be increased
  if (stripLengthGapTolerance <= 0.) {
    return Result<void>::failure(m_error);
  }

  spParams.mag_firstBtmToTop = spParams.firstBtmToTop.norm();
  // Increase the limits. This allows a check if the point is just slightly
  // outside the SDE
  spParams.limitExtended =
      spParams.limit + stripLengthGapTolerance / spParams.mag_firstBtmToTop;

  // Check if m is just slightly outside
  if (fabs(spParams.m) > spParams.limitExtended) {
    return Result<void>::failure(m_error);
  }
  // Calculate n if not performed previously
  if (spParams.n == 0.) {
    spParams.n =
        -spParams.vtxToSecondMid2.dot(spParams.firstBtmToTopXvtxToFirstMid2) /
        spParams.secondBtmToTop.dot(spParams.firstBtmToTopXvtxToFirstMid2);
  }
  // Check if n is just slightly outside
  if (fabs(spParams.n) > spParams.limitExtended) {
    return Result<void>::failure(m_error);
  }
  /// The following code considers an overshoot of m and n in the same direction
  /// of their SDE. The term "overshoot" represents the amount of m or n outside
  /// its regular interval (-1, 1).
  /// It calculates which overshoot is worse. In order to compare both, the
  /// overshoot in n is projected onto the first surface by considering the
  /// normalized projection of r onto q.
  /// This allows a rescaling of the overshoot. The worse overshoot will be set
  /// to +/-1, the parameter with less overshoot will be moved towards 0 by the
  /// worse overshoot.
  /// In order to treat both SDEs equally, the rescaling eventually needs to be
  /// performed several times. If these shifts allows m and n to be in the
  /// limits, the space point can be stored.
  /// @note This shift can be understood as a shift of the particle's
  /// trajectory. This is leads to a shift of the vertex. Since these two points
  /// are treated independently from other measurement, it is also possible to
  /// consider this as a change in the slope of the particle's trajectory.
  ///  The would also move the vertex position.

  // Calculate the scaling factor to project lengths of the second SDE on the
  // first SDE
  double secOnFirstScale =
      spParams.firstBtmToTop.dot(spParams.secondBtmToTop) /
      (spParams.mag_firstBtmToTop * spParams.mag_firstBtmToTop);
  // Check if both overshoots are in the same direction
  if (spParams.m > 1. && spParams.n > 1.) {
    // Calculate the overshoots
    double mOvershoot = spParams.m - 1.;
    double nOvershoot =
        (spParams.n - 1.) * secOnFirstScale;  // Perform projection
    // Resolve worse overshoot
    double biggerOvershoot = std::max(mOvershoot, nOvershoot);
    // Move m and n towards 0
    spParams.m -= biggerOvershoot;
    spParams.n -= (biggerOvershoot / secOnFirstScale);
    // Check if this recovered the space point

    if (fabs(spParams.m) < spParams.limit &&
        fabs(spParams.n) < spParams.limit) {
      return Result<void>::success();
    } else {
      return Result<void>::failure(m_error);
    }
  }
  // Check if both overshoots are in the same direction
  if (spParams.m < -1. && spParams.n < -1.) {
    // Calculate the overshoots
    double mOvershoot = -(spParams.m + 1.);
    double nOvershoot =
        -(spParams.n + 1.) * secOnFirstScale;  // Perform projection
    // Resolve worse overshoot
    double biggerOvershoot = std::max(mOvershoot, nOvershoot);
    // Move m and n towards 0
    spParams.m += biggerOvershoot;
    spParams.n += (biggerOvershoot / secOnFirstScale);
    // Check if this recovered the space point
    if (fabs(spParams.m) < spParams.limit &&
        fabs(spParams.n) < spParams.limit) {
      return Result<void>::success();
    }
  }
  // No solution could be found
  return Result<void>::failure(m_error);
}

Result<double> SpacePointUtility::calcPerpendicularProjection(
    const std::pair<Vector3, Vector3>& stripEnds1,
    const std::pair<Vector3, Vector3>& stripEnds2,
    SpacePointParameters& spParams) const {
  /// This approach assumes that no vertex is available. This option aims to
  /// approximate the space points from cosmic data.
  /// The underlying assumption is that the best point is given by the  closest
  /// distance between both lines describing the SDEs.
  /// The point x on the first SDE is parametrized as a + lambda0 * q with  the
  /// top end a of the strip and the vector q = a - b(ottom end of the  strip).
  /// An analogous parametrization is performed of the second SDE with y = c  +
  /// lambda1 * r.
  /// x get resolved by resolving lambda0 from the condition that |x-y| is  the
  /// shortest distance between two skew lines.

  spParams.firstBtmToTop = stripEnds1.first - stripEnds1.second;
  spParams.secondBtmToTop = stripEnds2.first - stripEnds2.second;

  Vector3 ac = stripEnds2.first - stripEnds1.first;
  double qr = (spParams.firstBtmToTop).dot(spParams.secondBtmToTop);
  double denom = spParams.firstBtmToTop.dot(spParams.firstBtmToTop) - qr * qr;
  // Check for numerical stability
  if (fabs(denom) > 1e-6) {
    // Return lambda0
    return Result<double>::success(
        (ac.dot(spParams.secondBtmToTop) * qr -
         ac.dot(spParams.firstBtmToTop) *
             (spParams.secondBtmToTop).dot(spParams.secondBtmToTop)) /
        denom);
  }
  return Result<double>::failure(m_error);
}

}  // namespace Acts
