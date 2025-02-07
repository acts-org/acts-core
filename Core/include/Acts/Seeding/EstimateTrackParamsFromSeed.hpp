// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Zip.hpp"

#include <array>
#include <optional>
#include <stdexcept>

namespace Acts {

/// Estimate the full track parameters from three space points
///
/// This method is based on the conformal map transformation. It estimates the
/// full free track parameters, i.e. (x, y, z, t, dx, dy, dz, q/p) at the bottom
/// space point. The bottom space is assumed to be the first element in the
/// range defined by the iterators. The magnetic field (which might be along any
/// direction) is also necessary for the momentum estimation.
///
/// This is a purely spatial estimation, i.e. the time parameter will be set to
/// 0.
///
/// It resembles the method used in ATLAS for the track parameters estimated
/// from seed, i.e. the function InDet::SiTrackMaker_xk::getAtaPlane here:
/// https://acode-browser.usatlas.bnl.gov/lxr/source/athena/InnerDetector/InDetRecTools/SiTrackMakerTool_xk/src/SiTrackMaker_xk.cxx
///
/// @tparam spacepoint_iterator_t  The type of space point iterator
///
/// @param sp0 is the bottom space point
/// @param sp1 is the middle space point
/// @param sp2 is the top space point
/// @param bField is the magnetic field vector
///
/// @return the free parameters
FreeVector estimateTrackParamsFromSeed(const Vector3& sp0, const Vector3& sp1,
                                       const Vector3& sp2,
                                       const Vector3& bField);

/// Estimate the full track parameters from three space points
///
/// This method is based on the conformal map transformation. It estimates the
/// full free track parameters, i.e. (x, y, z, t, dx, dy, dz, q/p) at the bottom
/// space point. The bottom space is assumed to be the first element in the
/// range defined by the iterators. The magnetic field (which might be along any
/// direction) is also necessary for the momentum estimation.
///
/// It resembles the method used in ATLAS for the track parameters estimated
/// from seed, i.e. the function InDet::SiTrackMaker_xk::getAtaPlane here:
/// https://acode-browser.usatlas.bnl.gov/lxr/source/athena/InnerDetector/InDetRecTools/SiTrackMakerTool_xk/src/SiTrackMaker_xk.cxx
///
/// @tparam spacepoint_iterator_t  The type of space point iterator
///
/// @param spRange is the range of space points
/// @param bField is the magnetic field vector
///
/// @return the free parameters
template <std::ranges::range spacepoint_range_t>
FreeVector estimateTrackParamsFromSeed(spacepoint_range_t spRange,
                                       const Vector3& bField) {
  // Check the number of provided space points
  if (spRange.size() != 3) {
    throw std::invalid_argument(
        "There should be exactly three space points provided.");
  }

  // The global positions of the bottom, middle and space points
  std::array<Vector3, 3> spPositions = {Vector3::Zero(), Vector3::Zero(),
                                        Vector3::Zero()};
  std::array<std::optional<double>, 3> spTimes = {std::nullopt, std::nullopt,
                                                  std::nullopt};
  // The first, second and third space point are assumed to be bottom, middle
  // and top space point, respectively
  for (auto [sp, spPosition, spTime] :
       Acts::zip(spRange, spPositions, spTimes)) {
    if (sp == nullptr) {
      throw std::invalid_argument("Empty space point found.");
    }
    spPosition = Vector3(sp->x(), sp->y(), sp->z());
    spTime = sp->t();
  }

  FreeVector params = estimateTrackParamsFromSeed(
      spPositions[0], spPositions[1], spPositions[2], bField);
  params[eFreeTime] = spTimes[0].value_or(0);
  return params;
}

/// Estimate the full track parameters from three space points
///
/// This method is based on the conformal map transformation. It estimates the
/// full bound track parameters, i.e. (loc0, loc1, phi, theta, q/p, t) at the
/// bottom space point. The bottom space is assumed to be the first element
/// in the range defined by the iterators. It must lie on the surface provided
/// for the representation of the bound track parameters. The magnetic field
/// (which might be along any direction) is also necessary for the momentum
/// estimation.
///
/// It resembles the method used in ATLAS for the track parameters estimated
/// from seed, i.e. the function InDet::SiTrackMaker_xk::getAtaPlane here:
/// https://acode-browser.usatlas.bnl.gov/lxr/source/athena/InnerDetector/InDetRecTools/SiTrackMakerTool_xk/src/SiTrackMaker_xk.cxx
///
/// @tparam spacepoint_iterator_t  The type of space point iterator
///
/// @param gctx is the geometry context
/// @param spRange is the range of space points
/// @param surface is the surface of the bottom space point. The estimated bound
/// track parameters will be represented also at this surface
/// @param bField is the magnetic field vector
///
/// @return bound parameters
template <std::ranges::range spacepoint_range_t>
Result<BoundVector> estimateTrackParamsFromSeed(const GeometryContext& gctx,
                                                spacepoint_range_t spRange,
                                                const Surface& surface,
                                                const Vector3& bField) {
  FreeVector freeParams = estimateTrackParamsFromSeed(spRange, bField);

  const auto* sp0 = *spRange.begin();
  Vector3 origin = Vector3(sp0->x(), sp0->y(), sp0->z());
  Vector3 direction = freeParams.segment<3>(eFreeDir0);

  BoundVector params = BoundVector::Zero();
  params[eBoundPhi] = VectorHelpers::phi(direction);
  params[eBoundTheta] = VectorHelpers::theta(direction);
  params[eBoundQOverP] = freeParams[eFreeQOverP];

  // Transform the bottom space point to local coordinates of the provided
  // surface
  auto lpResult = surface.globalToLocal(gctx, origin, direction);
  if (!lpResult.ok()) {
    return Result<BoundVector>::failure(lpResult.error());
  }
  Vector2 bottomLocalPos = lpResult.value();
  // The estimated loc0 and loc1
  params[eBoundLoc0] = bottomLocalPos.x();
  params[eBoundLoc1] = bottomLocalPos.y();
  params[eBoundTime] = sp0->t().value_or(0);

  return Result<BoundVector>::success(params);
}

/// Configuration for the estimation of the covariance matrix of the track
/// parameters with `estimateTrackParamCovariance`.
struct EstimateTrackParamCovarianceConfig {
  /// The initial sigmas for the track parameters
  BoundVector initialSigmas = {1. * UnitConstants::mm,
                               1. * UnitConstants::mm,
                               1. * UnitConstants::degree,
                               1. * UnitConstants::degree,
                               1. * UnitConstants::e / UnitConstants::GeV,
                               1. * UnitConstants::ns};

  /// The initial relative uncertainty of the q/pt
  double initialSigmaPtRel = 0.1;

  /// The inflation factors for the variances of the track parameters
  BoundVector initialVarInflation = {1., 1., 1., 1., 1., 1.};
  /// The inflation factor for time uncertainty if the time parameter was not
  /// estimated
  double noTimeVarInflation = 100.;
};

/// Estimate the covariance matrix of the given track parameters based on the
/// provided configuration. The assumption is that we can model the uncertainty
/// of the track parameters as a diagonal matrix with the provided initial
/// sigmas. The inflation factors are used to inflate the initial variances
/// based on the provided configuration. The uncertainty of q/p is estimated
/// based on the relative uncertainty of the q/pt and the theta uncertainty.
///
/// @param config is the configuration for the estimation
/// @param params is the track parameters
/// @param hasTime is true if the track parameters have time
///
/// @return the covariance matrix of the track parameters
BoundMatrix estimateTrackParamCovariance(
    const EstimateTrackParamCovarianceConfig& config, const BoundVector& params,
    bool hasTime);

}  // namespace Acts
