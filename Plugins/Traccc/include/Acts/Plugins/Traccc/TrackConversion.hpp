// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Plugin include(s)
#include "Acts/Plugins/Traccc/Detail/AlgebraConversion.hpp"

// Acts include(s)
#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/TrackProxy.hpp"
#include "Acts/EventData/detail/ParameterTraits.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/detail/Subspace.hpp"

// Detray include(s)
#include "detray/core/detector.hpp"
#include "detray/tracks/bound_track_parameters.hpp"

// Traccc include(s)
#include "traccc/edm/track_state.hpp"

// Boost include(s)
#include <boost/range/combine.hpp>

// System include(s)
#include <memory>
#include <stdexcept>
#include <variant>

namespace Acts::TracccPlugin {

/// @brief Creates a new Acts bound track parameters from detray bound track parameters.
/// @param dparams the detray bound track parameters.
/// @param detector the detray detector.
/// @param trackingGeometry the Acts tracking geometry.
/// @return An Acts BoundTrackParameters with data copied from a detray bound_track_parameters.
template <typename algebra_t, typename metadata_t, typename container_t>
inline auto newParams(const detray::bound_track_parameters<algebra_t>& dparams,
                      const detray::detector<metadata_t, container_t>& detector,
                      const Acts::TrackingGeometry& trackingGeometry) {
  constexpr std::size_t kFullSize =
      Acts::detail::kParametersSize<Acts::BoundIndices>;
  Acts::ActsVector<kFullSize> parameterVector =
      detail::toActsVector<kFullSize>(dparams.vector()[0]);
  typename Acts::BoundTrackParameters::CovarianceMatrix cov =
      detail::toActsSquareMatrix<kFullSize>(dparams.covariance());
  Acts::ParticleHypothesis particleHypothesis =
      Acts::ParticleHypothesis::pion();

  auto geoID =
      Acts::GeometryIdentifier(detector.surface(dparams.surface_link()).source);

  auto surface = trackingGeometry.findSurface(geoID);

  if (surface == nullptr) {
    throw std::runtime_error(
        "Mismatch between Acts geometry and detray detector: Acts tracking "
        "geometry does not contain geometry ID " +
        std::to_string(geoID.value()));
  }

  Acts::BoundTrackParameters params(surface->getSharedPtr(), parameterVector,
                                    std::make_optional(std::move(cov)),
                                    particleHypothesis);

  return params;
}

/// @brief Copies data from a traccc fitting result to an Acts track proxy.
/// @param source the traccc track fitting result to copy from.
/// @param destination the Acts track proxy to copy to.
/// @param detector the detray detector of the traccc track fitting result.
/// @param trackingGeometry the Acts tracking geometry.
template <typename algebra_t, typename track_container_t, typename trajectory_t,
          template <typename> class holder_t, typename metadata_t,
          typename container_t>
inline void copyFittingResult(
    const traccc::fitting_result<algebra_t>& source,
    Acts::TrackProxy<track_container_t, trajectory_t, holder_t, false>&
        destination,
    const detray::detector<metadata_t, container_t>& detector,
    const Acts::TrackingGeometry& trackingGeometry) {
  const auto params = newParams(source.fit_params, detector, trackingGeometry);
  destination.parameters() = params.parameters();
  destination.covariance() = params.covariance().value();
  destination.setReferenceSurface(params.referenceSurface().getSharedPtr());
}

/// @brief Copies data from a traccc track state to a Acts track state proxy.
/// @param source the traccc track state to copy from.
/// @param destination the Acts track state proxy to copy to.
/// @param detector the detray detector of the traccc track track state.
/// @param trackingGeometry the Acts tracking geometry.
/// @note Sets the uncalibrated source link and calibrated measurement to the traccc measurement.
template <typename algebra_t, typename metadata_t, typename container_t,
          typename trajectory_t, std::size_t M>
inline void copyTrackState(
    const traccc::track_state<algebra_t>& source,
    Acts::TrackStateProxy<trajectory_t, M, false>& destination,
    const detray::detector<metadata_t, container_t>& detector,
    const Acts::TrackingGeometry& trackingGeometry) {
  constexpr std::size_t kFullSize =
      Acts::detail::kParametersSize<Acts::BoundIndices>;
  constexpr std::size_t kSize = 2UL;

  auto geoID =
      Acts::GeometryIdentifier(detector.surface(source.surface_link()).source);
  auto surface = trackingGeometry.findSurface(geoID)->getSharedPtr();
  destination.setReferenceSurface(surface);

  using Parameters =
      typename Acts::TrackStateProxy<trajectory_t, M, false>::Parameters;
  using Covariance =
      typename Acts::TrackStateProxy<trajectory_t, M, false>::Covariance;

  destination.predicted() = Parameters(
      detail::toActsVector<kFullSize>(source.predicted().vector()[0]).data());
  destination.predictedCovariance() = Covariance(
      detail::toActsSquareMatrix<kFullSize>(source.predicted().covariance())
          .data());

  destination.smoothed() = Parameters(
      detail::toActsVector<kFullSize>(source.smoothed().vector()[0]).data());
  destination.smoothedCovariance() = Covariance(
      detail::toActsSquareMatrix<kFullSize>(source.smoothed().covariance())
          .data());

  destination.filtered() = Parameters(
      detail::toActsVector<kFullSize>(source.filtered().vector()[0]).data());
  destination.filteredCovariance() = Covariance(
      detail::toActsSquareMatrix<kFullSize>(source.filtered().covariance())
          .data());

  destination.jacobian() = Covariance(
      detail::toActsSquareMatrix<kFullSize>(source.jacobian()).data());

  destination.chi2() = source.smoothed_chi2();

  auto typeFlags = destination.typeFlags();
  typeFlags.set(TrackStateFlag::ParameterFlag);
  if (surface->surfaceMaterial() != nullptr) {
    typeFlags.set(TrackStateFlag::MaterialFlag);
  }
  if (source.is_hole) {
    typeFlags.set(TrackStateFlag::HoleFlag);
  }
  typeFlags.set(TrackStateFlag::MeasurementFlag);

  const traccc::measurement& measurement = source.get_measurement();

  destination.setUncalibratedSourceLink(Acts::SourceLink{measurement});

  destination.allocateCalibrated(kSize);

  destination.template calibrated<kSize>() =
      detail::toActsVector<kSize>(measurement.local);

  auto cov = Eigen::DiagonalMatrix<Acts::ActsScalar, kSize>(
                 detail::toActsVector<kSize>(measurement.variance))
                 .toDenseMatrix();
  destination.template calibratedCovariance<kSize>() = cov;

  Acts::detail::FixedSizeSubspace<kFullSize, kSize> subspace(
      measurement.subs.get_indices());
  destination.setProjector(subspace.template projector<Acts::ActsScalar>());
}

/// @brief Creates a new track in the Acts track container.
/// This new track will contain data copied from the traccc track container
/// element (track and track state data).
/// @param tracccTrack The traccc container element to copy from.
/// @param trackContainer The Acts track container. The new tracks will be added to this container.
/// @param detector The detray detector.
/// @param trackingGeometry The Acts tracking geometry.
/// @note Sets the uncalibrated source link and calibrated measurement to the traccc measurement.
template <typename fitting_result_t, typename track_state_vector_t,
          typename track_container_t, typename trajectory_t,
          template <typename> class holder_t, typename metadata_t,
          typename container_t>
inline auto makeTrack(
    const traccc::container_element<fitting_result_t, track_state_vector_t>&
        tracccTrack,
    Acts::TrackContainer<track_container_t, trajectory_t, holder_t>&
        trackContainer,
    const detray::detector<metadata_t, container_t>& detector,
    const Acts::TrackingGeometry& trackingGeometry) {
  auto fittingResult = tracccTrack.header;
  auto trackStates = tracccTrack.items;

  auto track = trackContainer.makeTrack();
  copyFittingResult(fittingResult, track, detector, trackingGeometry);

  // Make the track states.
  for (const auto& tstate : trackStates) {
    auto state = track.appendTrackState();
    copyTrackState(tstate, state, detector, trackingGeometry);
  }

  track.linkForward();

  return track;
}

/// @brief Creates a new track in the Acts track container for each track in the traccc track container.
/// The new tracks will contain data copied from the traccc track container
/// element (track and track state data).
/// @param tracccTrackContainer The traccc container containing the traccc tracks.
/// @param trackContainer The Acts track container. The new tracks will be added to this container.
/// @param detector The detray detector.
/// @param trackingGeometry The Acts tracking geometry.
/// @note Sets the uncalibrated source link and calibrated measurement to the traccc measurement.
template <typename traccc_track_container_t, typename track_container_t,
          typename trajectory_t, template <typename> class holder_t,
          typename metadata_t, typename container_t>
inline void makeTracks(
    const traccc_track_container_t& tracccTrackContainer,
    Acts::TrackContainer<track_container_t, trajectory_t, holder_t>&
        trackContainer,
    const detray::detector<metadata_t, container_t>& detector,
    const Acts::TrackingGeometry& trackingGeometry) {
  for (std::size_t i = 0; i < tracccTrackContainer.size(); i++) {
    makeTrack(tracccTrackContainer[i], trackContainer, detector,
              trackingGeometry);
  }
}

}  // namespace Acts::TracccPlugin
