// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/interface/IAssignmentFinder.hpp"
#include "Acts/Propagator/ActorList.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/SurfaceCollector.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <utility>
#include <vector>

namespace Acts {

/// @brief selector for finding surface
///
/// This is to be used with a SurfaceCollector<>
struct MaterialSurfaceIdentifier {
  /// check if the surface has material
  bool operator()(const Surface& sf) const {
    return (sf.surfaceMaterial() != nullptr);
  }
};

/// An Interaction volume collector with unique counting
struct InteractionVolumeCollector {
  /// Simple result struct that holds the collected volumes
  ///
  /// @note the map is to avoid double counting as this is
  /// called in an action list
  struct this_result {
    std::map<GeometryIdentifier, IAssignmentFinder::VolumeAssignment> collected;
  };

  using result_type = this_result;

  /// Collector action for the ActionList of the Propagator
  /// It checks if the propagator state has a current volume,
  /// in which case the action is performed:
  /// - it records the volume given the configuration
  ///
  /// @tparam propagator_state_t is the type of Propagator state
  /// @tparam stepper_t Type of the stepper used for the propagation
  /// @tparam navigator_t Type of the navigator used for the propagation
  ///
  /// @param [in,out] state is the mutable stepper state object
  /// @param [in] stepper The stepper in use
  /// @param [in] navigator The navigator in use
  /// @param [in,out] result is the mutable result object
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  void act(propagator_state_t& state, const stepper_t& stepper,
           const navigator_t& navigator, result_type& result,
           const Logger& /*logger*/) const {
    // Retrieve the current volume
    auto currentVolume = navigator.currentVolume(state.navigation);

    // The current volume has been assigned by the navigator
    if (currentVolume != nullptr) {
      auto collIt = result.collected.find(currentVolume->geometryId());
      // Check if the volume has been collected and if it has material
      if (collIt == result.collected.end() &&
          currentVolume->volumeMaterial() != nullptr) {
        Vector3 entryPosition = stepper.position(state.stepping);
        Vector3 exitPosition = entryPosition;
        IAssignmentFinder::VolumeAssignment vAssignment{
            InteractionVolume(currentVolume), entryPosition, exitPosition};
        result.collected.emplace(currentVolume->geometryId(), vAssignment);
      } else if (collIt != result.collected.end()) {
        // Update the exit position
        (collIt->second).exit = stepper.position(state.stepping);
      }
    }
  }
};

/// @class PropagatorMaterialAssigner
///
/// A Propagator based material assigner that uses the navigation and
/// transport of the propagator to assign the material to the surface
/// or the volume.
///
/// @note eventual navigation problems would affect he material mapping
template <typename propagator_t>
class PropagatorMaterialAssigner final : public IAssignmentFinder {
 public:
  /// @brief  Construct with propagator
  /// @param propagator
  PropagatorMaterialAssigner(propagator_t propagator)
      : m_propagator(std::move(propagator)) {}

  /// @brief Method for generating assignment candidates for the
  /// material interaction assignment to surfaces or volumes
  ///
  /// @param gctx is the geometry context
  /// @param mctx is the magnetic field context
  /// @param position is the position of the initial ray
  /// @param direction is the direction of initial ray
  ///
  /// @return a vector of Surface Assignments and Volume Assignments
  std::pair<std::vector<IAssignmentFinder::SurfaceAssignment>,
            std::vector<IAssignmentFinder::VolumeAssignment>>
  assignmentCandidates(const GeometryContext& gctx,
                       const MagneticFieldContext& mctx,
                       const Vector3& position,
                       const Vector3& direction) const final {
    // Return container
    std::pair<std::vector<IAssignmentFinder::SurfaceAssignment>,
              std::vector<IAssignmentFinder::VolumeAssignment>>
        candidates;

    using VectorHelpers::makeVector4;
    // Neutral curvilinear parameters
    NeutralCurvilinearTrackParameters start(
        makeVector4(position, 0), direction, 1, std::nullopt,
        NeutralParticleHypothesis::geantino());

    // Prepare Action list and abort list
    using MaterialSurfaceCollector =
        SurfaceCollector<MaterialSurfaceIdentifier>;
    using ActorList = ActorList<MaterialSurfaceCollector,
                                InteractionVolumeCollector, EndOfWorldReached>;
    using PropagatorOptions =
        typename propagator_t::template Options<ActorList>;

    PropagatorOptions options(gctx, mctx);

    const auto& result = m_propagator.propagate(start, options).value();

    // The surface collection results
    auto scResult =
        result.template get<MaterialSurfaceCollector::result_type>();
    auto mappingSurfaces = scResult.collected;

    for (auto& mSurface : mappingSurfaces) {
      candidates.first.push_back(IAssignmentFinder::SurfaceAssignment{
          mSurface.surface, mSurface.position, direction});
    }

    // The volume collection results
    auto vcResult =
        result.template get<InteractionVolumeCollector::result_type>();
    for (const auto& [geoId, vIntersection] : vcResult.collected) {
      candidates.second.push_back(vIntersection);
    }

    // Return the candidates
    return candidates;
  }

 private:
  propagator_t m_propagator;
};

}  // namespace Acts
