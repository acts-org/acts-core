// This file is part of the Acts project.
//
// Copyright (C) 2016-2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Propagator/ConstrainedStep.hpp"
#include "Acts/Propagator/NavigatorOptions.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/StringHelpers.hpp"

#include <sstream>
#include <string>

#include <boost/container/small_vector.hpp>

namespace Acts {

/// @brief struct for the Navigation options that are forwarded to
///        the geometry
///
/// @tparam object_t Type of the object for navigation to check against
template <typename object_t>
struct NavigationOptions {
  /// The boundary check directive
  BoundaryTolerance boundaryTolerance = BoundaryTolerance::None();

  // How to resolve the geometry
  /// Always look for sensitive
  bool resolveSensitive = true;
  /// Always look for material
  bool resolveMaterial = true;
  /// always look for passive
  bool resolvePassive = false;

  /// Hint for start object
  const object_t* startObject = nullptr;
  /// Hint for end object
  const object_t* endObject = nullptr;

  /// External surface identifier for which the boundary check is ignored
  std::vector<GeometryIdentifier> externalSurfaces = {};

  /// The minimum distance for a surface to be considered
  double nearLimit = 0;
  /// The maximum distance for a surface to be considered
  double farLimit = std::numeric_limits<double>::max();

  /// Force intersection with boundaries
  bool forceIntersectBoundaries = false;
};

/// @brief Steers the propagation through the geometry by adjusting the step
///        size and providing the next surface to be targeted.
///
/// The Navigator is part of the propagation and responsible for steering
/// the step size in order to encounter all the relevant surfaces which are
/// intersected by the trajectory.
///
/// The current navigation stage is cached in the state struct and updated
/// when necessary. If any surface in the extrapolation flow is hit, it is
/// set to the navigation state, such that other actors can deal with it.
///
/// The current target surface is referenced by an index which points into
/// the navigation candidates. The navigation candidates are ordered by the
/// path length to the surface. If a surface is hit, the
/// `state.navigation.currentSurface` pointer is set. This actors to observe
/// that we are on a surface.
///
class Navigator {
 public:
  using Surfaces = std::vector<const Surface*>;

  using NavigationSurfaces =
      boost::container::small_vector<SurfaceIntersection, 10>;

  using NavigationLayers =
      boost::container::small_vector<LayerIntersection, 10>;

  using NavigationBoundaries =
      boost::container::small_vector<BoundaryIntersection, 4>;

  using ExternalSurfaces = std::multimap<std::uint64_t, GeometryIdentifier>;

  /// The navigation stage
  enum struct Stage : int {
    undefined = 0,
    surfaceTarget = 1,
    layerTarget = 2,
    boundaryTarget = 3
  };

  struct Config {
    /// Tracking Geometry for this Navigator
    std::shared_ptr<const TrackingGeometry> trackingGeometry{nullptr};

    /// stop at every sensitive surface (whether it has material or not)
    bool resolveSensitive = true;
    /// stop at every material surface (whether it is passive or not)
    bool resolveMaterial = true;
    /// stop at every surface regardless what it is
    bool resolvePassive = false;
  };

  struct Options : public NavigatorPlainOptions {
    /// Externally provided surfaces - these are tried to be hit
    ExternalSurfaces externalSurfaces = {};

    void insertExternalSurface(GeometryIdentifier geoid) {
      externalSurfaces.insert(
          std::pair<std::uint64_t, GeometryIdentifier>(geoid.layer(), geoid));
    }

    void setPlainOptions(const NavigatorPlainOptions& options) {
      static_cast<NavigatorPlainOptions&>(*this) = options;
    }
  };

  /// @brief Nested State struct
  ///
  /// It acts as an internal state which is created for every propagation and
  /// meant to keep thread-local navigation information.
  struct State {
    Options options;

    // Navigation on surface level
    /// the vector of navigation surfaces to work through
    NavigationSurfaces navSurfaces = {};
    /// the current surface index of the navigation state
    std::size_t navSurfaceIndex = navSurfaces.size();

    // Navigation on layer level
    /// the vector of navigation layers to work through
    NavigationLayers navLayers = {};
    /// the current layer index of the navigation state
    std::size_t navLayerIndex = navLayers.size();

    // Navigation on volume level
    /// the vector of boundary surfaces to work through
    NavigationBoundaries navBoundaries = {};
    /// the current boundary index of the navigation state
    std::size_t navBoundaryIndex = navBoundaries.size();

    auto navSurface() const { return navSurfaces.at(navSurfaceIndex); }
    auto navLayer() const { return navLayers.at(navLayerIndex); }
    auto navBoundary() const { return navBoundaries.at(navBoundaryIndex); }

    /// Navigation state: the world volume
    const TrackingVolume* worldVolume = nullptr;

    /// Navigation state: the start volume
    const TrackingVolume* startVolume = nullptr;
    /// Navigation state: the start layer
    const Layer* startLayer = nullptr;
    /// Navigation state: the start surface
    const Surface* startSurface = nullptr;
    /// Navigation state: the current volume
    const TrackingVolume* currentVolume = nullptr;
    /// Navigation state: the current layer
    const Layer* currentLayer = nullptr;
    /// Navigation state - external state: the current surface
    const Surface* currentSurface = nullptr;
    /// Navigation state: the target volume
    const TrackingVolume* targetVolume = nullptr;
    /// Navigation state: the target layer
    const Layer* targetLayer = nullptr;
    /// Navigation state: the target surface
    const Surface* targetSurface = nullptr;

    /// Indicator if the target is reached
    bool targetReached = false;
    /// Navigation state : a break has been detected
    bool navigationBreak = false;
    // The navigation stage (@todo: integrate break, target)
    Stage navigationStage = Stage::undefined;
    /// Force intersection with boundaries
    bool forceIntersectBoundaries = false;
  };

