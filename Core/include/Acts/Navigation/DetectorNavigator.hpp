// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Geometry/BoundarySurfaceT.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Surfaces/BoundaryCheck.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BoundingBox.hpp"
#include "Acts/Utilities/Frustum.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/container/small_vector.hpp>

namespace Acts::Experimental {
using Frustum3 = Acts::Frustum<Acts::ActsScalar, 3, 3>;
using BoundingBox =
    Acts::AxisAlignedBoundingBox<DetectorVolume, Acts::ActsScalar, 3>;

class DetectorNavigator {
 public:
  struct Config {
    /// Detector for this Navigation
    const Detector* detector = nullptr;
    /// The octree for the world
    const BoundingBox* topBox = nullptr;
    /// The current frustum
    std::shared_ptr<Frustum3> frustum = nullptr;
    /// Configuration for this Navigator
    /// stop at every sensitive surface (whether it has material or not)
    bool resolveSensitive = true;
    /// stop at every material surface (whether it is passive or not)
    bool resolveMaterial = true;
    /// stop at every surface regardless what it is
    bool resolvePassive = false;
    /// using the frustum navigator
    bool frustumNavigator = false;
  };

  /// Nested State struct
  ///
  /// It acts as an internal state which is
  /// created for every propagation/extrapolation step
  /// and keep thread-local navigation information
  struct State : public NavigationState {
    /// Navigation state - external state: the start surface
    const Surface* startSurface = nullptr;
    /// Navigation state - external state: the current surface
    const Surface* currentSurface = nullptr;
    /// Navigation state - external state: the target surface
    const Surface* targetSurface = nullptr;
    /// Indicator if the target is reached
    bool targetReached = false;
    /// Navigation state : a break has been detected
    bool navigationBreak = false;
  };

  /// Constructor with configuration object
  ///
  /// @param cfg The navigator configuration
  /// @param _logger a logger instance
  explicit DetectorNavigator(Config cfg,
                             std::shared_ptr<const Logger> _logger =
                                 getDefaultLogger("DetectorNavigator",
                                                  Logging::Level::INFO))
      : m_cfg{std::move(cfg)}, m_logger{std::move(_logger)} {}

  State makeState(const Surface* startSurface,
                  const Surface* targetSurface) const {
    State result;
    result.startSurface = startSurface;
    result.targetSurface = targetSurface;
    return result;
  }

  const Surface* currentSurface(const State& state) const {
    return state.currentSurface;
  }

  const DetectorVolume* currentVolume(const State& state) const {
    return state.currentVolume;
  }

  const IVolumeMaterial* currentVolumeMaterial(const State& state) const {
    return state.currentVolume->volumeMaterial();
  }

  const Surface* startSurface(const State& state) const {
    return state.startSurface;
  }

  const Surface* targetSurface(const State& state) const {
    return state.targetSurface;
  }

  bool targetReached(const State& state) const { return state.targetReached; }

