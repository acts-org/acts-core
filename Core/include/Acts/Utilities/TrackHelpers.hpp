// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MeasurementHelpers.hpp"
#include "Acts/EventData/MultiTrajectoryHelpers.hpp"
#include "Acts/EventData/TrackContainerFrontendConcept.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/TrackProxyConcept.hpp"
#include "Acts/EventData/TrackStateProxyConcept.hpp"
#include "Acts/EventData/TrackStateType.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Propagator/StandardAborters.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/TrackFitting/GainMatrixSmoother.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"

#include <utility>

namespace Acts {

enum class TrackExtrapolationStrategy {
  /// Use the first track state to reach target surface
  first,
  /// Use the last track state to reach target surface
  last,
  /// Use the first or last track state to reach target surface depending on the
  /// distance
  firstOrLast,
};

enum class TrackExtrapolationError {
  CompatibleTrackStateNotFound = 1,
  ReferenceSurfaceUnreachable = 2,
};

std::error_code make_error_code(TrackExtrapolationError e);

template <TrackProxyConcept track_proxy_t>
Result<typename track_proxy_t::ConstTrackStateProxy> findFirstMeasurementState(
    const track_proxy_t &track) {
  using TrackStateProxy = typename track_proxy_t::ConstTrackStateProxy;

  // TODO specialize if track is forward linked

  auto result = Result<TrackStateProxy>::failure(
      TrackExtrapolationError::CompatibleTrackStateNotFound);

  for (const auto &trackState : track.trackStatesReversed()) {
    bool isMeasurement =
        trackState.typeFlags().test(TrackStateFlag::MeasurementFlag);
    bool isOutlier = trackState.typeFlags().test(TrackStateFlag::OutlierFlag);

    if (isMeasurement && !isOutlier) {
      result = trackState;
    }
  }

  return result;
}

template <TrackProxyConcept track_proxy_t>
Result<typename track_proxy_t::ConstTrackStateProxy> findLastMeasurementState(
    const track_proxy_t &track) {
  using TrackStateProxy = typename track_proxy_t::ConstTrackStateProxy;

  for (const auto &trackState : track.trackStatesReversed()) {
    bool isMeasurement =
        trackState.typeFlags().test(TrackStateFlag::MeasurementFlag);
    bool isOutlier = trackState.typeFlags().test(TrackStateFlag::OutlierFlag);

    if (isMeasurement && !isOutlier) {
      return trackState;
    }
  }

  return Result<TrackStateProxy>::failure(
      TrackExtrapolationError::CompatibleTrackStateNotFound);
}

/// @brief Smooth a track using the gain matrix smoother
///
/// @tparam track_proxy_t The track proxy type
/// @tparam smoother_t The smoother type
///
/// @param geoContext The geometry context
/// @param track The track to smooth
/// @param logger The logger
/// @param smoother The smoother
///
/// @return The result of the smoothing
template <TrackProxyConcept track_proxy_t,
          typename smoother_t = GainMatrixSmoother>
Result<void> smoothTrack(
    const GeometryContext &geoContext, track_proxy_t &track,
    const Logger &logger = *getDefaultLogger("TrackSmoother", Logging::INFO),
    smoother_t smoother = GainMatrixSmoother()) {
  auto &trackContainer = track.container();
  auto &trackStateContainer = trackContainer.trackStateContainer();

  auto last = findLastMeasurementState(track);
  if (!last.ok()) {
    ACTS_ERROR("no last track state found");
    return last.error();
  }

  auto smoothingResult =
      smoother(geoContext, trackStateContainer, last->index(), logger);

  if (!smoothingResult.ok()) {
    ACTS_ERROR("Smoothing track " << track.index() << " failed with error "
                                  << smoothingResult.error());
    return smoothingResult.error();
  }

  return Result<void>::success();
}

/// @brief Smooth tracks using the gain matrix smoother
///
/// @tparam track_container_t The track container type
///
/// @param geoContext The geometry context
/// @param trackContainer The track container
/// @param logger The logger
///
/// @return The result of the smoothing
template <TrackContainerFrontend track_container_t>
Result<void> smoothTracks(
    const GeometryContext &geoContext, const track_container_t &trackContainer,
    const Logger &logger = *getDefaultLogger("TrackSmoother", Logging::INFO)) {
  Result<void> result = Result<void>::success();

  for (const auto &track : trackContainer) {
    auto smoothingResult = smoothTrack(geoContext, track, logger);

    // Only keep the first error
    if (!smoothingResult.ok() && result.ok()) {
      result = smoothingResult.error();
    }
  }

  return result;
}

/// @brief Find a track state for extrapolation
///
/// @tparam track_proxy_t The track proxy type
///
/// @param geoContext The geometry context
/// @param track The track
/// @param referenceSurface The reference surface
/// @param strategy The extrapolation strategy
/// @param logger The logger
///
/// @return The result of the search containing the track state
///         and the distance to the reference surface
template <TrackProxyConcept track_proxy_t>
Result<std::pair<typename track_proxy_t::ConstTrackStateProxy, double>>
findTrackStateForExtrapolation(
    const GeometryContext &geoContext, const track_proxy_t &track,
    const Surface &referenceSurface, TrackExtrapolationStrategy strategy,
    const Logger &logger = *getDefaultLogger("TrackExtrapolation",
                                             Logging::INFO)) {
  using TrackStateProxy = typename track_proxy_t::ConstTrackStateProxy;

  auto intersect = [&](const TrackStateProxy &state) -> SurfaceIntersection {
    assert(state.hasSmoothed() || state.hasFiltered());

    FreeVector freeVector;
    if (state.hasSmoothed()) {
      freeVector = MultiTrajectoryHelpers::freeSmoothed(geoContext, state);
    } else {
      freeVector = MultiTrajectoryHelpers::freeFiltered(geoContext, state);
    }

    return referenceSurface
        .intersect(geoContext, freeVector.template segment<3>(eFreePos0),
                   freeVector.template segment<3>(eFreeDir0),
                   BoundaryTolerance::None(), s_onSurfaceTolerance)
        .closest();
  };

  switch (strategy) {
    case TrackExtrapolationStrategy::first: {
      ACTS_VERBOSE("looking for first track state");

      auto first = findFirstMeasurementState(track);
      if (!first.ok()) {
        ACTS_ERROR("no first track state found");
        return first.error();
      }

      SurfaceIntersection intersection = intersect(*first);
      if (!intersection.isValid()) {
        ACTS_ERROR("no intersection found");
        return Result<std::pair<TrackStateProxy, double>>::failure(
            TrackExtrapolationError::ReferenceSurfaceUnreachable);
      }

      ACTS_VERBOSE("found intersection at " << intersection.pathLength());
      return std::pair(*first, intersection.pathLength());
    }

    case TrackExtrapolationStrategy::last: {
      ACTS_VERBOSE("looking for last track state");

      auto last = findLastMeasurementState(track);
      if (!last.ok()) {
        ACTS_ERROR("no last track state found");
        return last.error();
      }

      SurfaceIntersection intersection = intersect(*last);
      if (!intersection.isValid()) {
        ACTS_ERROR("no intersection found");
        return Result<std::pair<TrackStateProxy, double>>::failure(
            TrackExtrapolationError::ReferenceSurfaceUnreachable);
      }

      ACTS_VERBOSE("found intersection at " << intersection.pathLength());
      return std::pair(*last, intersection.pathLength());
    }

    case TrackExtrapolationStrategy::firstOrLast: {
      ACTS_VERBOSE("looking for first or last track state");

      auto first = findFirstMeasurementState(track);
      if (!first.ok()) {
        ACTS_ERROR("no first track state found");
        return first.error();
      }

      auto last = findLastMeasurementState(track);
      if (!last.ok()) {
        ACTS_ERROR("no last track state found");
        return last.error();
      }

      SurfaceIntersection intersectionFirst = intersect(*first);
      SurfaceIntersection intersectionLast = intersect(*last);

      double absDistanceFirst = std::abs(intersectionFirst.pathLength());
      double absDistanceLast = std::abs(intersectionLast.pathLength());

      if (intersectionFirst.isValid() && absDistanceFirst <= absDistanceLast) {
        ACTS_VERBOSE("using first track state with intersection at "
                     << intersectionFirst.pathLength());
        return std::pair(*first, intersectionFirst.pathLength());
      }

      if (intersectionLast.isValid() && absDistanceLast <= absDistanceFirst) {
        ACTS_VERBOSE("using last track state with intersection at "
                     << intersectionLast.pathLength());
        return std::pair(*last, intersectionLast.pathLength());
      }

      ACTS_ERROR("no intersection found");
      return Result<std::pair<TrackStateProxy, double>>::failure(
          TrackExtrapolationError::ReferenceSurfaceUnreachable);
    }
  }

  // unreachable
  return Result<std::pair<TrackStateProxy, double>>::failure(
      TrackExtrapolationError::CompatibleTrackStateNotFound);
}

/// @brief Extrapolate a track to a reference surface
///
/// @tparam track_proxy_t The track proxy type
/// @tparam propagator_t The propagator type
/// @tparam propagator_options_t The propagator options type
///
/// @param track The track which is modified in-place
/// @param referenceSurface The reference surface
/// @param propagator The propagator
/// @param options The propagator options
/// @param strategy The extrapolation strategy
/// @param logger The logger
///
/// @return The result of the extrapolation
template <TrackProxyConcept track_proxy_t, typename propagator_t,
          typename propagator_options_t>
Result<void> extrapolateTrackToReferenceSurface(
    track_proxy_t &track, const Surface &referenceSurface,
    const propagator_t &propagator, propagator_options_t options,
    TrackExtrapolationStrategy strategy,
    const Logger &logger = *getDefaultLogger("TrackExtrapolation",
                                             Logging::INFO)) {
  auto findResult = findTrackStateForExtrapolation(
      options.geoContext, track, referenceSurface, strategy, logger);

  if (!findResult.ok()) {
    ACTS_ERROR("failed to find track state for extrapolation");
    return findResult.error();
  }

  auto &[trackState, distance] = *findResult;

  options.direction = Direction::fromScalarZeroAsPositive(distance);

  BoundTrackParameters parameters = track.createParametersFromState(trackState);
  ACTS_VERBOSE("extrapolating track to reference surface at distance "
               << distance << " with direction " << options.direction
               << " with starting parameters " << parameters);

  auto propagateResult =
      propagator.template propagate<BoundTrackParameters, propagator_options_t,
                                    ForcedSurfaceReached>(
          parameters, referenceSurface, options);

  if (!propagateResult.ok()) {
    ACTS_ERROR("failed to extrapolate track: " << propagateResult.error());
    return propagateResult.error();
  }

  track.setReferenceSurface(referenceSurface.getSharedPtr());
  track.parameters() = propagateResult->endParameters.value().parameters();
  track.covariance() =
      propagateResult->endParameters.value().covariance().value();

  return Result<void>::success();
}

/// @brief Extrapolate tracks to a reference surface
///
/// @tparam track_container_t The track container type
/// @tparam propagator_t The propagator type
/// @tparam propagator_options_t The propagator options type
///
/// @param trackContainer The track container which is modified in-place
/// @param referenceSurface The reference surface
/// @param propagator The propagator
/// @param options The propagator options
/// @param strategy The extrapolation strategy
/// @param logger The logger
///
/// @return The result of the extrapolation
template <TrackContainerFrontend track_container_t, typename propagator_t,
          typename propagator_options_t>
Result<void> extrapolateTracksToReferenceSurface(
    const track_container_t &trackContainer, const Surface &referenceSurface,
    const propagator_t &propagator, propagator_options_t options,
    TrackExtrapolationStrategy strategy,
    const Logger &logger = *getDefaultLogger("TrackExtrapolation",
                                             Logging::INFO)) {
  Result<void> result = Result<void>::success();

  for (const auto &track : trackContainer) {
    auto extrapolateResult = extrapolateTrackToReferenceSurface(
        track, referenceSurface, propagator, options, strategy, logger);

    // Only keep the first error
    if (!extrapolateResult.ok() && result.ok()) {
      result = extrapolateResult.error();
    }
  }

  return result;
}

/// Helper function to calculate a number of track level quantities and store
/// them on the track itself
/// @tparam track_proxy_t The track proxy type
/// @param track A mutable track proxy to operate on
template <TrackProxyConcept track_proxy_t>
void calculateTrackQuantities(track_proxy_t track)
  requires(!track_proxy_t::ReadOnly)
{
  using ConstTrackStateProxy = typename track_proxy_t::ConstTrackStateProxy;

  track.chi2() = 0;
  track.nDoF() = 0;

  track.nHoles() = 0;
  track.nMeasurements() = 0;
  track.nSharedHits() = 0;
  track.nOutliers() = 0;

  for (ConstTrackStateProxy trackState : track.trackStatesReversed()) {
    ConstTrackStateType typeFlags = trackState.typeFlags();

    if (typeFlags.test(Acts::TrackStateFlag::HoleFlag)) {
      track.nHoles()++;
    } else if (typeFlags.test(Acts::TrackStateFlag::OutlierFlag)) {
      track.nOutliers()++;
    } else if (typeFlags.test(Acts::TrackStateFlag::MeasurementFlag)) {
      if (typeFlags.test(Acts::TrackStateFlag::SharedHitFlag)) {
        track.nSharedHits()++;
      }
      track.nMeasurements()++;
      track.chi2() += trackState.chi2();
      track.nDoF() += trackState.calibratedSize();
    }
  }
}

/// Helper function to trim track states from the front of a track
/// @tparam track_proxy_t the track proxy type
/// @param track the track to trim
/// @param trimHoles whether to trim holes
/// @param trimOutliers whether to trim outliers
/// @param trimMaterial whether to trim pure material states
/// @param trimOtherNoneMeasurement whether to trim other, non measurement, states
template <TrackProxyConcept track_proxy_t>
void trimTrackFront(track_proxy_t track, bool trimHoles, bool trimOutliers,
                    bool trimMaterial, bool trimOtherNoneMeasurement)
  requires(!track_proxy_t::ReadOnly)
{
  using TrackStateProxy = typename track_proxy_t::TrackStateProxy;

  // TODO specialize if track is forward linked

  std::optional<TrackStateProxy> front;

  for (TrackStateProxy trackState : track.trackStatesReversed()) {
    TrackStateType typeFlags = trackState.typeFlags();
    bool isHole = typeFlags.test(TrackStateFlag::HoleFlag);
    bool isOutlier = typeFlags.test(TrackStateFlag::OutlierFlag);
    bool isMaterial = typeFlags.test(TrackStateFlag::MaterialFlag) &&
                      !typeFlags.test(TrackStateFlag::MeasurementFlag);
    bool isOtherNoneMeasurement =
        !typeFlags.test(TrackStateFlag::MeasurementFlag) && !isHole &&
        !isOutlier && !isMaterial;
    if (trimHoles && isHole) {
      continue;
    }
    if (trimOutliers && isOutlier) {
      continue;
    }
    if (trimMaterial && isMaterial) {
      continue;
    }
    if (trimOtherNoneMeasurement && isOtherNoneMeasurement) {
      continue;
    }

    front = trackState;
  }

  if (front.has_value()) {
    front.value().previous() = TrackStateProxy::kInvalid;
  }
}

/// Helper function to trim track states from the back of a track
/// @tparam track_proxy_t the track proxy type
/// @param track the track to trim
/// @param trimHoles whether to trim holes
/// @param trimOutliers whether to trim outliers
/// @param trimMaterial whether to trim pure material states
/// @param trimOtherNoneMeasurement whether to trim other, non measurement, states
template <TrackProxyConcept track_proxy_t>
void trimTrackBack(track_proxy_t track, bool trimHoles, bool trimOutliers,
                   bool trimMaterial, bool trimOtherNoneMeasurement)
  requires(!track_proxy_t::ReadOnly)
{
  using TrackStateProxy = typename track_proxy_t::TrackStateProxy;

  std::optional<TrackStateProxy> back;

  for (TrackStateProxy trackState : track.trackStatesReversed()) {
    back = trackState;

    TrackStateType typeFlags = trackState.typeFlags();
    bool isHole = typeFlags.test(TrackStateFlag::HoleFlag);
    bool isOutlier = typeFlags.test(TrackStateFlag::OutlierFlag);
    bool isMaterial = typeFlags.test(TrackStateFlag::MaterialFlag) &&
                      !typeFlags.test(TrackStateFlag::MeasurementFlag);
    bool isOtherNoneMeasurement =
        !typeFlags.test(TrackStateFlag::MeasurementFlag) && !isHole &&
        !isOutlier && !isMaterial;
    if (trimHoles && isHole) {
      continue;
    }
    if (trimOutliers && isOutlier) {
      continue;
    }
    if (trimMaterial && isMaterial) {
      continue;
    }
    if (trimOtherNoneMeasurement && isOtherNoneMeasurement) {
      continue;
    }

    break;
  }

  if (back.has_value()) {
    track.tipIndex() = back.value().index();
  }
}

/// Helper function to trim track states from the front and back of a track
/// @tparam track_proxy_t the track proxy type
/// @param track the track to trim
/// @param trimHoles whether to trim holes
/// @param trimOutliers whether to trim outliers
/// @param trimMaterial whether to trim pure material states
/// @param trimOtherNoneMeasurement whether to trim other, non measurement, states
template <TrackProxyConcept track_proxy_t>
void trimTrack(track_proxy_t track, bool trimHoles, bool trimOutliers,
               bool trimMaterial, bool trimOtherNoneMeasurement)
  requires(!track_proxy_t::ReadOnly)
{
  trimTrackFront(track, trimHoles, trimOutliers, trimMaterial,
                 trimOtherNoneMeasurement);
  trimTrackBack(track, trimHoles, trimOutliers, trimMaterial,
                trimOtherNoneMeasurement);
}

/// Helper function to calculate the predicted residual and its covariance
/// @tparam nMeasurementDim the dimension of the measurement
/// @tparam track_state_proxy_t the track state proxy type
/// @param trackState the track state to calculate the residual from
/// @return a pair of the residual and its covariance
template <std::size_t nMeasurementDim,
          TrackStateProxyConcept track_state_proxy_t>
std::pair<ActsVector<nMeasurementDim>, ActsSquareMatrix<nMeasurementDim>>
calculatePredictedResidual(track_state_proxy_t trackState) {
  using MeasurementVector = ActsVector<nMeasurementDim>;
  using MeasurementMatrix = ActsSquareMatrix<nMeasurementDim>;

  if (!trackState.hasPredicted()) {
    throw std::invalid_argument("track state has no predicted parameters");
  }
  if (!trackState.hasCalibrated()) {
    throw std::invalid_argument("track state has no calibrated parameters");
  }

  auto subspaceHelper =
      trackState.template projectorSubspaceHelper<nMeasurementDim>();

  auto measurement = trackState.template calibrated<nMeasurementDim>();
  auto measurementCovariance =
      trackState.template calibratedCovariance<nMeasurementDim>();
  MeasurementVector predicted =
      subspaceHelper.projectVector(trackState.predicted());
  MeasurementMatrix predictedCovariance =
      subspaceHelper.projectMatrix(trackState.predictedCovariance());

  MeasurementVector residual = measurement - predicted;
  MeasurementMatrix residualCovariance =
      measurementCovariance + predictedCovariance;

  return {residual, residualCovariance};
}

/// Helper function to calculate the filtered residual and its covariance
/// @tparam nMeasurementDim the dimension of the measurement
/// @tparam track_state_proxy_t the track state proxy type
/// @param trackState the track state to calculate the residual from
/// @return a pair of the residual and its covariance
template <std::size_t nMeasurementDim,
          TrackStateProxyConcept track_state_proxy_t>
std::pair<ActsVector<nMeasurementDim>, ActsSquareMatrix<nMeasurementDim>>
calculateFilteredResidual(track_state_proxy_t trackState) {
  using MeasurementVector = ActsVector<nMeasurementDim>;
  using MeasurementMatrix = ActsSquareMatrix<nMeasurementDim>;

  if (!trackState.hasFiltered()) {
    throw std::invalid_argument("track state has no filtered parameters");
  }
  if (!trackState.hasCalibrated()) {
    throw std::invalid_argument("track state has no calibrated parameters");
  }

  auto subspaceHelper =
      trackState.template projectorSubspaceHelper<nMeasurementDim>();

  auto measurement = trackState.template calibrated<nMeasurementDim>();
  auto measurementCovariance =
      trackState.template calibratedCovariance<nMeasurementDim>();
  MeasurementVector filtered =
      subspaceHelper.projectVector(trackState.filtered());
  MeasurementMatrix filteredCovariance =
      subspaceHelper.projectMatrix(trackState.filteredCovariance());

  MeasurementVector residual = measurement - filtered;
  MeasurementMatrix residualCovariance =
      measurementCovariance + filteredCovariance;

  return {residual, residualCovariance};
}

/// Helper function to calculate the smoothed residual and its covariance
/// @tparam nMeasurementDim the dimension of the measurement
/// @tparam track_state_proxy_t the track state proxy type
/// @param trackState the track state to calculate the residual from
/// @return a pair of the residual and its covariance
template <std::size_t nMeasurementDim,
          TrackStateProxyConcept track_state_proxy_t>
std::pair<ActsVector<nMeasurementDim>, ActsSquareMatrix<nMeasurementDim>>
calculateSmoothedResidual(track_state_proxy_t trackState) {
  using MeasurementVector = ActsVector<nMeasurementDim>;
  using MeasurementMatrix = ActsSquareMatrix<nMeasurementDim>;

  if (!trackState.hasSmoothed()) {
    throw std::invalid_argument("track state has no smoothed parameters");
  }
  if (!trackState.hasCalibrated()) {
    throw std::invalid_argument("track state has no calibrated parameters");
  }

  auto subspaceHelper =
      trackState.template projectorSubspaceHelper<nMeasurementDim>();

  auto measurement = trackState.template calibrated<nMeasurementDim>();
  auto measurementCovariance =
      trackState.template calibratedCovariance<nMeasurementDim>();
  MeasurementVector smoothed =
      subspaceHelper.projectVector(trackState.smoothed());
  MeasurementMatrix smoothedCovariance =
      subspaceHelper.projectMatrix(trackState.smoothedCovariance());

  MeasurementVector residual = measurement - smoothed;
  MeasurementMatrix residualCovariance =
      measurementCovariance + smoothedCovariance;

  return {residual, residualCovariance};
}

/// Helper function to calculate the predicted chi2
/// @tparam track_state_proxy_t the track state proxy type
/// @param trackState the track state to calculate the chi2 from
/// @return the chi2
template <TrackStateProxyConcept track_state_proxy_t>
double calculatePredictedChi2(track_state_proxy_t trackState) {
  if (!trackState.hasPredicted()) {
    throw std::invalid_argument("track state has no predicted parameters");
  }
  if (!trackState.hasCalibrated()) {
    throw std::invalid_argument("track state has no calibrated parameters");
  }

  return visit_measurement(
      trackState.calibratedSize(),
      [&]<std::size_t measdim>(
          std::integral_constant<std::size_t, measdim>) -> double {
        auto [residual, residualCovariance] =
            calculatePredictedResidual<measdim>(trackState);

        return (residual.transpose() * residualCovariance.inverse() * residual)
            .eval()(0, 0);
      });
}

/// Helper function to calculate the filtered chi2
/// @tparam track_state_proxy_t the track state proxy type
/// @param trackState the track state to calculate the chi2 from
/// @return the chi2
template <TrackStateProxyConcept track_state_proxy_t>
double calculateFilteredChi2(track_state_proxy_t trackState) {
  if (!trackState.hasFiltered()) {
    throw std::invalid_argument("track state has no filtered parameters");
  }
  if (!trackState.hasCalibrated()) {
    throw std::invalid_argument("track state has no calibrated parameters");
  }

  return visit_measurement(
      trackState.calibratedSize(),
      [&]<std::size_t measdim>(
          std::integral_constant<std::size_t, measdim>) -> double {
        auto [residual, residualCovariance] =
            calculateFilteredResidual<measdim>(trackState);

        return (residual.transpose() * residualCovariance.inverse() * residual)
            .eval()(0, 0);
      });
}

/// Helper function to calculate the smoothed chi2
/// @tparam track_state_proxy_t the track state proxy type
/// @param trackState the track state to calculate the chi2 from
/// @return the chi2
template <TrackStateProxyConcept track_state_proxy_t>
double calculateSmoothedChi2(track_state_proxy_t trackState) {
  if (!trackState.hasSmoothed()) {
    throw std::invalid_argument("track state has no smoothed parameters");
  }
  if (!trackState.hasCalibrated()) {
    throw std::invalid_argument("track state has no calibrated parameters");
  }

  return visit_measurement(
      trackState.calibratedSize(),
      [&]<std::size_t measdim>(
          std::integral_constant<std::size_t, measdim>) -> double {
        auto [residual, residualCovariance] =
            calculateSmoothedResidual<measdim>(trackState);

        return (residual.transpose() * residualCovariance.inverse() * residual)
            .eval()(0, 0);
      });
}

/// Helper function to calculate the unbiased track parameters and their
/// covariance (i.e. fitted track parameters with this measurement removed)
/// using Eq.(12a)-Eq.(12c) of NIMA 262, 444 (1987)
/// @tparam track_state_proxy_t the track state proxy type
/// @param trackState the track state to calculate the unbiased parameters from
/// @return a pair of the unbiased parameters and their covariance
template <TrackStateProxyConcept track_state_proxy_t>
std::pair<BoundVector, BoundMatrix> calculateUnbiasedParametersCovariance(
    track_state_proxy_t trackState) {
  if (!trackState.hasSmoothed()) {
    throw std::invalid_argument("track state has no smoothed parameters");
  }
  if (!trackState.hasCalibrated()) {
    throw std::invalid_argument("track state has no calibrated parameters");
  }

  return visit_measurement(
      trackState.calibratedSize(),
      [&]<std::size_t measdim>(std::integral_constant<std::size_t, measdim>) {
        FixedBoundSubspaceHelper<measdim> subspaceHelper =
            trackState.template projectorSubspaceHelper<measdim>();

        // TODO use subspace helper for projection instead
        auto H = subspaceHelper.projector();
        auto s = trackState.smoothed();
        auto C = trackState.smoothedCovariance();
        auto m = trackState.template calibrated<measdim>();
        auto V = trackState.template calibratedCovariance<measdim>();
        auto K =
            (C * H.transpose() * (H * C * H.transpose() - V).inverse()).eval();
        BoundVector unbiasedParamsVec = s + K * (m - H * s);
        BoundMatrix unbiasedParamsCov = C - K * H * C;
        return std::make_pair(unbiasedParamsVec, unbiasedParamsCov);
      });
}

}  // namespace Acts

namespace std {
// register with STL
template <>
struct is_error_code_enum<Acts::TrackExtrapolationError> : std::true_type {};
}  // namespace std