  /// Constructor with configuration object
  ///
  /// @param cfg The navigator configuration
  /// @param _logger a logger instance
  explicit Navigator(Config cfg,
                     std::shared_ptr<const Logger> _logger =
                         getDefaultLogger("Navigator", Logging::Level::INFO))
      : m_cfg{std::move(cfg)}, m_logger{std::move(_logger)} {}

  State makeState(const Options& options) const {
    assert(options.startSurface != nullptr && "Start surface must be set");

    State state;
    state.options = options;
    state.startSurface = options.startSurface;
    state.targetSurface = options.targetSurface;
    return state;
  }

  const Surface* currentSurface(const State& state) const {
    return state.currentSurface;
  }

  const TrackingVolume* currentVolume(const State& state) const {
    return state.currentVolume;
  }

  const IVolumeMaterial* currentVolumeMaterial(const State& state) const {
    if (state.currentVolume == nullptr) {
      return nullptr;
    }
    return state.currentVolume->volumeMaterial();
  }

  const Surface* startSurface(const State& state) const {
    return state.startSurface;
  }

  const Surface* targetSurface(const State& state) const {
    return state.targetSurface;
  }

  bool targetReached(const State& state) const { return state.targetReached; }

  bool endOfWorldReached(const State& state) const {
    return state.currentVolume == nullptr;
  }

  bool navigationBreak(const State& state) const {
    return state.navigationBreak;
  }

  void currentSurface(State& state, const Surface* surface) const {
    state.currentSurface = surface;
  }

  void targetReached(State& state, bool targetReached) const {
    state.targetReached = targetReached;
  }

  void navigationBreak(State& state, bool navigationBreak) const {
    state.navigationBreak = navigationBreak;
  }

  /// @brief Initialize call - start of navigation
  ///
  /// @tparam propagator_state_t The state type of the propagator
  /// @tparam stepper_t The type of stepper used for the propagation
  ///
  /// @param [in,out] state is the propagation state object
  /// @param [in] stepper Stepper in use
  template <typename propagator_state_t, typename stepper_t>
  void initialize(propagator_state_t& state, const stepper_t& stepper) const {
    // Call the navigation helper prior to actual navigation
    ACTS_VERBOSE(volInfo(state) << "Initialization.");

    // Set the world volume if it is not set
    if (state.navigation.worldVolume == nullptr) {
      state.navigation.worldVolume =
          m_cfg.trackingGeometry->highestTrackingVolume();
    }

    // We set the current surface to the start surface
    // for eventual post-update action, e.g. material integration
    // or collection when leaving a surface at the start of
    // an extrapolation process
    state.navigation.currentSurface = state.navigation.startSurface;
    if (state.navigation.currentSurface != nullptr) {
      ACTS_VERBOSE(volInfo(state)
                   << "Current surface set to start surface "
                   << state.navigation.currentSurface->geometryId());
    }

    // Fast Navigation initialization for start condition:
    // - short-cut through object association, saves navigation in the
    // - geometry and volume tree search for the lowest volume
    if (state.navigation.startSurface != nullptr &&
        state.navigation.startSurface->associatedLayer() != nullptr) {
      ACTS_VERBOSE(
          volInfo(state)
          << "Fast start initialization through association from Surface.");
      // assign the current layer and volume by association
      state.navigation.startLayer =
          state.navigation.startSurface->associatedLayer();
      state.navigation.startVolume =
          state.navigation.startLayer->trackingVolume();
    } else if (state.navigation.startVolume != nullptr) {
      ACTS_VERBOSE(
          volInfo(state)
          << "Fast start initialization through association from Volume.");
      state.navigation.startLayer =
          state.navigation.startVolume->associatedLayer(
              state.geoContext, stepper.position(state.stepping));
    } else {
      ACTS_VERBOSE(volInfo(state)
                   << "Slow start initialization through search.");
      // current volume and layer search through global search
      ACTS_VERBOSE(volInfo(state)
                   << "Starting from position "
                   << toString(stepper.position(state.stepping))
                   << " and direction "
                   << toString(stepper.direction(state.stepping)));
      state.navigation.startVolume =
          m_cfg.trackingGeometry->lowestTrackingVolume(
              state.geoContext, stepper.position(state.stepping));
      state.navigation.startLayer =
          state.navigation.startVolume != nullptr
              ? state.navigation.startVolume->associatedLayer(
                    state.geoContext, stepper.position(state.stepping))
              : nullptr;
      if (state.navigation.startVolume != nullptr) {
        ACTS_VERBOSE(volInfo(state) << "Start volume resolved.");
      }
    }

    // Set the start volume as current volume
    state.navigation.currentVolume = state.navigation.startVolume;
    // Set the start layer as current layer
    state.navigation.currentLayer = state.navigation.startLayer;

    if (state.navigation.startLayer != nullptr) {
      ACTS_VERBOSE(volInfo(state) << "Start layer to be resolved.");
      // We provide the layer to the resolve surface method in this case
      bool startResolved = resolveSurfaces(state, stepper);
      if (!startResolved &&
          state.navigation.startLayer == state.navigation.targetLayer) {
        ACTS_VERBOSE(volInfo(state)
                     << "Start is target layer and we have no surface "
                        "candidates. Nothing left to do.");
        // set the navigation break
        state.navigation.navigationBreak = true;
        stepper.releaseStepSize(state.stepping, ConstrainedStep::actor);
      }
    }
  }

