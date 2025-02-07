// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsFatras/Physics/StandardInteractions.hpp"

ActsFatras::StandardChargedElectroMagneticInteractions
ActsFatras::makeStandardChargedElectroMagneticInteractions(
    double minimumAbsMomentum) {
  StandardChargedElectroMagneticInteractions pl;
  pl.get<detail::StandardBetheBloch>().selectOutputParticle.valMin =
      minimumAbsMomentum;
  pl.get<detail::StandardBetheHeitler>().selectOutputParticle.valMin =
      minimumAbsMomentum;
  pl.get<detail::StandardBetheHeitler>().selectChildParticle.valMin =
      minimumAbsMomentum;
  return pl;
}
