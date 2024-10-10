// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Detector/GeometryCompatibilityConcept.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/detail/PointwiseMaterialInteraction.hpp"
#include "Acts/Propagator/detail/VolumeMaterialInteraction.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <sstream>

namespace Acts {

/// Material interactor propagator action.
///
/// Apply material interactions at a surface and update the track state.
struct MaterialInteractor {
  /// Whether to consider multiple scattering.
  bool multipleScattering = true;
  /// Whether to consider energy loss.
  bool energyLoss = true;
  /// Whether to record all material interactions.
  bool recordInteractions = false;
  /// Whether to add or remove noise.
  NoiseUpdateMode noiseUpdateMode = NoiseUpdateMode::addNoise;

  using result_type = RecordedMaterial;

  /// @brief Interaction with detector material for the ActionList
  /// of the Propagator
  ///
  /// It checks if the state has a current surface, in which case
  /// the action is performed: the covariance is transported to the position,
  /// multiple scattering and energy loss is applied  according to the
  /// configuration.
  ///
  /// @tparam propagator_state_t is the type of Propagator state
  /// @tparam stepper_t Type of the stepper of the propagation
  /// @tparam navigator_t Type of the navigator of the propagation
  ///
  /// @param state is the mutable propagator state object
  /// @param stepper The stepper in use
  /// @param navigator The navigator in use
  /// @param result is the mutable result state object
  /// @param logger a logger instance
  template <typename propagator_state_t, typename stepper_t,
            typename navigator_t>
  void act(propagator_state_t& state, const stepper_t& stepper,
           const navigator_t& navigator, result_type& result,
           const Logger& logger) const {
    if (state.stage == PropagatorStage::postPropagation) {
      return;
    }

    // Do nothing if nothing is what is requested.
    if (!(multipleScattering || energyLoss || recordInteractions)) {
      return;
    }

    static_assert(
        Acts::NavigationCompatibleConcept<propagator_state_t, navigator_t>,
        "Navigation does not fulfill geometry compatibility concept");

    // Handle surface material

    // Note that start and target surface conditions are handled in the
    // interaction code
    const Surface* surface = navigator.currentSurface(state.navigation);

    // We only have material interactions if there is potential material
    if (surface && surface->surfaceMaterial()) {
      ACTS_VERBOSE("MaterialInteractor | " << "Found material on surface "
                                           << surface->geometryId());

      // Prepare relevant input particle properties
      detail::PointwiseMaterialInteraction interaction(surface, state, stepper);

      // Determine the effective traversed material and its properties
      // Material exists but it's not real, i.e. vacuum; there is nothing to do
      if (interaction.evaluateMaterialSlab(state, navigator)) {
        // Evaluate the material effects
        interaction.evaluatePointwiseMaterialInteraction(multipleScattering,
                                                         energyLoss);

        if (energyLoss) {
          using namespace UnitLiterals;
          ACTS_VERBOSE("MaterialInteractor | "
                       << interaction.slab << " absPdg=" << interaction.absPdg
                       << " mass=" << interaction.mass / 1_MeV << "MeV"
                       << " momentum=" << interaction.momentum / 1_GeV << "GeV"
                       << " energyloss=" << interaction.Eloss / 1_MeV << "MeV");
        }

        // To integrate process noise, we need to transport
        // the covariance to the current position in space
        if (interaction.performCovarianceTransport) {
          stepper.transportCovarianceToCurvilinear(state.stepping);
        }
        // Apply the material interactions
        interaction.updateState(state, stepper, noiseUpdateMode);

        // Record the result
        recordResult(interaction, result);
      }
    }

    // Handle volume material

    // In case of Volume material update the result of the previous step
    if (!result.materialInteractions.empty() &&
        !result.materialInteractions.back().volume.empty() &&
        result.materialInteractions.back().updatedVolumeStep == false) {
      updateResult(state, stepper, result);
    }

    auto volume = navigator.currentVolume(state.navigation);

    // We only have material interactions if there is potential material
    if (volume && volume->volumeMaterial()) {
      ACTS_VERBOSE("MaterialInteractor | " << "Found material in volume "
                                           << volume->geometryId());

      // Prepare relevant input particle properties
      detail::VolumeMaterialInteraction interaction(volume, state, stepper);
      // Determine the effective traversed material and its properties
      // Material exists but it's not real, i.e. vacuum; there is nothing to do
      if (interaction.evaluateMaterialSlab(state, navigator)) {
        // Record the result
        recordResult(interaction, result);
      }
    }
  }

 private:
  /// @brief This function records the material effect
  ///
  /// @param [in] interaction Interaction cache container
  /// @param [in, out] result Result storage
  void recordResult(const detail::PointwiseMaterialInteraction& interaction,
                    result_type& result) const {
    result.materialInX0 += interaction.slab.thicknessInX0();
    result.materialInL0 += interaction.slab.thicknessInL0();

    // Record the interaction if requested
    if (!recordInteractions) {
      return;
    }

    MaterialInteraction mi;
    mi.position = interaction.pos;
    mi.time = interaction.time;
    mi.direction = interaction.dir;
    mi.deltaP = interaction.nextP - interaction.momentum;
    mi.sigmaPhi2 = interaction.variancePhi;
    mi.sigmaTheta2 = interaction.varianceTheta;
    mi.sigmaQoP2 = interaction.varianceQoverP;
    mi.surface = interaction.surface;
    mi.volume = InteractionVolume();
    mi.pathCorrection = interaction.pathCorrection;
    mi.materialSlab = interaction.slab;
    result.materialInteractions.push_back(std::move(mi));
  }

  /// @brief This function records the material effect
  ///
  /// @param [in] interaction Interaction cache container
  /// @param [in, out] result Result storage
  void recordResult(const detail::VolumeMaterialInteraction& interaction,
                    result_type& result) const {
    // Record the interaction if requested
    if (!recordInteractions) {
      return;
    }

    MaterialInteraction mi;
    mi.position = interaction.pos;
    mi.time = interaction.time;
    mi.direction = interaction.dir;
    mi.surface = nullptr;
    mi.volume = interaction.volume;
    mi.pathCorrection = interaction.pathCorrection;
    mi.materialSlab = interaction.slab;
    result.materialInteractions.push_back(std::move(mi));
  }

  /// @brief This function update the previous material step
  ///
  /// @param [in,out] state The state object
  /// @param [in] stepper The stepper instance
  /// @param [in, out] result Result storage
  template <typename propagator_state_t, typename stepper_t>
  void updateResult(propagator_state_t& state, const stepper_t& stepper,
                    result_type& result) const {
    // Update the previous interaction if requested
    if (!recordInteractions) {
      return;
    }

    Vector3 shift = stepper.position(state.stepping) -
                    result.materialInteractions.back().position;
    double momentum = stepper.direction(state.stepping).norm();
    result.materialInteractions.back().deltaP =
        momentum - result.materialInteractions.back().direction.norm();
    result.materialInteractions.back().materialSlab.scaleThickness(
        shift.norm());
    result.materialInteractions.back().updatedVolumeStep = true;
    result.materialInX0 +=
        result.materialInteractions.back().materialSlab.thicknessInX0();
    result.materialInL0 +=
        result.materialInteractions.back().materialSlab.thicknessInL0();
  }
};

}  // namespace Acts
