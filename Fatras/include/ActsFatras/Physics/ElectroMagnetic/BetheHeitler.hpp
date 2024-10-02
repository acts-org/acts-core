// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <array>
#include <cmath>
#include <random>

namespace ActsFatras {

/// Simulate electron energy loss using the Bethe-Heitler description.
///
/// Bethe-Heitler for electron bremsstrahlung description as described here:
/// "A Gaussian-mixture approximation of the Bethe–Heitler model of electron
/// energy loss by bremsstrahlung" R. Frühwirth
struct BetheHeitler {
  using Scalar = Particle::Scalar;
  using Vector3 = Particle::Vector3;

  /// A scaling factor to
  double scaleFactor = 1.;

  // Simplified angle evaluation
  bool uniformHertzDipoleAngle = false;

  /// Simulate the photon emission
  ///
  /// @param [in] particle The unmodified electron
  /// @param [in] gammaE Energy of the photon
  /// @param [in] rndPsi Random number for the azimuthal angle
  /// @param [in] rndTheta1 Random number for the polar angle
  /// @param [in] rndTheta2 Random number for the polar angle
  /// @param [in] rndTheta3 Random number for the polar angle
  Particle bremPhoton(const Particle &particle, Scalar gammaE, Scalar rndPsi,
                      Scalar rndTheta1, Scalar rndTheta2,
                      Scalar rndTheta3) const;

  /// Simulate energy loss and update the particle parameters.
  ///
  /// @param[in]     generator is the random number generator
  /// @param[in]     slab      defines the passed material
  /// @param[in,out] particle  is the particle being updated
  /// @return Produced photon.
  ///
  /// @tparam generator_t is a RandomNumberEngine
  template <typename generator_t>
  std::array<Particle, 1> operator()(generator_t &generator,
                                     const Acts::MaterialSlab &slab,
                                     Particle &particle) const {
    // Take a random gamma-distributed value - depending on t/X0
    std::gamma_distribution<double> gDist(slab.thicknessInX0() / std::log(2.0),
                                          1.0);

    const auto u = gDist(generator);
    const auto z = std::exp(-u);  // MARK: fpeMask(FLTUND, 1, #2346)
    const auto sampledEnergyLoss =
        std::abs(scaleFactor * particle.energy() * (z - 1.));

    std::uniform_real_distribution<Scalar> uDist(0., 1.);
    // Build the produced photon
    Particle photon =
        bremPhoton(particle, sampledEnergyLoss, uDist(generator),
                   uDist(generator), uDist(generator), uDist(generator));
    // Recoil input momentum
    particle.setDirection(particle.direction() * particle.absoluteMomentum() -
                          photon.energy() * photon.direction());

    // apply the energy loss
    particle.correctEnergy(-sampledEnergyLoss);

    return {photon};
  }
};

}  // namespace ActsFatras
