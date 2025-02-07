// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Material/Interactions.hpp"

#include <numbers>
#include <random>

namespace ActsFatras::detail {

/// Generate scattering angles using the Highland/PDG parametrization.
///
/// The angles are drawn from a single normal distribution with a width
/// given by the Highland/PDG formula.
struct Highland {
  /// Generate a single 3D scattering angle.
  ///
  /// @param[in]     generator is the random number generator
  /// @param[in]     slab      defines the passed material
  /// @param[in,out] particle  is the particle being scattered
  /// @return a 3d scattering angle
  ///
  /// @tparam generator_t is a RandomNumberEngine
  template <typename generator_t>
  double operator()(generator_t &generator, const Acts::MaterialSlab &slab,
                    Particle &particle) const {
    // compute the planar scattering angle
    const auto theta0 = Acts::computeMultipleScatteringTheta0(
        slab, particle.absolutePdg(), particle.mass(), particle.qOverP(),
        particle.absoluteCharge());
    // draw from the normal distribution representing the 3d angle distribution
    return std::normal_distribution<double>(
        0., std::numbers::sqrt2 * theta0)(generator);
  }
};

}  // namespace ActsFatras::detail