  /// @brief Navigator pre step call
  ///
  /// Call options
  /// (a) there are still surfaces to be resolved: handle those
  /// (b) there no surfaces but still layers to be resolved, handle those
  /// (c) there are no surfaces nor layers to be resolved, handle boundary
  ///
  /// @tparam propagator_state_t is the type of Propagatgor state
  /// @tparam stepper_t is the used type of the Stepper by the Propagator
  ///
  /// @param [in,out] state is the mutable propagator state object
  /// @param [in] stepper Stepper in use
  template <typename propagator_state_t, typename stepper_t>
  void preStep(propagator_state_t& state, const stepper_t& stepper) const {
    // Check if the navigator is inactive
    if (inactive(state, stepper)) {
      return;
    }

    // Call the navigation helper prior to actual navigation
    ACTS_VERBOSE(volInfo(state) << "Entering navigator::preStep.");

    // Initialize the target and target volume
    if (state.navigation.targetSurface != nullptr &&
        state.navigation.targetVolume == nullptr) {
      // Find out about the target as much as you can
      initializeTarget(state, stepper);
    }
    // Try targeting the surfaces - then layers - then boundaries
    if (state.navigation.navigationStage <= Stage::surfaceTarget &&
        targetSurfaces(state, stepper)) {
      ACTS_VERBOSE(volInfo(state) << "Target set to next surface.");
    } else if (state.navigation.navigationStage <= Stage::layerTarget &&
               targetLayers(state, stepper)) {
      ACTS_VERBOSE(volInfo(state) << "Target set to next layer.");
    } else if (targetBoundaries(state, stepper)) {
      ACTS_VERBOSE(volInfo(state) << "Target set to next boundary.");
    } else {
      ACTS_VERBOSE(volInfo(state)
                   << "No further navigation action, proceed to target.");
      // Set navigation break and release the navigation step size
      state.navigation.navigationBreak = true;
      stepper.releaseStepSize(state.stepping, ConstrainedStep::actor);
    }

    // Navigator target always resets the current surface
    state.navigation.currentSurface = nullptr;
  }

  /// @brief Navigator post step call
  ///
  /// (a) It initializes the Navigation stream if start volume is
  ///     not yet defined:
  ///  - initialize the volume
  ///  - establish the start layer and start volume
  ///  - set the current surface to the start surface
  ///
  /// (b) It establishes the currentSurface status during
  ///     the propagation flow, currentSurface can be
  ///  - surfaces still to be handled within a layer
  ///  - layers still to be handled within a volume
  ///  - boundaries still to be handled to exit a volume
  ///
  /// @tparam propagator_state_t is the type of Propagatgor state
  /// @tparam stepper_t is the used type of the Stepper by the Propagator
  ///
  /// @param [in,out] state is the mutable propagator state object
  /// @param [in] stepper Stepper in use
  template <typename propagator_state_t, typename stepper_t>
  void postStep(propagator_state_t& state, const stepper_t& stepper) const {
    // Check if the navigator is inactive
    if (inactive(state, stepper)) {
      return;
    }

    // Set the navigation stage
    state.navigation.navigationStage = Stage::undefined;

    // Call the navigation helper prior to actual navigation
    ACTS_VERBOSE(volInfo(state) << "Entering navigator::postStep.");

    // Navigator post step always starts without current surface
    state.navigation.currentSurface = nullptr;

    // (b) Status call within propagation loop
    // Try finding status of surfaces
    if (surfaceStatus(state, stepper, state.navigation.navSurfaces,
                      state.navigation.navSurfaceIndex)) {
      ACTS_VERBOSE(volInfo(state) << "Post step: in surface handling.");
      if (state.navigation.currentSurface != nullptr) {
        ACTS_VERBOSE(volInfo(state)
                     << "On surface: switch forward or release.");
        if (++state.navigation.navSurfaceIndex ==
            state.navigation.navSurfaces.size()) {
          // this was the last surface, check if we have layers
          if (!state.navigation.navLayers.empty()) {
            ++state.navigation.navLayerIndex;
          } else {
            state.navigation.navigationStage = Stage::layerTarget;
            ACTS_VERBOSE(volInfo(state) << "Target layers.");
            return;
          }
        }
      }
      // Set the navigation stage to surface target
      state.navigation.navigationStage = Stage::surfaceTarget;
      ACTS_VERBOSE(volInfo(state) << "Staying focussed on surface.");
      // Try finding status of layer
    } else if (surfaceStatus(state, stepper, state.navigation.navLayers,
                             state.navigation.navLayerIndex)) {
      ACTS_VERBOSE(volInfo(state) << "Post step: in layer handling.");
      if (state.navigation.currentSurface != nullptr) {
        ACTS_VERBOSE(volInfo(state) << "On layer: update layer information.");
        state.navigation.currentLayer = state.navigation.navLayer().second;
        if (resolveSurfaces(state, stepper)) {
          // Set the navigation stage back to surface handling
          state.navigation.navigationStage = Stage::surfaceTarget;
          return;
        }
      } else {
        // Set the navigation stage to layer target
        state.navigation.navigationStage = Stage::layerTarget;
        ACTS_VERBOSE(volInfo(state) << "Staying focussed on layer.");
      }
      // Try finding status of boundaries
    } else if (surfaceStatus(state, stepper, state.navigation.navBoundaries,
                             state.navigation.navBoundaryIndex)) {
      ACTS_VERBOSE(volInfo(state) << "Post step: in boundary handling.");

      // Are we on the boundary - then overwrite the stage
      if (state.navigation.currentSurface != nullptr) {
        // Set the navigation stage back to surface handling
        ACTS_VERBOSE(volInfo(state)
                     << "On boundary: update volume information.");
        // We are on a boundary, reset all information
        state.navigation.navSurfaces.clear();
        state.navigation.navSurfaceIndex = state.navigation.navSurfaces.size();
        state.navigation.navLayers.clear();
        state.navigation.navLayerIndex = state.navigation.navLayers.size();
        // Update volume information
        // get the attached volume information
        const BoundarySurface* boundary = state.navigation.navBoundary().second;
        state.navigation.currentVolume = boundary->attachedVolume(
            state.geoContext, stepper.position(state.stepping),
            state.options.direction * stepper.direction(state.stepping));
        state.navigation.currentLayer = nullptr;
        // No volume anymore : end of known world
        if (state.navigation.currentVolume == nullptr) {
          ACTS_VERBOSE(
              volInfo(state)
              << "No more volume to progress to, stopping navigation.");
          // Navigation break & release navigation stepping
          state.navigation.navigationBreak = true;
          return;
        } else {
          ACTS_VERBOSE(volInfo(state) << "Volume updated.");
          // Forget the boundary information
          state.navigation.navBoundaries.clear();
          state.navigation.navBoundaryIndex =
              state.navigation.navBoundaries.size();
        }
      } else {
        // Set the navigation stage back to boundary target
        state.navigation.navigationStage = Stage::boundaryTarget;
        ACTS_VERBOSE(volInfo(state) << "Staying focussed on boundary.");
      }
    } else if (state.navigation.currentVolume ==
               state.navigation.targetVolume) {
      if (state.navigation.targetSurface == nullptr) {
        ACTS_WARNING(volInfo(state)
                     << "No further navigation action, proceed to "
                        "target. This is very likely an error");
      } else {
        ACTS_VERBOSE(volInfo(state)
                     << "No further navigation action, proceed to target.");
      }
      // Set navigation break and release the navigation step size
      state.navigation.navigationBreak = true;
    } else {
      ACTS_VERBOSE(volInfo(state)
                   << "Status could not be determined - good luck.");
    }
  }

