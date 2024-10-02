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
#include "Acts/EventData/Types.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/HashedString.hpp"

#include <any>

namespace Acts {

namespace detail {

using Parameters = Eigen::Map<BoundVector>;
using Covariance = Eigen::Map<BoundMatrix>;

using ConstParameters = Eigen::Map<const BoundVector>;
using ConstCovariance = Eigen::Map<const BoundMatrix>;

template <typename R>
concept RangeLike = requires(R r) {
  { r.begin() };
  { r.end() };

  requires std::forward_iterator<decltype(r.begin())>;
  requires std::forward_iterator<decltype(r.end())>;
};

}  // namespace detail

template <typename T>
concept CommonMultiTrajectoryBackend =
    requires(const T& cv, HashedString key, TrackIndexType istate) {
      { cv.calibratedSize_impl(istate) } -> std::same_as<TrackIndexType>;

      { cv.getUncalibratedSourceLink_impl(istate) } -> std::same_as<SourceLink>;

      { cv.referenceSurface_impl(istate) } -> std::same_as<const Surface*>;

      { cv.parameters_impl(istate) } -> std::same_as<detail::ConstParameters>;

      { cv.covariance_impl(istate) } -> std::same_as<detail::ConstCovariance>;

      { cv.jacobian_impl(istate) } -> std::same_as<detail::ConstCovariance>;

      {
        cv.template calibrated_impl<2>(istate)
      } -> std::same_as<Eigen::Map<const ActsVector<2>>>;

      {
        cv.template calibratedCovariance_impl<2>(istate)
      } -> std::same_as<Eigen::Map<const ActsSquareMatrix<2>>>;

      { cv.has_impl(key, istate) } -> std::same_as<bool>;

      { cv.size_impl() } -> std::same_as<TrackIndexType>;

      { cv.component_impl(key, istate) } -> std::same_as<std::any>;

      { cv.hasColumn_impl(key) } -> std::same_as<bool>;

      { cv.dynamicKeys_impl() };
      requires detail::RangeLike<decltype(cv.dynamicKeys_impl())>;
    };

template <typename T>
concept ConstMultiTrajectoryBackend =
    CommonMultiTrajectoryBackend<T> &&
    requires(T v, HashedString key, TrackIndexType istate) {
      { v.parameters_impl(istate) } -> std::same_as<detail::ConstParameters>;

      { v.covariance_impl(istate) } -> std::same_as<detail::ConstCovariance>;

      { v.jacobian_impl(istate) } -> std::same_as<detail::ConstCovariance>;

      {
        v.template calibrated_impl<2>(istate)
      } -> std::same_as<Eigen::Map<const ActsVector<2>>>;

      {
        v.template calibratedCovariance_impl<2>(istate)
      } -> std::same_as<Eigen::Map<const ActsSquareMatrix<2>>>;
    };

template <typename T>
concept MutableMultiTrajectoryBackend =
    CommonMultiTrajectoryBackend<T> &&
    requires(T v, HashedString key, TrackIndexType istate,
             TrackStatePropMask mask, std::string col, std::size_t dim,
             SourceLink sl, std::shared_ptr<const Surface> surface) {
      { v.parameters_impl(istate) } -> std::same_as<detail::Parameters>;

      { v.covariance_impl(istate) } -> std::same_as<detail::Covariance>;

      { v.jacobian_impl(istate) } -> std::same_as<detail::Covariance>;

      {
        v.template calibrated_impl<2>(istate)
      } -> std::same_as<Eigen::Map<ActsVector<2>>>;

      {
        v.template calibratedCovariance_impl<2>(istate)
      } -> std::same_as<Eigen::Map<ActsSquareMatrix<2>>>;

      { v.addTrackState_impl() } -> std::same_as<TrackIndexType>;

      { v.shareFrom_impl(istate, istate, mask, mask) };

      { v.unset_impl(mask, istate) };

      { v.clear_impl() };

      // As far as I know there's no good way to assert that there's a generic
      // template function
      { v.template addColumn_impl<std::uint32_t>(col) };
      { v.template addColumn_impl<std::uint64_t>(col) };
      { v.template addColumn_impl<std::int32_t>(col) };
      { v.template addColumn_impl<std::int64_t>(col) };
      { v.template addColumn_impl<float>(col) };
      { v.template addColumn_impl<double>(col) };

      { v.allocateCalibrated_impl(istate, dim) };

      { v.setUncalibratedSourceLink_impl(istate, std::move(sl)) };

      { v.setReferenceSurface_impl(istate, surface) };

      { v.copyDynamicFrom_impl(istate, key, std::declval<const std::any&>()) };
    };

}  // namespace Acts
