// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectoryBackendConcept.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
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
}  // namespace detail

template <typename T>
concept ConstTrackContainerBackend =
    requires(const T& cv, HashedString key, TrackIndexType itrack) {
      { cv.size_impl() } -> std::same_as<std::size_t>;

      { cv.component_impl(key, itrack) } -> std::same_as<std::any>;

      { cv.parameters(itrack) } -> std::same_as<detail::ConstParameters>;

      { cv.covariance(itrack) } -> std::same_as<detail::ConstCovariance>;

      { cv.hasColumn_impl(key) } -> std::same_as<bool>;

      { cv.referenceSurface_impl(itrack) } -> std::same_as<const Surface*>;

      {
        cv.particleHypothesis_impl(itrack)
      } -> std::same_as<ParticleHypothesis>;

      { cv.dynamicKeys_impl() };
      requires detail::RangeLike<decltype(cv.dynamicKeys_impl())>;
    };

template <typename T>
concept MutableTrackContainerBackend =
    ConstTrackContainerBackend<T> &&
    requires(T v, HashedString key, TrackIndexType itrack, std::string col,
             const T& other, std::shared_ptr<const Surface> sharedSurface) {
      { v.parameters(itrack) } -> std::same_as<detail::Parameters>;

      { v.covariance(itrack) } -> std::same_as<detail::Covariance>;

      { v.addTrack_impl() } -> std::same_as<TrackIndexType>;

      { v.removeTrack_impl(itrack) };

      // As far as I know there's no good way to assert that there's a
      // generic template function
      { v.template addColumn_impl<std::uint32_t>(col) };
      { v.template addColumn_impl<std::uint64_t>(col) };
      { v.template addColumn_impl<std::int32_t>(col) };
      { v.template addColumn_impl<std::int64_t>(col) };
      { v.template addColumn_impl<float>(col) };
      { v.template addColumn_impl<double>(col) };

      { v.copyDynamicFrom_impl(itrack, key, std::declval<const std::any&>()) };

      { v.ensureDynamicColumns_impl(other) };

      { v.reserve(itrack) };

      { v.setReferenceSurface_impl(itrack, sharedSurface) };

      {
        v.setParticleHypothesis_impl(
            itrack, std::declval<const Acts::ParticleHypothesis&>())
      };

      { v.clear() };
    };

template <typename T>
struct IsReadOnlyTrackContainer;

template <typename T>
concept TrackContainerBackend =
    ConstTrackContainerBackend<T> &&
    (IsReadOnlyTrackContainer<T>::value || MutableTrackContainerBackend<T>);
}  // namespace Acts