 private:
  const SurfaceIntersection& candidateIntersection(
      const NavigationSurfaces& surfaces, std::size_t index) const {
    return surfaces.at(index);
  }
  const SurfaceIntersection& candidateIntersection(
      const NavigationLayers& surfaces, std::size_t index) const {
    return surfaces.at(index).first;
  }
  const SurfaceIntersection& candidateIntersection(
      const NavigationBoundaries& surfaces, std::size_t index) const {
    return surfaces.at(index).first;
  }

  /// @brief Status call for test surfaces (surfaces, layers, boundaries)
  ///
  /// If there are surfaces to be handled, check if the current
  /// state is on the surface
  ///
  /// @tparam propagator_state_t The state type of the propagator
  /// @tparam stepper_t The type of stepper used for the propagation
  /// @tparam navigation_surfaces_t Type of the propagator
  ///
  /// @param [in,out] state is the propagation state object
  /// @param [in] stepper Stepper in use
  /// @param [in] navSurfaces is the navigation status objects
  /// @param [in] navIndex test surface fore the status test
  ///
  /// @return boolean return triggers exit to stepper
  template <typename propagator_state_t, typename stepper_t,
            typename navigation_surfaces_t>
  bool surfaceStatus(propagator_state_t& state, const stepper_t& stepper,
                     const navigation_surfaces_t& navSurfaces,
                     std::size_t navIndex) const {
    // No surfaces, status check will be done on layer
    if (navSurfaces.empty() || navIndex == navSurfaces.size()) {
      return false;
    }
    const auto& intersection = candidateIntersection(navSurfaces, navIndex);
    // Take the current surface
    const auto* surface = intersection.object();
    // Check if we are at a surface
    // If we are on the surface pointed at by the index, we can make
    // it the current one to pass it to the other actors
    auto surfaceStatus = stepper.updateSurfaceStatus(
        state.stepping, *surface, intersection.index(), state.options.direction,
        BoundaryTolerance::None(), state.options.surfaceTolerance, logger());
    if (surfaceStatus == Intersection3D::Status::onSurface) {
      ACTS_VERBOSE(volInfo(state)
                   << "Status Surface successfully hit, storing it.");
      // Set in navigation state, so actors and aborters can access it
      state.navigation.currentSurface = surface;
      if (state.navigation.currentSurface != nullptr) {
        ACTS_VERBOSE(volInfo(state)
                     << "Current surface set to surface "
                     << state.navigation.currentSurface->geometryId());
      }
    }
    // Return a positive status: either on it, or on the way
    return true;
  }

