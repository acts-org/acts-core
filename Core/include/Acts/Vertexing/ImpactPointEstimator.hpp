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
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "Acts/Vertexing/Vertex.hpp"

namespace Acts {

struct ImpactParametersAndSigma {
  // Impact parameters ...
  double d0 = 0.;
  double z0 = 0.;
  // ... and their standard deviations wrt a vertex, e.g.:
  // sigmaD0 = sqrt(Var(X) + Var(Y) + Var(d0)),
  // where X and Y are the x- and y-coordinate of the vertex
  double sigmaD0 = 0.;
  double sigmaZ0 = 0.;
  // Absolute difference in time between the vertex and the track at the 2D PCA
  // ...
  std::optional<double> deltaT = std::nullopt;
  // ... and standard deviation wrt a vertex
  std::optional<double> sigmaDeltaT = std::nullopt;
};

/// @class ImpactPointEstimator
///
/// @brief Estimator for impact point calculations
/// A description of the underlying mathematics can be found here:
/// https://github.com/acts-project/acts/pull/2506
/// TODO: Upload reference at a better place
class ImpactPointEstimator {
 public:
  /// State struct
  struct State {
    /// Magnetic field cache
    MagneticFieldProvider::Cache fieldCache;
  };

  struct Config {
    /// @brief Config constructor if magnetic field is present
    ///
    /// @param bIn The magnetic field
    /// @param prop The propagator
    Config(std::shared_ptr<const MagneticFieldProvider> bIn,
           std::shared_ptr<const BasePropagator> prop)
        : bField(std::move(bIn)), propagator(std::move(prop)) {}

    /// @brief Config constructor without B field -> uses NullBField
    /// provided)
    ///
    /// @param prop The propagator
    Config(std::shared_ptr<const BasePropagator> prop)
        : bField{std::make_shared<NullBField>()}, propagator(std::move(prop)) {}

    /// Magnetic field
    std::shared_ptr<const MagneticFieldProvider> bField;
    /// Propagator
    std::shared_ptr<const BasePropagator> propagator;
    /// Max. number of iterations in Newton method
    int maxIterations = 20;
    /// Desired precision of deltaPhi in Newton method
    double precision = 1.e-10;
  };

  /// @brief Constructor
  ///
  /// @param cfg Configuration object
  /// @param logger Logging instance
  ImpactPointEstimator(const Config& cfg,
                       std::unique_ptr<const Logger> logger = getDefaultLogger(
                           "ImpactPointEstimator", Logging::INFO))
      : m_cfg(cfg), m_logger(std::move(logger)) {}

  /// @brief Copy constructor to clone logger (class owns a unique pointer to it,
  /// which can't be copied)
  ///
  /// @param other Impact point estimator to be cloned
  ImpactPointEstimator(const ImpactPointEstimator& other)
      : m_cfg(other.m_cfg), m_logger(other.logger().clone()) {}

  /// Move constructor for impact point estimator
  ImpactPointEstimator(ImpactPointEstimator&&) = default;

  /// @brief Calculates 3D distance between a track and a vertex
  ///
  /// @param gctx The geometry context
  /// @param trkParams Track parameters
  /// @param vtxPos 3D position to calculate the distance to
  /// @param state The state object
  ///
  /// @return Distance
  Result<double> calculateDistance(const GeometryContext& gctx,
                                   const BoundTrackParameters& trkParams,
                                   const Vector3& vtxPos, State& state) const;

  /// @brief Estimates the track parameters at the 3D PCA (i.e., a point of
  /// minimal 3D distance) to a vertex. The track parameters are defined wrt a
  /// reference plane that has its origin at the vertex position and whose
  /// z-axis points in the direction of the track momentum. The plane's x-axis
  /// points approximately from the vertex to the 3D PCA (it is only approximate
  /// because we force it to be orthogonal to the z-axis). The y-axis is
  /// calculated as a cross product between x- and z-axis.
  ///
  /// @param gctx The geometry context
  /// @param mctx The magnetic field context
  /// @param trkParams Track parameters
  /// @param vtxPos Reference position (vertex)
  /// @param state The state object
  ///
  /// @return Track parameters at the 3D PCA
  Result<BoundTrackParameters> estimate3DImpactParameters(
      const GeometryContext& gctx, const Acts::MagneticFieldContext& mctx,
      const BoundTrackParameters& trkParams, const Vector3& vtxPos,
      State& state) const;

