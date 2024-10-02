// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/detail/TestSourceLink.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/ActorList.hpp"
#include "Acts/Propagator/StandardAborters.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <random>
#include <vector>

namespace Acts::Test {

/// All supported simulated measurement types.
enum class MeasurementType {
  eLoc0,
  eLoc1,
  eLoc01,
};

/// Measurement resolution configuration for a single detector type.
struct MeasurementResolution {
  MeasurementType type = MeasurementType::eLoc0;
  // Depending on the type, only the first value is used.
  std::array<double, 2> stddev = {50 * UnitConstants::um, 0};
};

/// Measurement resolution configuration for a full detector geometry.
using MeasurementResolutionMap =
    Acts::GeometryHierarchyMap<MeasurementResolution>;

/// Result struct for generated measurements and outliers.
struct Measurements {
  std::vector<Acts::detail::Test::TestSourceLink> sourceLinks;
  std::vector<Acts::detail::Test::TestSourceLink> outlierSourceLinks;
  std::vector<BoundVector> truthParameters;
};

/// Propagator action to create smeared measurements.
struct MeasurementsCreator {
  using result_type = Measurements;

  MeasurementResolutionMap resolutions;
  std::default_random_engine* rng = nullptr;
  std::size_t sourceId = 0;
  // how far away from the measurements the outliers should be
  double distanceOutlier = 10 * Acts::UnitConstants::mm;

  /// @brief Operator that is callable by an ActorList. The function
  /// collects the surfaces
  ///
  /// @tparam propagator_state_t Type of the propagator state
  /// @tparam stepper_t Type of the stepper
  /// @tparam navigator_t Type of the navigator
  ///
  /// @param [out] result Vector of matching surfaces
  /// @param [in] state State of the propagator
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  void act(propagator_state_t& state, const stepper_t& stepper,
           const navigator_t& navigator, result_type& result,
           const Logger& logger) const {
    using namespace Acts::UnitLiterals;

    // only generate measurements on surfaces
    if (!navigator.currentSurface(state.navigation)) {
      return;
    }
    const Acts::Surface& surface = *navigator.currentSurface(state.navigation);
    const Acts::GeometryIdentifier geoId = surface.geometryId();
    // only generate measurements on sensitive surface
    if (!geoId.sensitive()) {
      ACTS_VERBOSE("Create no measurements on non-sensitive surface " << geoId);
      return;
    }
    // only generate measurements if a resolution is configured
    auto found = resolutions.find(geoId);
    if (found == resolutions.end()) {
      ACTS_VERBOSE("No resolution configured for sensitive surface " << geoId);
      return;
    }
    const MeasurementResolution& resolution = *found;

    // Apply global to local
    Acts::Vector2 loc =
        surface
            .globalToLocal(state.geoContext, stepper.position(state.stepping),
                           stepper.direction(state.stepping))
            .value();
    // The truth info
    BoundVector parameters = BoundVector::Zero();
    parameters[eBoundLoc0] = loc[eBoundLoc0];
    parameters[eBoundLoc1] = loc[eBoundLoc1];
    const auto& direction = stepper.position(state.stepping);
    parameters[eBoundPhi] = VectorHelpers::phi(direction);
    parameters[eBoundTheta] = VectorHelpers::theta(direction);
    parameters[eBoundQOverP] = state.stepping.pars[eFreeQOverP];
    parameters[eBoundTime] = state.stepping.pars[eFreeTime];
    result.truthParameters.push_back(std::move(parameters));

    std::normal_distribution<double> normalDist(0., 1.);

    // compute covariance for all components, might contain bogus values
    // depending on the configuration. but those remain unused.
    Vector2 stddev(resolution.stddev[0], resolution.stddev[1]);
    SquareMatrix2 cov = stddev.cwiseProduct(stddev).asDiagonal();

    if (resolution.type == MeasurementType::eLoc0) {
      double val = loc[0] + stddev[0] * normalDist(*rng);
      double out = val + distanceOutlier;
      result.sourceLinks.emplace_back(eBoundLoc0, val, cov(0, 0), geoId,
                                      sourceId);
      result.outlierSourceLinks.emplace_back(eBoundLoc0, out, cov(0, 0), geoId,
                                             sourceId);
    } else if (resolution.type == MeasurementType::eLoc1) {
      // yes, using stddev[0] and cov(0,0) is correct here. this accesses the
      // first configuration parameter not the first local coordinate.
      double val = loc[1] + stddev[0] * normalDist(*rng);
      double out = val + distanceOutlier;
      result.sourceLinks.emplace_back(eBoundLoc1, val, cov(0, 0), geoId,
                                      sourceId);
      result.outlierSourceLinks.emplace_back(eBoundLoc1, out, cov(0, 0), geoId,
                                             sourceId);
    } else if (resolution.type == MeasurementType::eLoc01) {
      Vector2 val = loc + stddev.cwiseProduct(
                              Vector2(normalDist(*rng), normalDist(*rng)));
      Vector2 out = val + Vector2(distanceOutlier, -distanceOutlier);
      result.sourceLinks.emplace_back(eBoundLoc0, eBoundLoc1, val, cov, geoId,
                                      sourceId);
      result.outlierSourceLinks.emplace_back(eBoundLoc0, eBoundLoc1, out, cov,
                                             geoId, sourceId);
    }
  }
};

/// Propagate the track create smeared measurements from local coordinates.
template <typename propagator_t, typename track_parameters_t>
Measurements createMeasurements(const propagator_t& propagator,
                                const Acts::GeometryContext& geoCtx,
                                const Acts::MagneticFieldContext& magCtx,
                                const track_parameters_t& trackParameters,
                                const MeasurementResolutionMap& resolutions,
                                std::default_random_engine& rng,
                                std::size_t sourceId = 0u) {
  using ActorList =
      Acts::ActorList<MeasurementsCreator, Acts::EndOfWorldReached>;
  using PropagatorOptions = typename propagator_t::template Options<ActorList>;

  // Set options for propagator
  PropagatorOptions options(geoCtx, magCtx);
  auto& creator = options.actorList.template get<MeasurementsCreator>();
  creator.resolutions = resolutions;
  creator.rng = &rng;
  creator.sourceId = sourceId;

  // Launch and collect the measurements
  auto result = propagator.propagate(trackParameters, options).value();
  return std::move(result.template get<Measurements>());
}

}  // namespace Acts::Test