  /// Loop over surface candidates here:
  ///  - if an intersect is  valid but not yet reached
  ///    then return with updated step size
  ///  - if an intersect is not valid, switch to next
  ///
  /// @tparam propagator_state_t The state type of the propagator
  /// @tparam stepper_t The type of stepper used for the propagation
  ///
  /// @param [in,out] state is the propagation state object
  /// @param [in] stepper Stepper in use
  ///
  /// boolean return triggers exit to stepper
  template <typename propagator_state_t, typename stepper_t>
  bool targetSurfaces(propagator_state_t& state,
                      const stepper_t& stepper) const {
    if (state.navigation.navigationBreak) {
      return false;
    }

    // The call that we are on a layer and have not yet resolved the surfaces
    // No surfaces, do not return to stepper
    if (state.navigation.navSurfaces.empty() ||
        state.navigation.navSurfaceIndex ==
            state.navigation.navSurfaces.size()) {
      ACTS_VERBOSE(volInfo(state)
                   << "No surfaces present, target at layer first.");
      return false;
    }

    auto layerID = state.navigation.navSurface().object()->geometryId().layer();
    std::pair<ExternalSurfaces::iterator, ExternalSurfaces::iterator>
        externalSurfaceRange =
            state.navigation.options.externalSurfaces.equal_range(layerID);
    // Loop over the remaining navigation surfaces
    while (state.navigation.navSurfaceIndex !=
           state.navigation.navSurfaces.size()) {
      // Screen output how much is left to try
      ACTS_VERBOSE(volInfo(state)
                   << (state.navigation.navSurfaces.size() -
                       state.navigation.navSurfaceIndex)
                   << " out of " << state.navigation.navSurfaces.size()
                   << " surfaces remain to try.");
      const auto& intersection = state.navigation.navSurface();
      // Take the surface
      const auto* surface = intersection.object();
      // Screen output which surface you are on
      ACTS_VERBOSE(volInfo(state) << "Next surface candidate will be "
                                  << surface->geometryId());
      // Estimate the surface status
      BoundaryTolerance boundaryTolerance = BoundaryTolerance::None();
      for (auto it = externalSurfaceRange.first;
           it != externalSurfaceRange.second; it++) {
        if (surface->geometryId() == it->second) {
          boundaryTolerance = BoundaryTolerance::Infinite();
          break;
        }
      }
      auto surfaceStatus = stepper.updateSurfaceStatus(
          state.stepping, *surface, intersection.index(),
          state.options.direction, boundaryTolerance,
          state.options.surfaceTolerance, logger());
      if (surfaceStatus == Intersection3D::Status::reachable) {
        ACTS_VERBOSE(volInfo(state)
                     << "Surface reachable, step size updated to "
                     << stepper.outputStepSize(state.stepping));
        return true;
      }
      ++state.navigation.navSurfaceIndex;
      continue;
    }

    // Reached the end of the surface iteration
    if (state.navigation.navSurfaceIndex ==
        state.navigation.navSurfaces.size()) {
      // first clear the surface cache
      state.navigation.navSurfaces.clear();
      state.navigation.navSurfaceIndex = state.navigation.navSurfaces.size();

      if (state.navigation.navLayerIndex != state.navigation.navLayers.size()) {
        ACTS_VERBOSE(volInfo(state)
                     << "Last surface on layer reached, switching layer.");
        // now switch to the next layer
        ++state.navigation.navLayerIndex;
      } else {
        ACTS_VERBOSE(volInfo(state)
                     << "Last surface on layer reached, and no layer.");
        state.navigation.navigationBreak =
            (state.navigation.currentVolume == state.navigation.targetVolume);
      }
    }

    // Do not return to the propagator
    return false;
  }

  /// @brief Target layer candidates.
  ///
  /// We are now trying to advance to the next layer (with surfaces)
  /// Check if we are on the representing surface of the layer pointed
  /// at by navLayerIndex. If so, we unpack the compatible surfaces
  /// (determined by straight line intersect), and set up the index
  /// so that the next postStep() call will enter the surface
  /// check mode above. If no surfaces are found, we skip the layer.
  /// If we unpack a surface, the step size is set to the path length
  /// to the first surface, as determined by straight line intersect.
  ///
  /// @tparam propagator_state_t The state type of the propagator
  /// @tparam stepper_t The type of stepper used for the propagation
  ///
  /// @param [in,out] state is the propagation state object
  /// @param [in] stepper Stepper in use
  ///
  /// @return boolean return triggers exit to stepper
  template <typename propagator_state_t, typename stepper_t>
  bool targetLayers(propagator_state_t& state, const stepper_t& stepper) const {
    using namespace UnitLiterals;

    if (state.navigation.navigationBreak) {
      return false;
    }

    // if there are no layers, go back to the navigator (not stepper yet)
    if (state.navigation.navLayers.empty()) {
      ACTS_VERBOSE(volInfo(state)
                   << "No layers present, resolve volume first.");

      if (resolveLayers(state, stepper)) {
        // The layer resolving worked
        return true;
      }
    }

    // loop over the available navigation layer candidates
    while (state.navigation.navLayerIndex !=
           state.navigation.navLayers.size()) {
      const auto& intersection = state.navigation.navLayer().first;
      // The layer surface
      const auto* layerSurface = intersection.object();
      // We are on the layer
      if (state.navigation.currentSurface == layerSurface) {
        ACTS_VERBOSE(volInfo(state) << "We are on a layer, resolve Surfaces.");
        // If you found surfaces return to the propagator
        if (resolveSurfaces(state, stepper)) {
          return true;
        } else {
          // Try the next one
          ++state.navigation.navLayerIndex;
          continue;
        }
      }
      // Try to step towards it
      auto layerStatus = stepper.updateSurfaceStatus(
          state.stepping, *layerSurface, intersection.index(),
          state.options.direction, BoundaryTolerance::None(),
          state.options.surfaceTolerance, logger());
      if (layerStatus == Intersection3D::Status::reachable) {
        ACTS_VERBOSE(volInfo(state) << "Layer reachable, step size updated to "
                                    << stepper.outputStepSize(state.stepping));
        return true;
      }
      ACTS_VERBOSE(volInfo(state)
                   << "Layer intersection not valid, skipping it.");
      ++state.navigation.navLayerIndex;
    }

    // Re-initialize target at last layer, only in case it is the target volume
    // This avoids a wrong target volume estimation
    if (state.navigation.currentVolume == state.navigation.targetVolume) {
      initializeTarget(state, stepper);
    }
    // Screen output
    if (logger().doPrint(Logging::VERBOSE)) {
      std::ostringstream os;
      os << "Last layer";
      if (state.navigation.currentVolume == state.navigation.targetVolume) {
        os << " (final volume) done, proceed to target.";
      } else {
        os << " done, target volume boundary.";
      }
      logger().log(Logging::VERBOSE, os.str());
    }
    // Set the navigation break if necessary
    state.navigation.navigationBreak =
        (state.navigation.currentVolume == state.navigation.targetVolume);
    return false;
  }