  /// @brief Estimates the compatibility of a track to a vertex based on their
  /// 3D (if nDim = 3) or 4D (if nDim = 4) distance and the track covariance.
  /// @note Confusingly, a *smaller* compatibility means that a track is *more*
  /// compatible.
  ///
  /// @tparam nDim Number of dimensions used to compute compatibility
  /// @note If @p nDim = 3 we only consider spatial dimensions; if nDim = 4, we
  ///       also consider time. Other values are not allowed.
  /// @param gctx The Geometry context
  /// @param trkParams Track parameters at point of closest
  /// approach in 3D as retrieved by estimate3DImpactParameters
  /// @param vertexPos The vertex position
  ///
  /// @return The compatibility value
  template <int nDim>
  Result<double> getVertexCompatibility(
      const GeometryContext& gctx, const BoundTrackParameters* trkParams,
      const ActsVector<nDim>& vertexPos) const {
    static_assert(nDim == 3 || nDim == 4,
                  "Only 3D and 4D vertex positions allowed");
    return getVertexCompatibility(gctx, trkParams, {vertexPos.data(), nDim});
  }

  /// @brief Calculate the distance between a track and a vertex by finding the
  /// corresponding 3D PCA. Returns also the momentum direction at the 3D PCA.
  /// The template parameter nDim determines whether we calculate the 3D
  /// distance (nDim = 3) or the 4D distance (nDim = 4) to the 3D PCA.
  /// @note For straight tracks we use an analytical solution; for helical
  /// tracks we use the Newton method.
  ///
  /// @tparam nDim Number of dimensions used to compute compatibility
  /// @note If @p nDim = 3 we only consider spatial dimensions; if nDim = 4, we
  ///       also consider time. Other values are not allowed.
  /// @param gctx Geometry context
  /// @param trkParams Track parameters
  /// @param vtxPos Vertex position
  /// @param state The state object
  template <int nDim>
  Result<std::pair<Acts::ActsVector<nDim>, Acts::Vector3>>
  getDistanceAndMomentum(const GeometryContext& gctx,
                         const BoundTrackParameters& trkParams,
                         const ActsVector<nDim>& vtxPos, State& state) const {
    static_assert(nDim == 3 || nDim == 4,
                  "Only 3D and 4D vertex positions allowed");
    auto res =
        getDistanceAndMomentum(gctx, trkParams, {vtxPos.data(), nDim}, state);
    if (!res.ok()) {
      return res.error();
    }
    auto& [distance, momentum] = *res;
    return std::pair{distance.template head<nDim>(), momentum};
  }

  /// @brief Calculates the impact parameters of a track w.r.t. a vertex. The
  /// corresponding errors are approximated by summing the variances of the
  /// track and the vertex.
  ///
  /// @param track Track whose impact parameters are calculated
  /// @param vtx Vertex corresponding to the track
  /// @param gctx The geometry context
  /// @param mctx The magnetic field context
  /// @param calculateTimeIP If true, the difference in time is computed
  Result<ImpactParametersAndSigma> getImpactParameters(
      const BoundTrackParameters& track, const Vertex& vtx,
      const GeometryContext& gctx, const MagneticFieldContext& mctx,
      bool calculateTimeIP = false) const;

  /// @brief Estimates the sign of the 2D and Z lifetime of a given track
  /// w.r.t. a vertex and a direction (e.g. a jet direction)
  /// by propagating the trajectory state towards the vertex position
  /// and computing the scalar product with the direction vector
  ///
  /// @param track Track to estimate the IP from
  /// @param vtx   Vertex the track belongs to
  /// @param direction   The direction
  /// @param gctx  The geometry context
  /// @param mctx  The magnetic field context
  ///
  /// @return A pair holding the sign for the 2D and Z lifetimes
  Result<std::pair<double, double>> getLifetimeSignOfTrack(
      const BoundTrackParameters& track, const Vertex& vtx,
      const Acts::Vector3& direction, const GeometryContext& gctx,
      const MagneticFieldContext& mctx) const;

  /// @brief Estimates the sign of the 3D lifetime of a given track
  /// w.r.t. a vertex and a direction (e.g. a jet direction)
  ///
  /// @param track Track to estimate the IP from
  /// @param vtx   Vertex the track belongs to
  /// @param direction   The direction
  /// @param gctx  The geometry context
  /// @param mctx  The magnetic field context
  ///
  /// @return The value of the 3D lifetime
  Result<double> get3DLifetimeSignOfTrack(
      const BoundTrackParameters& track, const Vertex& vtx,
      const Acts::Vector3& direction, const GeometryContext& gctx,
      const MagneticFieldContext& mctx) const;

 private:
  Result<std::pair<Acts::Vector4, Acts::Vector3>> getDistanceAndMomentum(
      const GeometryContext& gctx, const BoundTrackParameters& trkParams,
      Eigen::Map<const ActsDynamicVector> vtxPos, State& state) const;

  Result<double> getVertexCompatibility(
      const GeometryContext& gctx, const BoundTrackParameters* trkParams,
      Eigen::Map<const ActsDynamicVector> vertexPos) const;

  /// Configuration object
  const Config m_cfg;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to logging instance
  const Logger& logger() const { return *m_logger; }
};

}  // namespace Acts
