// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/PdgParticle.hpp"
#include "ActsFatras/Kernel/ContinuousProcess.hpp"
#include "ActsFatras/Kernel/InteractionList.hpp"
#include "ActsFatras/Physics/ElectroMagnetic/BetheBloch.hpp"
#include "ActsFatras/Physics/ElectroMagnetic/BetheHeitler.hpp"
#include "ActsFatras/Physics/ElectroMagnetic/Scattering.hpp"
#include "ActsFatras/Selectors/KinematicCasts.hpp"
#include "ActsFatras/Selectors/ParticleSelectors.hpp"
#include "ActsFatras/Selectors/SelectorHelpers.hpp"

namespace ActsFatras {
namespace Casts {
struct P;
}  // namespace Casts

namespace detail {

/// Select electrons and positrons only.
using SelectElectronLike = AbsPdgSelector<Acts::PdgParticle::eElectron>;
/// Select particles above a minimum absolute momentum.
using SelectPMin = Min<Casts::P>;

/// Highland multiple scattering that applies to all charged particles.
using StandardScattering =
    ContinuousProcess<HighlandScattering, ChargedSelector, EveryParticle,
                      EveryParticle>;
/// Ionisation/excitation energy loss with a lower p cut on output particles.
///
/// Bethe-Bloch generates no particles and the child selector has no effect.
using StandardBetheBloch =
    ContinuousProcess<BetheBloch, ChargedSelector, SelectPMin, EveryParticle>;
/// Electron Bremsstrahlung energy loss with a lower p cut on output particles.
///
/// Only applies to electrons and positrons.
using StandardBetheHeitler =
    ContinuousProcess<BetheHeitler, SelectElectronLike, SelectPMin, SelectPMin>;

}  // namespace detail

/// Standard set of electro-magnetic interactions for charged particles.
///
/// Scattering must come first so it is computed with the unmodified initial
/// energy before energy loss is applied.
///
/// @warning The list has no cuts on input particle charge or kinematics, i.e.
///          it relies on the simulator to preselect relevant charged particles
///          before application.
/// @todo Bethe-Bloch does not describe electrons; add correct ionisation loss
///       descriptions for electrons.
/// @todo Bethe-Heitler is applied after energy loss and thus sees the wrong
///       input energy.
using StandardChargedElectroMagneticInteractions =
    InteractionList<detail::StandardScattering, detail::StandardBetheBloch,
                    detail::StandardBetheHeitler>;

/// Construct the standard electro-magnetic interactions for charged particles.
///
/// @param minimumAbsMomentum lower p cut on output particles
StandardChargedElectroMagneticInteractions
makeStandardChargedElectroMagneticInteractions(double minimumAbsMomentum);

}  // namespace ActsFatras