  /// @brief Navigation through volumes
  ///
  /// This is the boundary check routine. If the code above set up the
  /// boundary surface index, we advance through them here. If we are on
  /// the boundary surface, we set the current surface to the boundary
  /// surface, and get the volume pointed at by the boundary surface.  Next
  /// we unpack the layers from that volume. If the volume contains layers
  /// we set the step size to the straight line path length to the first
  /// layer.  If we don't find a next volume, the navigationBreak
  /// indicator is set.  This ends the navigation. Finally, the boundary
  /// index is cleared, so that the subsequent call goes back to
  /// the layer iteration logic.
  ///
  /// If we are not on the current boundary surface, we try the next one.
  /// The index is advanced and the step size is set. If no straight
  /// line intersect is found, the boundary surface is skipped.
  /// If we are out of boundary surfaces, the navigation is terminated.
  ///
  /// @tparam propagator_state_t The state type of the propagator
  /// @tparam stepper_t The type of stepper used for the propagation
  ///
  /// @param [in,out] state is the propagation state object
  /// @param [in] stepper Stepper in use
  ///
  /// boolean return triggers exit to stepper
  template <typename propagator_state_t, typename stepper_t>
  bool targetBoundaries(propagator_state_t& state,
                        const stepper_t& stepper) const {
    if (state.navigation.navigationBreak) {
      return false;
    }

    if (state.navigation.currentVolume == nullptr) {
      ACTS_VERBOSE(volInfo(state)
                   << "No sufficient information to resolve boundary, "
                      "stopping navigation.");
      stepper.releaseStepSize(state.stepping, ConstrainedStep::actor);
      return false;
    } else if (state.navigation.currentVolume ==
               state.navigation.targetVolume) {
      ACTS_VERBOSE(volInfo(state)
                   << "In target volume: no need to resolve boundary, "
                      "stopping navigation.");
      state.navigation.navigationBreak = true;
      stepper.releaseStepSize(state.stepping, ConstrainedStep::actor);
      return true;
    }

    // Re-initialize target at volume boundary
    initializeTarget(state, stepper);

    // Helper function to find boundaries
    auto findBoundaries = [&]() -> bool {
      // The navigation options
      NavigationOptions<Surface> navOpts;
      // Exclude the current surface in case it's a boundary
      navOpts.startObject = state.navigation.currentSurface;
      navOpts.nearLimit = state.options.surfaceTolerance;
      navOpts.farLimit =
          stepper.getStepSize(state.stepping, ConstrainedStep::aborter);
      navOpts.forceIntersectBoundaries =
          state.navigation.forceIntersectBoundaries;

      ACTS_VERBOSE(volInfo(state)
                   << "Try to find boundaries, we are at: "
                   << stepper.position(state.stepping).transpose() << ", dir: "
                   << stepper.direction(state.stepping).transpose());

      // Evaluate the boundary surfaces
      state.navigation.navBoundaries =
          state.navigation.currentVolume->compatibleBoundaries(
              state.geoContext, stepper.position(state.stepping),
              state.options.direction * stepper.direction(state.stepping),
              navOpts, logger());
      std::sort(state.navigation.navBoundaries.begin(),
                state.navigation.navBoundaries.end(),
                [](const auto& a, const auto& b) {
                  return SurfaceIntersection::pathLengthOrder(a.first, b.first);
                });

      // Print boundary information
      if (logger().doPrint(Logging::VERBOSE)) {
        std::ostringstream os;
        os << state.navigation.navBoundaries.size();
        os << " boundary candidates found at path(s): ";
        for (auto& bc : state.navigation.navBoundaries) {
          os << bc.first.pathLength() << "  ";
        }
        logger().log(Logging::VERBOSE, os.str());
      }

      // Set the begin index
      state.navigation.navBoundaryIndex = 0;
      if (!state.navigation.navBoundaries.empty()) {
        // Set to the first and return to the stepper
        stepper.updateStepSize(state.stepping,
                               state.navigation.navBoundary().first,
                               state.options.direction, true);
        ACTS_VERBOSE(volInfo(state) << "Navigation stepSize updated to "
                                    << stepper.outputStepSize(state.stepping));
        return true;
      }
      return false;
    };

    // No boundaries are assigned yet, find them
    if (state.navigation.navBoundaries.empty() && findBoundaries()) {
      return true;
    }

    // Loop over the boundary surface
    while (state.navigation.navBoundaryIndex !=
           state.navigation.navBoundaries.size()) {
      const auto& intersection = state.navigation.navBoundary().first;
      // That is the current boundary surface
      const auto* boundarySurface = intersection.object();
      // Step towards the boundary surfrace
      auto boundaryStatus = stepper.updateSurfaceStatus(
          state.stepping, *boundarySurface, intersection.index(),
          state.options.direction, BoundaryTolerance::None(),
          state.options.surfaceTolerance, logger());
      if (boundaryStatus == Intersection3D::Status::reachable) {
        ACTS_VERBOSE(volInfo(state)
                     << "Boundary reachable, step size updated to "
                     << stepper.outputStepSize(state.stepping));
        return true;
      } else {
        ACTS_VERBOSE("Boundary "
                     << (state.navigation.navBoundaries.size() -
                         state.navigation.navBoundaryIndex)
                     << " out of " << state.navigation.navBoundaries.size()
                     << " not reachable anymore, switching to next.");
        ACTS_VERBOSE("Targeted boundary surface was: \n"
                     << boundarySurface->toStream(state.geoContext));
      }
      // Increase the index to the next one
      ++state.navigation.navBoundaryIndex;
    }
    // We have to leave the volume somehow, so try again
    state.navigation.navBoundaries.clear();
    ACTS_VERBOSE(volInfo(state) << "Boundary navigation lost, re-targetting.");
    state.navigation.forceIntersectBoundaries = true;
    if (findBoundaries()) {
      // Resetting intersection check for boundary surfaces
      state.navigation.forceIntersectBoundaries = false;
      return true;
    }

    // Tried our best, but couldn't do anything
    return false;
  }

