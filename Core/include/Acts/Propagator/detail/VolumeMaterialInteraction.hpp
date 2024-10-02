// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Surfaces/Surface.hpp"

namespace Acts::detail {

/// @brief Struct to handle volume material interaction
struct VolumeMaterialInteraction {
  /// The material interaction volume
  InteractionVolume volume{};
  /// The particle current position
  const Vector3 pos = Vector3::Zero();
  /// The particle current time
  const double time = 0;
  /// The particle current direction
  const Vector3 dir = Vector3::Zero();
  /// The particle q/p at the interaction
  const float qOverP = 0;
  /// The absolute particle charge
  const float absQ = 0;
  /// The particle momentum at the interaction
  const float momentum = 0;
  /// The particle mass
  const float mass = 0;
  /// The particle pdg
  const PdgParticle absPdg = eInvalid;
  /// The covariance transport decision at the interaction
  const bool performCovarianceTransport = false;
  /// The navigation direction
  const Direction navDir;

  /// Data evaluated within this struct
  MaterialSlab slab;
  /// The path correction factor due to non-zero incidence on the surface.
  double pathCorrection = 0;

  /// @brief Constructor
  ///
  /// @tparam propagator_state_t Type of the propagator state
  /// @tparam stepper_t Type of the stepper
  ///
  /// @param [in] vVolume The current volume
  /// @param [in] state State of the propagation
  /// @param [in] stepper Stepper in use
  template <typename propagator_state_t, typename stepper_t>
  VolumeMaterialInteraction(const TrackingVolume* vVolume,
                            const propagator_state_t& state,
                            const stepper_t& stepper)
      : volume(vVolume),
        pos(stepper.position(state.stepping)),
        time(stepper.time(state.stepping)),
        dir(stepper.direction(state.stepping)),
        qOverP(stepper.qOverP(state.stepping)),
        absQ(stepper.particleHypothesis(state.stepping).absoluteCharge()),
        momentum(stepper.absoluteMomentum(state.stepping)),
        mass(stepper.particleHypothesis(state.stepping).mass()),
        absPdg(stepper.particleHypothesis(state.stepping).absolutePdg()),
        performCovarianceTransport(state.stepping.covTransport),
        navDir(state.options.direction) {}

  /// @brief Constructor
  ///
  /// @tparam propagator_state_t Type of the propagator state
  /// @tparam stepper_t Type of the stepper
  ///
  /// @param [in] vVolume The current volume
  /// @param [in] state State of the propagation
  /// @param [in] stepper Stepper in use
  template <typename propagator_state_t, typename stepper_t>
  VolumeMaterialInteraction(const Acts::Experimental::DetectorVolume* vVolume,
                            const propagator_state_t& state,
                            const stepper_t& stepper)
      : volume(vVolume),
        pos(stepper.position(state.stepping)),
        time(stepper.time(state.stepping)),
        dir(stepper.direction(state.stepping)),
        qOverP(stepper.qOverP(state.stepping)),
        absQ(stepper.particleHypothesis(state.stepping).absoluteCharge()),
        momentum(stepper.absoluteMomentum(state.stepping)),
        mass(stepper.particleHypothesis(state.stepping).mass()),
        absPdg(stepper.particleHypothesis(state.stepping).absolutePdg()),
        performCovarianceTransport(state.stepping.covTransport),
        navDir(state.options.direction) {}

  /// @brief This function evaluates the material properties to interact with
  ///
  /// @tparam propagator_state_t Type of the propagator state
  /// @tparam navigator_t Type of the propagator state
  ///
  /// @param [in] state State of the propagation
  /// @param [in] navigator Navigator of the propagation
  ///
  /// @return Boolean statement whether the material is valid
  template <typename propagator_state_t, typename navigator_t>
  bool evaluateMaterialSlab(const propagator_state_t& state,
                            const navigator_t& navigator) {
    pathCorrection = 0;
    if (navigator.currentVolume(state.navigation) != nullptr &&
        navigator.currentVolume(state.navigation)->volumeMaterial() !=
            nullptr) {
      slab = MaterialSlab(navigator.currentVolume(state.navigation)
                              ->volumeMaterial()
                              ->material(pos),
                          1);  // state.stepping.StepSize
    } else {
      slab = MaterialSlab();
    }
    return slab.isValid();
  }
};

}  // namespace Acts::detail
