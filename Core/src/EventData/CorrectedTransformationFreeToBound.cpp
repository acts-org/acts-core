// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/EventData/detail/CorrectedTransformationFreeToBound.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/TransformationHelpers.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <ostream>
#include <type_traits>
#include <utility>
#include <vector>

Acts::FreeToBoundCorrection::FreeToBoundCorrection(bool apply_,
                                                   ActsScalar alpha_,
                                                   ActsScalar beta_)
    : apply(apply_), alpha(alpha_), beta(beta_) {}

Acts::FreeToBoundCorrection::FreeToBoundCorrection(bool apply_)
    : apply(apply_) {}

Acts::FreeToBoundCorrection::operator bool() const {
  return apply;
}

Acts::detail::CorrectedFreeToBoundTransformer::CorrectedFreeToBoundTransformer(
    ActsScalar alpha, ActsScalar beta, ActsScalar cosIncidentAngleMinCutoff,
    ActsScalar cosIncidentAngleMaxCutoff)
    : m_alpha(alpha),
      m_beta(beta),
      m_cosIncidentAngleMinCutoff(cosIncidentAngleMinCutoff),
      m_cosIncidentAngleMaxCutoff(cosIncidentAngleMaxCutoff) {}

Acts::detail::CorrectedFreeToBoundTransformer::CorrectedFreeToBoundTransformer(
    const FreeToBoundCorrection& freeToBoundCorrection) {
  m_alpha = freeToBoundCorrection.alpha;
  m_beta = freeToBoundCorrection.beta;
  m_cosIncidentAngleMinCutoff = freeToBoundCorrection.cosIncidentAngleMinCutoff;
  m_cosIncidentAngleMaxCutoff = freeToBoundCorrection.cosIncidentAngleMaxCutoff;
}

std::optional<std::tuple<Acts::BoundVector, Acts::BoundSquareMatrix>>
Acts::detail::CorrectedFreeToBoundTransformer::operator()(
    const Acts::FreeVector& freeParams,
    const Acts::FreeSquareMatrix& freeCovariance, const Acts::Surface& surface,
    const Acts::GeometryContext& geoContext, Direction navDir,
    const Logger& logger) const {
  // Get the incidence angle
  Vector3 dir = freeParams.segment<3>(eFreeDir0);
  Vector3 normal =
      surface.normal(geoContext, freeParams.segment<3>(eFreePos0), dir);
  ActsScalar absCosIncidenceAng = std::abs(dir.dot(normal));
  // No correction if the incidentAngle is small enough (not necessary ) or too
  // large (correction could be invalid). Fall back to nominal free to bound
  // transformation
  if (absCosIncidenceAng < m_cosIncidentAngleMinCutoff ||
      absCosIncidenceAng > m_cosIncidentAngleMaxCutoff) {
    ACTS_VERBOSE("Incident angle: " << std::acos(absCosIncidenceAng)
                                    << " is out of range for correction");
    return std::nullopt;
  }

  // The number of sigma points
  std::size_t sampleSize = 2 * eFreeSize + 1;
  // The sampled free parameters, the weight for measurement W_m and weight for
  // covariance, W_c
  std::vector<std::tuple<FreeVector, ActsScalar, ActsScalar>> sampledFreeParams;
  sampledFreeParams.reserve(sampleSize);

  // Initialize the covariance sqrt root matrix
  FreeSquareMatrix covSqrt = FreeSquareMatrix::Zero();
  // SVD decomposition: freeCovariance = U*S*U^T here
  Eigen::JacobiSVD<FreeSquareMatrix> svd(
      freeCovariance, Eigen::ComputeFullU | Eigen::ComputeFullV);
  auto S = svd.singularValues();
  FreeMatrix U = svd.matrixU();
  // Get the sqrt root matrix of S
  FreeMatrix D = FreeMatrix::Zero();
  for (unsigned i = 0; i < eFreeSize; ++i) {
    if (S(i) > 0) {
      D(i, i) = std::sqrt(S(i));
    }
  }
  // Get the covariance sqrt root matrix
  covSqrt = U * D;

  // Define kappa = alpha*alpha*N
  ActsScalar kappa = m_alpha * m_alpha * static_cast<double>(eFreeSize);
  // lambda = alpha*alpha*N - N
  ActsScalar lambda = kappa - static_cast<double>(eFreeSize);
  // gamma = sqrt(labmda + N)
  ActsScalar gamma = std::sqrt(kappa);

  // Sample the free parameters
  // 1. the nominal parameter
  sampledFreeParams.push_back(
      {freeParams, lambda / kappa,
       lambda / kappa + (1.0 - m_alpha * m_alpha + m_beta)});
  // 2. the shifted parameters
  for (unsigned i = 0; i < eFreeSize; ++i) {
    sampledFreeParams.push_back(
        {freeParams + covSqrt.col(i) * gamma, 0.5 / kappa, 0.5 / kappa});
    sampledFreeParams.push_back(
        {freeParams - covSqrt.col(i) * gamma, 0.5 / kappa, 0.5 / kappa});
  }

  // Initialize the mean of the bound parameters
  BoundVector bpMean = BoundVector::Zero();
  // Initialize the bound covariance
  BoundSquareMatrix bv = BoundSquareMatrix::Zero();

  // The transformed bound parameters and weight for each sampled free
  // parameters
  std::vector<std::pair<BoundVector, ActsScalar>> transformedBoundParams;

  // 1. The nominal one
  // The sampled free parameters, the weight for measurement W_m and weight for
  // covariance, W_c
  const auto& [paramsNom, mweightNom, cweightNom] = sampledFreeParams[0];
  // Transform the free to bound
  auto nominalRes =
      transformFreeToBoundParameters(paramsNom, surface, geoContext);
  // Not successful, fall back to nominal free to bound transformation
  if (!nominalRes.ok()) {
    ACTS_WARNING(
        "Free to bound transformation for nominal free parameters failed.");
    return std::nullopt;
  }
  auto nominalBound = nominalRes.value();
  transformedBoundParams.push_back({nominalBound, cweightNom});
  bpMean = bpMean + mweightNom * nominalBound;

  // 2. Loop over the rest sample points of the free parameters to get the
  // corrected bound parameters
  for (unsigned i = 1; i < sampledFreeParams.size(); ++i) {
    const auto& [params, mweight, cweight] = sampledFreeParams[i];
    FreeVector correctedFreeParams = params;

    // Reintersect to get the corrected free params without boundary check
    SurfaceIntersection intersection =
        surface
            .intersect(geoContext, params.segment<3>(eFreePos0),
                       navDir * params.segment<3>(eFreeDir0),
                       BoundaryTolerance::Infinite())
            .closest();
    correctedFreeParams.segment<3>(eFreePos0) = intersection.position();

    // Transform the free to bound
    auto result = transformFreeToBoundParameters(correctedFreeParams, surface,
                                                 geoContext);
    // Not successful, fall back to nominal free to bound transformation
    if (!result.ok()) {
      ACTS_WARNING(
          "Free to bound transformation for sampled free parameters: \n"
          << correctedFreeParams << " failed.");
      return std::nullopt;
    }

    auto bp = result.value();
    transformedBoundParams.push_back({bp, cweight});
    bpMean = bpMean + mweight * bp;
  }

  // Get the corrected bound covariance
  for (unsigned isample = 0; isample < sampleSize; ++isample) {
    BoundVector bSigma = transformedBoundParams[isample].first - bpMean;

    bv = bv +
         transformedBoundParams[isample].second * bSigma * bSigma.transpose();
  }

  return std::make_tuple(bpMean, bv);
}