  /// @brief Navigation (re-)initialisation for the target
  ///
  /// @note This is only called a few times every propagation/extrapolation
  ///
  /// As a straight line estimate can lead you to the wrong destination
  /// Volume, this will be called at:
  /// - initialization
  /// - attempted volume switch
  /// Target finding by association will not be done again
  ///
  /// @tparam propagator_state_t The state type of the propagator
  /// @tparam stepper_t The type of stepper used for the propagation
  ///
  /// @param [in,out] state is the propagation state object
  /// @param [in] stepper Stepper in use
  ///
  /// boolean return triggers exit to stepper
  template <typename propagator_state_t, typename stepper_t>
  void initializeTarget(propagator_state_t& state,
                        const stepper_t& stepper) const {
    if (state.navigation.targetVolume != nullptr &&
        state.stepping.pathAccumulated == 0.) {
      ACTS_VERBOSE(volInfo(state)
                   << "Re-initialzing cancelled as it is the first step.");
      return;
    }
    // Fast Navigation initialization for target:
    if (state.navigation.targetSurface != nullptr &&
        state.navigation.targetSurface->associatedLayer() &&
        state.navigation.targetVolume == nullptr) {
      ACTS_VERBOSE(volInfo(state)
                   << "Fast target initialization through association.");
      ACTS_VERBOSE(volInfo(state)
                   << "Target surface set to "
                   << state.navigation.targetSurface->geometryId());
      state.navigation.targetLayer =
          state.navigation.targetSurface->associatedLayer();
      state.navigation.targetVolume =
          state.navigation.targetLayer->trackingVolume();
    } else if (state.navigation.targetSurface != nullptr) {
      // screen output that you do a re-initialization
      if (state.navigation.targetVolume != nullptr) {
        ACTS_VERBOSE(volInfo(state)
                     << "Re-initialization of target volume triggered.");
      }
      // Slow navigation initialization for target:
      // target volume and layer search through global search
      auto targetIntersection =
          state.navigation.targetSurface
              ->intersect(
                  state.geoContext, stepper.position(state.stepping),
                  state.options.direction * stepper.direction(state.stepping),
                  BoundaryTolerance::Infinite(), state.options.surfaceTolerance)
              .closest();
      if (targetIntersection.isValid()) {
        ACTS_VERBOSE(volInfo(state)
                     << "Target estimate position ("
                     << targetIntersection.position().x() << ", "
                     << targetIntersection.position().y() << ", "
                     << targetIntersection.position().z() << ")");
        // get the target volume from the intersection
        auto tPosition = targetIntersection.position();
        state.navigation.targetVolume =
            m_cfg.trackingGeometry->lowestTrackingVolume(state.geoContext,
                                                         tPosition);
        state.navigation.targetLayer =
            state.navigation.targetVolume != nullptr
                ? state.navigation.targetVolume->associatedLayer(
                      state.geoContext, tPosition)
                : nullptr;
        if (state.navigation.targetVolume != nullptr) {
          ACTS_VERBOSE(volInfo(state)
                       << "Target volume estimated : "
                       << state.navigation.targetVolume->volumeName());
        }
      }
    }
  }

  /// @brief Resolve the surfaces of this layer
  ///
  /// @tparam propagator_state_t The state type of the propagator
  /// @tparam stepper_t The type of stepper used for the propagation
  ///
  /// @param [in,out] state is the propagation state object
  /// @param [in] stepper Stepper in use
  ///
  /// boolean return triggers exit to stepper
  template <typename propagator_state_t, typename stepper_t>
  bool resolveSurfaces(propagator_state_t& state,
                       const stepper_t& stepper) const {
    // get the layer and layer surface
    const Layer* currentLayer = state.navigation.currentLayer;
    assert(currentLayer != nullptr && "Current layer is not set.");
    const Surface* layerSurface = &currentLayer->surfaceRepresentation();
    // Use navigation parameters and NavigationOptions
    NavigationOptions<Surface> navOpts;
    navOpts.resolveSensitive = m_cfg.resolveSensitive;
    navOpts.resolveMaterial = m_cfg.resolveMaterial;
    navOpts.resolvePassive = m_cfg.resolvePassive;
    navOpts.startObject = state.navigation.currentSurface;
    navOpts.endObject = state.navigation.targetSurface;

    std::vector<GeometryIdentifier> externalSurfaces;
    if (!state.navigation.options.externalSurfaces.empty()) {
      auto layerID = layerSurface->geometryId().layer();
      auto externalSurfaceRange =
          state.navigation.options.externalSurfaces.equal_range(layerID);
      navOpts.externalSurfaces.reserve(
          state.navigation.options.externalSurfaces.count(layerID));
      for (auto itSurface = externalSurfaceRange.first;
           itSurface != externalSurfaceRange.second; itSurface++) {
        navOpts.externalSurfaces.push_back(itSurface->second);
      }
    }
    navOpts.nearLimit = state.options.surfaceTolerance;
    navOpts.farLimit =
        stepper.getStepSize(state.stepping, ConstrainedStep::aborter);

    // get the surfaces
    state.navigation.navSurfaces = currentLayer->compatibleSurfaces(
        state.geoContext, stepper.position(state.stepping),
        state.options.direction * stepper.direction(state.stepping), navOpts);
    std::sort(state.navigation.navSurfaces.begin(),
              state.navigation.navSurfaces.end(),
              SurfaceIntersection::pathLengthOrder);

    // Print surface information
    if (logger().doPrint(Logging::VERBOSE)) {
      std::ostringstream os;
      os << state.navigation.navSurfaces.size();
      os << " surface candidates found at path(s): ";
      for (auto& sfc : state.navigation.navSurfaces) {
        os << sfc.pathLength() << "  ";
      }
      logger().log(Logging::VERBOSE, os.str());
    }

    // Surface candidates have been found
    if (!state.navigation.navSurfaces.empty()) {
      // set the index
      state.navigation.navSurfaceIndex = 0;
      // The stepper updates the step size ( single / multi component)
      stepper.updateStepSize(state.stepping, state.navigation.navSurface(),
                             state.options.direction, true);
      ACTS_VERBOSE(volInfo(state) << "Navigation stepSize updated to "
                                  << stepper.outputStepSize(state.stepping));
      return true;
    }

    state.navigation.navSurfaceIndex = state.navigation.navSurfaces.size();
    ACTS_VERBOSE(volInfo(state) << "No surface candidates found.");
    return false;
  }