  bool endOfWorldReached(State& state) const {
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

  void insertExternalSurface(State& /*state*/,
                             GeometryIdentifier /*geoid*/) const {
    // TODO what about external surfaces?
  }

  /// Initialize call - start of propagation
  ///
  /// @tparam propagator_state_t The state type of the propagator
  /// @tparam stepper_t The type of stepper used for the propagation
  ///
  /// @param [in,out] state is the propagation state object
  /// @param [in] stepper Stepper in use
  ///
  /// @return boolean return triggers exit to stepper
  template <typename propagator_state_t, typename stepper_t>
  void initialize(propagator_state_t& state, const stepper_t& stepper) const {
    ACTS_VERBOSE(volInfo(state) << posInfo(state, stepper) << "initialize");

    auto& nState = state.navigation;

    if (nState.currentDetector == nullptr) {
      ACTS_VERBOSE("Assigning detector from the config.");
      nState.currentDetector = m_cfg.detector;
      if (m_cfg.frustumNavigator) {
        if (m_cfg.topBox == nullptr) {
          throw std::invalid_argument(
              "DetectorNavigator: no octree assigned for frustum navigator");
        }
        ACTS_VERBOSE("Assigning top box of the octree for frustum navigator.");
        nState.topBox = m_cfg.topBox;
        nState.frustum = m_cfg.frustum;
      } else if (m_cfg.topBox != nullptr) {
        throw std::invalid_argument(
            "DetectorNavigator: octree assigned but frustum navigator not "
            "enabled");
      }
    }
    if (nState.currentDetector == nullptr) {
      throw std::invalid_argument("DetectorNavigator: no detector assigned");
    }

    fillNavigationState(state, stepper, nState);
    if (nState.currentVolume == nullptr) {
      nState.currentVolume = nState.currentDetector->findDetectorVolume(
          state.geoContext, nState.position);
    }
    if (nState.currentVolume == nullptr) {
      throw std::invalid_argument("DetectorNavigator: no current volume found");
    }
    updateCandidateSurfaces(state, stepper);
  }

  /// @brief Navigator pre step call
  ///
  /// This will invalid the current surface and current portal in order
  /// to navigate to the next ones.
  ///
  /// @tparam propagator_state_t is the type of Propagatgor state
  /// @tparam stepper_t is the used type of the Stepper by the Propagator
  ///
  /// @param [in,out] state is the mutable propagator state object
  /// @param [in] stepper Stepper in use
  template <typename propagator_state_t, typename stepper_t>
  void preStep(propagator_state_t& state, const stepper_t& stepper) const {
    ACTS_VERBOSE(volInfo(state)
                 << posInfo(state, stepper) << "Entering navigator::preStep.");

    if (inactive()) {
      ACTS_VERBOSE(volInfo(state)
                   << posInfo(state, stepper) << "navigator inactive");
      return;
    }

    auto& nState = state.navigation;
    fillNavigationState(state, stepper, nState);

    if (nState.currentSurface != nullptr) {
      ACTS_VERBOSE(volInfo(state)
                   << posInfo(state, stepper) << "stepping through surface");
    } else if (nState.currentPortal != nullptr) {
      ACTS_VERBOSE(volInfo(state)
                   << posInfo(state, stepper) << "stepping through portal");

      nState.surfaceCandidates.clear();
      nState.surfaceCandidateIndex = 0;

      nState.currentPortal->updateDetectorVolume(state.geoContext, nState);

      // If no Volume is found, we are at the end of the world
      if (nState.currentVolume == nullptr) {
        ACTS_VERBOSE(volInfo(state) << posInfo(state, stepper)
                                    << "no volume after Portal update");
        nState.navigationBreak = true;
        return;
      }

      // Switched to a new volume
      // Update candidate surfaces
      updateCandidateSurfaces(state, stepper);
    }
    for (; nState.surfaceCandidateIndex != nState.surfaceCandidates.size();
         ++nState.surfaceCandidateIndex) {
      // Screen output how much is left to try
      ACTS_VERBOSE(volInfo(state)
                   << posInfo(state, stepper)
                   << (nState.surfaceCandidates.size() -
                       nState.surfaceCandidateIndex)
                   << " out of " << nState.surfaceCandidates.size()
                   << " surfaces remain to try.");
      // Take the surface
      const auto& c = nState.surfaceCandidate();
      const auto& surface =
          (c.surface != nullptr) ? (*c.surface) : (c.portal->surface());
      // Screen output which surface you are on
      ACTS_VERBOSE(volInfo(state)
                   << posInfo(state, stepper)
                   << "next surface candidate will be " << surface.geometryId()
                   << " (" << surface.center(state.geoContext).transpose()
                   << ")");
      // Estimate the surface status
      bool boundaryCheck = c.boundaryCheck.isEnabled();
      auto surfaceStatus = stepper.updateSurfaceStatus(
          state.stepping, surface, c.objectIntersection.index(),
          state.options.direction, BoundaryCheck(boundaryCheck),
          state.options.surfaceTolerance, logger());

      ACTS_VERBOSE(volInfo(state) << posInfo(state, stepper)
                                  << "surface status is " << surfaceStatus);

      if (surfaceStatus == Intersection3D::Status::reachable) {
        ACTS_VERBOSE(volInfo(state)
                     << posInfo(state, stepper) << "surface "
                     << surface.center(state.geoContext).transpose()
                     << " is reachable, step size updated to "
                     << stepper.outputStepSize(state.stepping));
        break;
      }
    }

    nState.currentSurface = nullptr;
    nState.currentPortal = nullptr;
  }

  /// @brief Navigator post step call
  ///
  /// @tparam propagator_state_t is the type of Propagatgor state
  /// @tparam stepper_t is the used type of the Stepper by the Propagator
  ///
  /// @param [in,out] state is the mutable propagator state object
  /// @param [in] stepper Stepper in use
  template <typename propagator_state_t, typename stepper_t>
  void postStep(propagator_state_t& state, const stepper_t& stepper) const {
    ACTS_VERBOSE(volInfo(state)
                 << posInfo(state, stepper) << "Entering navigator::postStep.");

    auto& nState = state.navigation;
    fillNavigationState(state, stepper, nState);

    if (inactive()) {
      ACTS_VERBOSE(volInfo(state)
                   << posInfo(state, stepper) << "navigator inactive");
      return;
    }

    if (nState.surfaceCandidateIndex == nState.surfaceCandidates.size()) {
      ACTS_VERBOSE(volInfo(state)
                   << posInfo(state, stepper)
                   << "no surface candidates - waiting for target call");
      return;
    }

    const Portal* nextPortal = nullptr;
    const Surface* nextSurface = nullptr;
    bool isPortal = false;
    bool boundaryCheck = nState.surfaceCandidate().boundaryCheck.isEnabled();

    if (nState.surfaceCandidate().surface != nullptr) {
      nextSurface = nState.surfaceCandidate().surface;
    } else if (nState.surfaceCandidate().portal != nullptr) {
      nextPortal = nState.surfaceCandidate().portal;
      nextSurface = &nextPortal->surface();
      isPortal = true;
    } else {
      std::string msg = "DetectorNavigator: " + volInfo(state) +
                        posInfo(state, stepper) +
                        "panic: not a surface not a portal - what is it?";
      throw std::runtime_error(msg);
    }

    // TODO not sure about the boundary check
    auto surfaceStatus = stepper.updateSurfaceStatus(
        state.stepping, *nextSurface,
        nState.surfaceCandidate().objectIntersection.index(),
        state.options.direction, BoundaryCheck(boundaryCheck),
        state.options.surfaceTolerance, logger());

    // Check if we are at a surface
    if (surfaceStatus == Intersection3D::Status::onSurface) {
      ACTS_VERBOSE(volInfo(state)
                   << posInfo(state, stepper) << "landed on surface");

      if (isPortal) {
        ACTS_VERBOSE(volInfo(state) << posInfo(state, stepper)
                                    << "this is a portal, storing it.");

        nState.currentPortal = nextPortal;

        ACTS_VERBOSE(volInfo(state)
                     << posInfo(state, stepper) << "current portal set to "
                     << nState.currentPortal->surface().geometryId());
      } else {
        ACTS_VERBOSE(volInfo(state) << posInfo(state, stepper)
                                    << "this is a surface, storing it.");

        // If we are on the surface pointed at by the iterator, we can make
        // it the current one to pass it to the other actors
        nState.currentSurface = nextSurface;
        ACTS_VERBOSE(volInfo(state)
                     << posInfo(state, stepper) << "current surface set to "
                     << nState.currentSurface->geometryId());
        ++nState.surfaceCandidateIndex;
      }
    }
  }

 private:
  Config m_cfg;

  std::shared_ptr<const Logger> m_logger;

  template <typename propagator_state_t>
  std::string volInfo(const propagator_state_t& state) const {
    auto& nState = state.navigation;

    return (nState.currentVolume ? nState.currentVolume->name() : "No Volume") +
           " | ";
  }

  template <typename propagator_state_t, typename stepper_t>
  std::string posInfo(const propagator_state_t& state,
                      const stepper_t& stepper) const {
    std::stringstream ss;
    ss << stepper.position(state.stepping).transpose();
    ss << " | ";
    return ss.str();
  }

  const Logger& logger() const { return *m_logger; }

  /// This checks if a navigation break had been triggered or navigator
  /// is misconfigured
  ///
  /// boolean return triggers exit to stepper
  bool inactive() const {
    if (m_cfg.detector == nullptr) {
      return true;
    }

    if (!m_cfg.resolveSensitive && !m_cfg.resolveMaterial &&
        !m_cfg.resolvePassive) {
      return true;
    }

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
  void updateCandidateSurfaces(propagator_state_t& state,
                               const stepper_t& stepper) const {
    ACTS_VERBOSE(volInfo(state)
                 << posInfo(state, stepper) << "initialize target");

    auto& nState = state.navigation;

    // Here we get the candidate surfaces
    nState.currentVolume->updateNavigationState(state.geoContext, nState);

    ACTS_VERBOSE("SURFACE CANDIDATES: " << nState.surfaceCandidates.size());

    // Sort properly the surface candidates
    auto& nCandidates = nState.surfaceCandidates;
    std::sort(nCandidates.begin(), nCandidates.end(),
              [&](const auto& a, const auto& b) {
                // The two path lengths
                ActsScalar pathToA = a.objectIntersection.pathLength();
                ActsScalar pathToB = b.objectIntersection.pathLength();
                return pathToA < pathToB;
              });
    // Set the surface candidate
    nState.surfaceCandidateIndex = 0;
  }

  template <typename propagator_state_t, typename stepper_t>
  void fillNavigationState(propagator_state_t& state, const stepper_t& stepper,
                           NavigationState& nState) const {
    nState.position = stepper.position(state.stepping);
    nState.direction =
        state.options.direction * stepper.direction(state.stepping);
    nState.absMomentum = stepper.absoluteMomentum(state.stepping);
    auto fieldResult = stepper.getField(state.stepping, nState.position);
    if (!fieldResult.ok()) {
      std::string msg = "DetectorNavigator: " + volInfo(state) +
                        posInfo(state, stepper) +
                        "could not read from the magnetic field";
      throw std::runtime_error(msg);
    }
    nState.magneticField = *fieldResult;
  }
};

}  // namespace Acts::Experimental
