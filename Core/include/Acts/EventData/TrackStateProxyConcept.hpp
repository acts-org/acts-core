// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/EventData/TrackStateType.hpp"
#include "Acts/EventData/Types.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/HashedString.hpp"

#include <utility>

namespace Acts {

namespace detail {
using Parameters = Eigen::Map<BoundVector>;
using Covariance = Eigen::Map<BoundMatrix>;

using ConstParameters = Eigen::Map<const BoundVector>;
using ConstCovariance = Eigen::Map<const BoundMatrix>;

using Measurement = Eigen::Map<ActsVector<2>>;
using MeasurementCovariance = Eigen::Map<ActsSquareMatrix<2>>;

using ConstMeasurement = Eigen::Map<const ActsVector<2>>;
using ConstMeasurementCovariance = Eigen::Map<const ActsSquareMatrix<2>>;

using DynamicMeasurement =
    Eigen::Map<Eigen::Matrix<Covariance::Scalar, Eigen::Dynamic, 1,
                             Eigen::ColMajor | Eigen::AutoAlign>>;
using DynamicMeasurementCovariance =
    Eigen::Map<Eigen::Matrix<Covariance::Scalar, Eigen::Dynamic, Eigen::Dynamic,
                             Eigen::ColMajor | Eigen::AutoAlign>>;

using ConstDynamicMeasurement =
    Eigen::Map<const Eigen::Matrix<Covariance::Scalar, Eigen::Dynamic, 1,
                                   Eigen::ColMajor | Eigen::AutoAlign>>;
using ConstDynamicMeasurementCovariance = Eigen::Map<
    const Eigen::Matrix<Covariance::Scalar, Eigen::Dynamic, Eigen::Dynamic,
                        Eigen::ColMajor | Eigen::AutoAlign>>;

constexpr static auto ProjectorFlags = Eigen::RowMajor | Eigen::AutoAlign;
using Projector = Eigen::Matrix<typename Covariance::Scalar, eBoundSize,
                                eBoundSize, ProjectorFlags>;
using EffectiveProjector =
    Eigen::Matrix<typename Projector::Scalar, Eigen::Dynamic, eBoundSize,
                  ProjectorFlags, eBoundSize, eBoundSize>;

}  // namespace detail

template <typename T>
concept TrackStateProxyConcept =
    requires(const T& cv, T v, HashedString key,
             std::shared_ptr<const Surface> surface) {
      { cv.index() } -> std::same_as<TrackIndexType>;

      { cv.previous() } -> std::same_as<TrackIndexType>;

      { cv.hasPrevious() } -> std::same_as<bool>;

      { cv.getMask() } -> std::same_as<TrackStatePropMask>;

      { cv.referenceSurface() } -> std::same_as<const Surface&>;

      { cv.hasReferenceSurface() } -> std::same_as<bool>;

      { cv.template has<hashString("blubb")>() } -> std::same_as<bool>;

      { cv.has(key) } -> std::same_as<bool>;

      { cv.has("blubb") } -> std::same_as<bool>;

      // Cannot verify for all types, so just check int
      {
        cv.template component<int, hashString("blubb")>()
      } -> std::same_as<const int&>;

      { cv.template component<int>(key) } -> std::same_as<const int&>;

      { cv.parameters() } -> std::same_as<detail::ConstParameters>;
      { cv.covariance() } -> std::same_as<detail::ConstCovariance>;

      { cv.predicted() } -> std::same_as<detail::ConstParameters>;
      { cv.predictedCovariance() } -> std::same_as<detail::ConstCovariance>;
      { cv.hasPredicted() } -> std::same_as<bool>;
      { v.hasPredicted() } -> std::same_as<bool>;

      { cv.filtered() } -> std::same_as<detail::ConstParameters>;
      { cv.filteredCovariance() } -> std::same_as<detail::ConstCovariance>;
      { cv.hasFiltered() } -> std::same_as<bool>;
      { v.hasFiltered() } -> std::same_as<bool>;

      { cv.smoothed() } -> std::same_as<detail::ConstParameters>;
      { cv.smoothedCovariance() } -> std::same_as<detail::ConstCovariance>;
      { cv.hasSmoothed() } -> std::same_as<bool>;
      { v.hasSmoothed() } -> std::same_as<bool>;

      { cv.jacobian() } -> std::same_as<detail::ConstCovariance>;
      { cv.hasJacobian() } -> std::same_as<bool>;
      { v.hasJacobian() } -> std::same_as<bool>;

      { cv.hasProjector() } -> std::same_as<bool>;
      { v.hasProjector() } -> std::same_as<bool>;

      { cv.effectiveProjector() } -> std::same_as<detail::EffectiveProjector>;
      { v.effectiveProjector() } -> std::same_as<detail::EffectiveProjector>;

      { cv.projectorBitset() } -> std::same_as<ProjectorBitset>;
      { v.projectorBitset() } -> std::same_as<ProjectorBitset>;

      { cv.getUncalibratedSourceLink() } -> std::same_as<SourceLink>;
      { v.getUncalibratedSourceLink() } -> std::same_as<SourceLink>;

      { cv.hasCalibrated() } -> std::same_as<bool>;
      { v.hasCalibrated() } -> std::same_as<bool>;

      { cv.template calibrated<2>() } -> std::same_as<detail::ConstMeasurement>;
      {
        cv.template calibratedCovariance<2>()
      } -> std::same_as<detail::ConstMeasurementCovariance>;

      {
        cv.effectiveCalibrated()
      } -> std::same_as<detail::ConstDynamicMeasurement>;
      {
        cv.effectiveCalibratedCovariance()
      } -> std::same_as<detail::ConstDynamicMeasurementCovariance>;

      { cv.calibratedSize() } -> std::same_as<TrackIndexType>;
      { v.calibratedSize() } -> std::same_as<TrackIndexType>;

      { cv.chi2() } -> std::same_as<float>;

      { cv.pathLength() } -> std::same_as<double>;

      { cv.typeFlags() } -> std::same_as<ConstTrackStateType>;
    };

template <typename T>
concept ConstTrackStateProxyConcept =
    TrackStateProxyConcept<T> && requires(T v, HashedString key) {
      // Cannot verify for all types, so just check int
      {
        v.template component<int, hashString("blubb")>()
      } -> std::same_as<const int&>;

      { v.template component<int>(key) } -> std::same_as<const int&>;

      { v.predicted() } -> std::same_as<detail::ConstParameters>;
      { v.predictedCovariance() } -> std::same_as<detail::ConstCovariance>;

      { v.filtered() } -> std::same_as<detail::ConstParameters>;
      { v.filteredCovariance() } -> std::same_as<detail::ConstCovariance>;

      { v.smoothed() } -> std::same_as<detail::ConstParameters>;
      { v.smoothedCovariance() } -> std::same_as<detail::ConstCovariance>;

      { v.jacobian() } -> std::same_as<detail::ConstCovariance>;

      { v.template calibrated<2>() } -> std::same_as<detail::ConstMeasurement>;
      {
        v.template calibratedCovariance<2>()
      } -> std::same_as<detail::ConstMeasurementCovariance>;

      {
        v.effectiveCalibrated()
      } -> std::same_as<detail::ConstDynamicMeasurement>;
      {
        v.effectiveCalibratedCovariance()
      } -> std::same_as<detail::ConstDynamicMeasurementCovariance>;

      { v.chi2() } -> std::same_as<double>;

      { v.pathLength() } -> std::same_as<double>;

      { v.typeFlags() } -> std::same_as<ConstTrackStateType>;
    };

template <typename T>
concept MutableTrackStateProxyConcept =
    TrackStateProxyConcept<T> &&
    requires(T v, HashedString key, TrackStatePropMask mask,
             TrackIndexType index, std::shared_ptr<const Surface> surface,
             Eigen::Matrix<double, 3, 6> projector,
             ProjectorBitset projectorBitset, SourceLink sl,
             std::size_t measdim) {
      { v.shareFrom(mask, mask) };

      {
        v.shareFrom(
            std::declval<typename T::Trajectory::ConstTrackStateProxy>(), mask)
      };

      { v.shareFrom(std::declval<T>(), mask) };

      // Cannot verify copyFrom compatibility with other backend proxies
      {
        v.copyFrom(std::declval<typename T::Trajectory::ConstTrackStateProxy>(),
                   mask)
      };

      { v.copyFrom(std::declval<T>(), mask) };

      { v.unset(mask) };

      // Cannot verify for all types, so just check int
      {
        v.template component<int, hashString("blubb")>()
      } -> std::same_as<int&>;

      { v.template component<int>(key) } -> std::same_as<int&>;

      { v.predicted() } -> std::same_as<detail::Parameters>;
      { v.predictedCovariance() } -> std::same_as<detail::Covariance>;

      { v.filtered() } -> std::same_as<detail::Parameters>;
      { v.filteredCovariance() } -> std::same_as<detail::Covariance>;

      { v.smoothed() } -> std::same_as<detail::Parameters>;
      { v.smoothedCovariance() } -> std::same_as<detail::Covariance>;

      { v.jacobian() } -> std::same_as<detail::Covariance>;

      { v.setProjector(projector) };

      { v.setProjectorBitset(projectorBitset) };

      { v.setUncalibratedSourceLink(sl) };

      { v.template calibrated<2>() } -> std::same_as<detail::Measurement>;
      {
        v.template calibratedCovariance<2>()
      } -> std::same_as<detail::MeasurementCovariance>;

      { v.effectiveCalibrated() } -> std::same_as<detail::DynamicMeasurement>;
      {
        v.effectiveCalibratedCovariance()
      } -> std::same_as<detail::DynamicMeasurementCovariance>;

      { v.allocateCalibrated(measdim) };

      { v.chi2() } -> std::same_as<double&>;

      { v.pathLength() } -> std::same_as<double&>;

      { v.typeFlags() } -> std::same_as<TrackStateType>;
    };
}  // namespace Acts