  /// @brief Navigation through layers
  ///
  /// Resolve layers.
  ///
  /// This initializes the layer candidates when starting
  /// or when entering a new volume
  ///
  /// @tparam propagator_state_t The state type of the propagator
  /// @tparam stepper_t The type of stepper used for the propagation
  ///
  /// @param [in,out] state is the propagation state object
  /// @param [in] stepper Stepper in use
  ///
  /// @return boolean return triggers exit to stepper
  template <typename propagator_state_t, typename stepper_t>
  bool resolveLayers(propagator_state_t& state,
                     const stepper_t& stepper) const {
    ACTS_VERBOSE(volInfo(state) << "Searching for compatible layers.");

    // Create the navigation options
    // - and get the compatible layers, start layer will be excluded
    NavigationOptions<Layer> navOpts;
    navOpts.resolveSensitive = m_cfg.resolveSensitive;
    navOpts.resolveMaterial = m_cfg.resolveMaterial;
    navOpts.resolvePassive = m_cfg.resolvePassive;
    navOpts.startObject = state.navigation.currentLayer;
    navOpts.nearLimit = state.options.surfaceTolerance;
    navOpts.farLimit =
        stepper.getStepSize(state.stepping, ConstrainedStep::aborter);

    // Request the compatible layers
    state.navigation.navLayers =
        state.navigation.currentVolume->compatibleLayers(
            state.geoContext, stepper.position(state.stepping),
            state.options.direction * stepper.direction(state.stepping),
            navOpts);
    std::sort(state.navigation.navLayers.begin(),
              state.navigation.navLayers.end(),
              [](const auto& a, const auto& b) {
                return SurfaceIntersection::pathLengthOrder(a.first, b.first);
              });

    // Print layer information
    if (logger().doPrint(Logging::VERBOSE)) {
      std::ostringstream os;
      os << state.navigation.navLayers.size();
      os << " layer candidates found at path(s): ";
      for (auto& lc : state.navigation.navLayers) {
        os << lc.first.pathLength() << "  ";
      }
      logger().log(Logging::VERBOSE, os.str());
    }

    // Layer candidates have been found
    if (!state.navigation.navLayers.empty()) {
      // Set the index to the first
      state.navigation.navLayerIndex = 0;
      // Setting the step size towards first
      ACTS_VERBOSE(volInfo(state) << "Target at layer.");
      // The stepper updates the step size ( single / multi component)
      stepper.updateStepSize(state.stepping, state.navigation.navLayer().first,
                             state.options.direction, true);
      ACTS_VERBOSE(volInfo(state) << "Navigation stepSize updated to "
                                  << stepper.outputStepSize(state.stepping));
      return true;
    }

    // Set the index to the end of the list
    state.navigation.navLayerIndex = state.navigation.navLayers.size();

    // Screen output - no layer candidates found
    ACTS_VERBOSE(volInfo(state) << "No compatible layer candidates found.");
    // Release the step size
    stepper.releaseStepSize(state.stepping, ConstrainedStep::actor);
    return false;
  }

  /// Inactive
  ///
  /// This checks if a navigation break had been triggered or navigator
  /// is misconfigured
  ///
  /// @tparam propagator_state_t The state type of the propagator
  /// @tparam stepper_t The type of stepper used for the propagation
  ///
  /// @param [in,out] state is the propagation state object
  /// @param [in] stepper Stepper in use
  ///
  /// boolean return triggers exit to stepper
  template <typename propagator_state_t, typename stepper_t>
  bool inactive(propagator_state_t& state, const stepper_t& stepper) const {
    // Void behavior in case no tracking geometry is present
    if (m_cfg.trackingGeometry == nullptr) {
      return true;
    }
    // turn the navigator into void when you are instructed to do nothing
    if (!m_cfg.resolveSensitive && !m_cfg.resolveMaterial &&
        !m_cfg.resolvePassive) {
      return true;
    }

    // Navigation break handling
    // This checks if a navigation break had been triggered:
    // - If so & the target exists or was hit - it simply returns
    // - If a target exists and was not yet hit, it checks for it
    // -> return is always to the stepper
    if (state.navigation.navigationBreak) {
      // target exists and reached, or no target exists
      if (state.navigation.targetReached ||
          state.navigation.targetSurface == nullptr) {
        return true;
      }
      // TODO we do not know the intersection index - passing 0
      auto targetStatus = stepper.updateSurfaceStatus(
          state.stepping, *state.navigation.targetSurface, 0,
          state.options.direction, BoundaryTolerance::None(),
          state.options.surfaceTolerance, logger());
      // the only advance could have been to the target
      if (targetStatus == Intersection3D::Status::onSurface) {
        // set the target surface
        state.navigation.currentSurface = state.navigation.targetSurface;
        ACTS_VERBOSE(volInfo(state)
                     << volInfo(state)
                     << "Current surface set to target surface "
                     << state.navigation.currentSurface->geometryId());
        return true;
      }
    }
    return false;
  }

 private:
  template <typename propagator_state_t>
  std::string volInfo(const propagator_state_t& state) const {
    return (state.navigation.currentVolume != nullptr
                ? state.navigation.currentVolume->volumeName()
                : "No Volume") +
           " | ";
  }

  const Logger& logger() const { return *m_logger; }

  Config m_cfg;

  std::shared_ptr<const Logger> m_logger;
};

}  // namespace Acts
