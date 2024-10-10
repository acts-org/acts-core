// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <optional>
#include <tuple>

namespace Acts {

/// @brief Free to bound transformation Correction configuration class
///
struct FreeToBoundCorrection {
  /// Apply correction or not
  bool apply = false;

  /// UKF tuning parameters
  ActsScalar alpha = 0.1;
  ActsScalar beta = 2;

  /// The cutoff of incident angles cosine for correction
  ActsScalar cosIncidentAngleMinCutoff = 1e-5;
  ActsScalar cosIncidentAngleMaxCutoff = 0.99500417;

  /// Default constructor
  FreeToBoundCorrection() = default;

  /// Construct from boolean and UKF parameters (alpha, beta)
  ///
  /// @param apply_ Whether to apply correction
  /// @param alpha_ The UKF tuning parameter alpha
  /// @param beta_ The UKF tuning parameter beta
  FreeToBoundCorrection(bool apply_, ActsScalar alpha_, ActsScalar beta_);

  /// Construct from boolean only
  ///
  /// @param apply_ Whether to apply correction
  explicit FreeToBoundCorrection(bool apply_);

  /// Return boolean for applying correction or not
  operator bool() const;
};

namespace detail {

/// @brief Corrected free to bound transform class based on covariance matrix sqrt root in UKF: https://doi.org/10.1117/12.280797
///
struct CorrectedFreeToBoundTransformer {
  /// Construct from boolean, UKF tuning parameters (alpha, beta) and incident
  /// angle cutoff for correction
  ///
  /// @param alpha The UKF tuning parameter alpha
  /// @param beta The UKF tuning parameter beta
  /// @param cosIncidentAngleMinCutoff The cosine of max incident angle
  /// @param cosIncidentAngleMaxCutoff The cosine of min incident angle
  CorrectedFreeToBoundTransformer(ActsScalar alpha, ActsScalar beta,
                                  ActsScalar cosIncidentAngleMinCutoff,
                                  ActsScalar cosIncidentAngleMaxCutoff);

  /// Construct from a FreeToBoundCorrection
  ///
  /// @param freeToBoundCorrection The freeToBoundCorrection object
  CorrectedFreeToBoundTransformer(
      const FreeToBoundCorrection& freeToBoundCorrection);

  /// Default constructors
  CorrectedFreeToBoundTransformer() = default;
  CorrectedFreeToBoundTransformer(const CorrectedFreeToBoundTransformer&) =
      default;
  CorrectedFreeToBoundTransformer(CorrectedFreeToBoundTransformer&&) = default;
  CorrectedFreeToBoundTransformer& operator=(
      const CorrectedFreeToBoundTransformer&) = default;
  CorrectedFreeToBoundTransformer& operator=(
      CorrectedFreeToBoundTransformer&&) = default;

  /// Get the non-linearity corrected bound parameters and its covariance
  ///
  /// @param freeParams The free parameters vector
  /// @param freeCovariance The free parameters covariance
  /// @param Surface The surface of the bound parameters being represented
  /// @param geoContext The geometry context
  /// @param navDir The navigation direction
  /// @param logger The logger
  std::optional<std::tuple<BoundVector, BoundSquareMatrix>> operator()(
      const FreeVector& freeParams, const FreeSquareMatrix& freeCovariance,
      const Surface& surface, const GeometryContext& geoContext,
      Direction navDir = Direction::Forward,
      const Logger& logger = getDummyLogger()) const;

 private:
  /// The parameters to tune the weight in UKF (0 < alpha <=1)
  ActsScalar m_alpha = 0.1;
  ActsScalar m_beta = 2;

  /// The maximum incident angle (i.e. minimum cos incident angle) cutoff for
  /// correction
  ActsScalar m_cosIncidentAngleMinCutoff = 1e-5;

  /// The minimum incident angle (i.e. maximum cos incident angle) cutoff for
  /// correction, note cos(0.1) = 0.99500417
  ActsScalar m_cosIncidentAngleMaxCutoff = 0.99500417;
};

}  // namespace detail
}  // namespace Acts
