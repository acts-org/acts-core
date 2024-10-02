// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Material/MaterialSlab.hpp"

#include <cmath>

namespace Acts {

/// Compute the mean energy loss due to ionisation and excitation.
///
/// @param slab      The traversed material and its properties
/// @param m         Particle mass
/// @param qOverP    Particle charge divided by absolute momentum
/// @param absQ      Absolute particle charge
///
/// This computes the mean energy loss -dE(x) through a material with
/// the given properties, i.e. it computes
///
///     -dE(x) = -dE/dx * x
///
/// where -dE/dx is given by the Bethe formula. The computations are valid
/// for intermediate particle energies.
float computeEnergyLossBethe(const MaterialSlab& slab, float m, float qOverP,
                             float absQ);
/// Derivative of the Bethe energy loss with respect to q/p.
///
/// @copydoc computeEnergyLossBethe
float deriveEnergyLossBetheQOverP(const MaterialSlab& slab, float m,
                                  float qOverP, float absQ);

/// Compute the most propable energy loss due to ionisation and excitation.
///
/// @copydoc computeEnergyLossBethe
///
/// This computes the most probable energy loss -dE(x) through a material of
/// the given properties and thickness as described by the mode of the
/// Landau-Vavilov-Bichsel distribution. The computations are valid
/// for intermediate particle energies.
float computeEnergyLossLandau(const MaterialSlab& slab, float m, float qOverP,
                              float absQ);
/// Derivative of the most probable ionisation energy loss with respect to q/p.
///
/// @copydoc computeEnergyLossBethe
float deriveEnergyLossLandauQOverP(const MaterialSlab& slab, float m,
                                   float qOverP, float absQ);

/// Compute the Gaussian-equivalent sigma for the ionisation loss fluctuations.
///
/// @see computeEnergyLossBethe for parameters description
///
/// This is the sigma parameter of a Gaussian distribution with the same
/// full-width-half-maximum as the Landau-Vavilov-Bichsel distribution. The
/// computations are valid for intermediate particle energies.
float computeEnergyLossLandauSigma(const MaterialSlab& slab, float m,
                                   float qOverP, float absQ);

/// Compute the full with half maximum of landau energy loss distribution
///
/// @see computeEnergyLossBethe for parameters description
float computeEnergyLossLandauFwhm(const MaterialSlab& slab, float m,
                                  float qOverP, float absQ);

/// Compute q/p Gaussian-equivalent sigma due to ionisation loss fluctuations.
///
/// @copydoc computeEnergyLossBethe
float computeEnergyLossLandauSigmaQOverP(const MaterialSlab& slab, float m,
                                         float qOverP, float absQ);

/// Compute the mean energy loss due to radiative effects at high energies.
///
/// @param slab      The traversed material and its properties
/// @param absPdg    Absolute particle type PDG identifier
/// @param m         Particle mass
/// @param qOverP    Particle charge divided by absolute momentum
/// @param absQ      Absolute particle charge
///
/// This computes the mean energy loss -dE(x) using an approximative formula.
/// Bremsstrahlung is always included; direct e+e- pair production and
/// photo-nuclear interactions only for muons.
float computeEnergyLossRadiative(const MaterialSlab& slab, PdgParticle absPdg,
                                 float m, float qOverP, float absQ);
/// Derivative of the mean radiative energy loss with respect to q/p.
///
/// @copydoc computeEnergyLossRadiative
float deriveEnergyLossRadiativeQOverP(const MaterialSlab& slab,
                                      PdgParticle absPdg, float m, float qOverP,
                                      float absQ);

/// Compute the combined mean energy loss.
///
/// @param slab      The traversed material and its properties
/// @param absPdg    Absolute particle type PDG identifier
/// @param m         Particle mass
/// @param qOverP    Particle charge divided by absolute momentum
/// @param absQ      Absolute particle charge
///
/// This computes the combined mean energy loss -dE(x) including ionisation and
/// radiative effects. The computations are valid over a wide range of particle
/// energies.
float computeEnergyLossMean(const MaterialSlab& slab, PdgParticle absPdg,
                            float m, float qOverP, float absQ);
/// Derivative of the combined mean energy loss with respect to q/p.
///
/// @copydoc computeEnergyLossMean
float deriveEnergyLossMeanQOverP(const MaterialSlab& slab, PdgParticle absPdg,
                                 float m, float qOverP, float absQ);

/// Compute the combined most probably energy loss.
///
/// @copydoc computeEnergyLossMean
float computeEnergyLossMode(const MaterialSlab& slab, PdgParticle absPdg,
                            float m, float qOverP, float absQ);
/// Derivative of the combined most probable energy loss with respect to q/p.
///
/// @copydoc computeEnergyLossMean
float deriveEnergyLossModeQOverP(const MaterialSlab& slab, PdgParticle absPdg,
                                 float m, float qOverP, float absQ);

/// Compute the core width of the projected planar scattering distribution.
///
/// @param slab      The traversed material and its properties
/// @param absPdg    Absolute particle type PDG identifier
/// @param m         Particle mass
/// @param qOverP    Particle charge divided by absolute momentum
/// @param absQ      Absolute particle charge
float computeMultipleScatteringTheta0(const MaterialSlab& slab,
                                      PdgParticle absPdg, float m, float qOverP,
                                      float absQ);

}  // namespace Acts
