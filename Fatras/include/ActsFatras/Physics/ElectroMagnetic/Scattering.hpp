// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Physics/ElectroMagnetic/detail/GaussianMixture.hpp"
#include "ActsFatras/Physics/ElectroMagnetic/detail/GeneralMixture.hpp"
#include "ActsFatras/Physics/ElectroMagnetic/detail/Highland.hpp"

#include <array>
#include <numbers>
#include <random>

namespace ActsFatras {

/// Simulate (multiple) scattering using a configurable scattering model.
///
/// @tparam scattering_model_t Model implementation to draw a scattering angle.
template <typename scattering_model_t>
struct GenericScattering {
  /// The scattering formula
  scattering_model_t angle;

  /// Simulate scattering and update the particle parameters.
  ///
  /// @param[in]     generator is the random number generator
  /// @param[in]     slab      defines the passed material
  /// @param[in,out] particle  is the particle being updated
  /// @return Empty secondaries containers.
  ///
  /// @tparam generator_t is a RandomNumberEngine
  template <typename generator_t>
  std::array<Particle, 0> operator()(generator_t &generator,
                                     const Acts::MaterialSlab &slab,
                                     Particle &particle) const {
    // the scattered direction can be computed by rotating the initial
    // direction around a vector orthogonal to the initial direction, i.e. the
    // scattering deflector, by the scattering angle. there are an infinite
    // number of vectors orthogonal to the initial direction. the deflector is
    // rotated by some angle relative to some fixpoint.
    //
    // thus two random angles are required: the random deflector orientation
    // angle drawn uniformly from the [-pi,pi) range and the scattering angle
    // drawn from the specific scattering model distribution.

    // draw the random orientation angle
    const auto psi = std::uniform_real_distribution<double>(
        -std::numbers::pi, std::numbers::pi)(generator);
    // draw the scattering angle
    const auto theta = angle(generator, slab, particle);

    Acts::Vector3 direction = particle.direction();
    // construct the combined rotation to the scattered direction
    Acts::RotationMatrix3 rotation(
        // rotation of the scattering deflector axis relative to the reference
        Acts::AngleAxis3(psi, direction) *
        // rotation by the scattering angle around the deflector axis
        Acts::AngleAxis3(theta, Acts::makeCurvilinearUnitU(direction)));
    direction.applyOnTheLeft(rotation);
    particle.setDirection(direction);

    // scattering is non-destructive and produces no secondaries
    return {};
  }
};

using GaussianMixtureScattering = GenericScattering<detail::GaussianMixture>;
using GeneralMixtureScattering = GenericScattering<detail::GeneralMixture>;
using HighlandScattering = GenericScattering<detail::Highland>;

}  // namespace ActsFatras
