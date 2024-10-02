// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Material/Interactions.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Utilities/LandauDistribution.hpp"

#include <array>

namespace ActsFatras {

/// Simulate energy loss using the Bethe-Bloch/Landau description.
///
/// Energy loss is computed using the most probable value and appropriate
/// fluctuations from a Landau distribution. No secondaries are generated
/// for the removed energy.
struct BetheBloch {
  /// Scaling for most probable value
  double scaleFactorMPV = 1.;
  /// Scaling for Sigma
  double scaleFactorSigma = 1.;

  /// Simulate energy loss and update the particle parameters.
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
    // compute energy loss distribution parameters
    const float m = particle.mass();
    const float qOverP = particle.qOverP();
    const float absQ = particle.absoluteCharge();
    // most probable value
    const float energyLoss =
        Acts::computeEnergyLossLandau(slab, m, qOverP, absQ);
    // Gaussian-equivalent sigma
    const float energyLossSigma =
        Acts::computeEnergyLossLandauSigma(slab, m, qOverP, absQ);

    // Simulate the energy loss
    // TODO landau location and scale parameters are not identical to the most
    //      probable value and the Gaussian-equivalent sigma
    LandauDistribution lossDistribution(scaleFactorMPV * energyLoss,
                                        scaleFactorSigma * energyLossSigma);
    const auto loss = lossDistribution(generator);

    // Apply the energy loss
    particle.correctEnergy(-loss);

    // Generates no new particles
    return {};
  }
};

}  // namespace ActsFatras
