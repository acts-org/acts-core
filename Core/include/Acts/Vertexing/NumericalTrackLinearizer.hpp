// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/MagneticField/NullBField.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/LinearizedTrack.hpp"

namespace Acts {

/// @class NumericalTrackLinearizer
/// Linearizes the track parameters at the PCA to a user-provided
/// point (linPoint). The track parameters are written as a function
/// of the global 4D PCA position and the momentum of the particle at
/// the PCA (i.e., (phi, theta, q/p)). The linearization then reads
/// (see Eq. 5.7 in Ref(1)):
///
/// q = A (r - r_0) + B (p - p_0) + c,
///
/// where q are the Perigee parameters wrt linPoint, {r_0} r is the {initial}
/// 4D PCA position, {p_0} p is the {initial} momentum at the PCA, and c is
/// the constant term of the expansion. A and B are matrices of derivatives,
/// denoted hereafter as "positionJacobian" and "momentumJacobian" respectively.
/// Note that, unlike in Ref. (1), we add the time to the parametrization, which
/// adds a row and a column to A and a row to B.
///
/// This class computes A and B by wiggling one of the 7 parameters
/// at the PCA and computing the new PCA wrt linPoint. The derivatives wrt
/// the k-th parameter pk are then calculated via
///
/// (q(p1, p2, ..., pk+delta, ... p7) - q(p1, p2, ..., pk, ... p7))/delta,
///
/// where q(p1, p2, ..., pk+delta, ... p7) are the new Perigee parameters
/// (corresponding to the new PCA to linPoint). Note that p1 corresponds to
/// the x-position of the PCA, p2 corresponds to the y-position of the PCA, etc.
///
/// @note Connection to RiddersPropagator: The RiddersPropagator does a very
/// similar thing to what this class does, but it wiggles BoundTrackParameters
/// (FreeTrackParameters could also be used if Propagator.hpp and Propagator.ipp
/// were adapted to accommodate them). Here, we wiggle neither
/// BoundTrackParameters nor FreeTrackParameters, but rather the parameters
/// described above.
///
/// Ref.(1) - CERN-THESIS-2010-027, Giacinto Piacquadio (Freiburg U.)
class NumericalTrackLinearizer {
 public:
  /// @brief Configuration struct
  struct Config {
    /// @ Config constructor if magnetic field is present
    ///
    /// @param bIn The magnetic field
    /// @param prop The propagator
    Config(std::shared_ptr<const MagneticFieldProvider> bIn,
           std::shared_ptr<const BasePropagator> prop)
        : bField(std::move(bIn)), propagator(std::move(prop)) {}

    /// @brief Config constructor without B field -> uses NullBField
    ///
    /// @param prop Propagator
    Config(std::shared_ptr<const BasePropagator> prop)
        : bField{std::make_shared<NullBField>()}, propagator(std::move(prop)) {}

    std::shared_ptr<const MagneticFieldProvider> bField;

    std::shared_ptr<const BasePropagator> propagator;

    /// Tolerance determining how close we need to get to a surface to
    /// reach it during propagation
    ActsScalar targetTolerance = 1e-12;

    /// Setting size of the perturbation delta for calculation of numerical
    /// derivatives (i.e., f'(x) ~ (f(x+delta) - f(x)) / delta)
    ActsScalar delta = 1e-8;
  };

  /// @brief Constructor
  ///
  /// @param config Configuration object
  /// @param _logger Logger instance
  NumericalTrackLinearizer(const Config& config,
                           std::unique_ptr<const Logger> _logger =
                               getDefaultLogger("NumTrkLinProp", Logging::INFO))
      : m_cfg(config), m_logger{std::move(_logger)} {}

  /// @brief Function that linearizes BoundTrackParameters at
  /// the PCA to a given Perigee surface
  ///
  /// @param params Parameters to linearize
  /// @param linPointTime Time associated to the linearization point
  /// @note Transverse plane of the Perigee corresponding to @p linPoint is
  /// parallel to the global x-y plane
  /// @param perigeeSurface Perigee surface belonging to @p linPoint
  /// @param gctx Geometry context
  /// @param mctx Magnetic field context
  ///
  /// @return Linearized track
  Result<LinearizedTrack> linearizeTrack(
      const BoundTrackParameters& params, double linPointTime,
      const Surface& perigeeSurface, const Acts::GeometryContext& gctx,
      const Acts::MagneticFieldContext& mctx,
      MagneticFieldProvider::Cache& /*fieldCache*/) const;

 private:
  const Config m_cfg;

  std::unique_ptr<const Logger> m_logger;

  const Logger& logger() const { return *m_logger; }
};

}  // namespace Acts
