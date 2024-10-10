// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Propagator/detail/PointwiseMaterialInteraction.hpp"

#include "Acts/Material/Interactions.hpp"

namespace Acts::detail {

void PointwiseMaterialInteraction::evaluatePointwiseMaterialInteraction(
    bool multipleScattering, bool energyLoss) {
  if (energyLoss) {
    Eloss = computeEnergyLossBethe(slab, mass, qOverP, absQ);
  }
  // Compute contributions from interactions
  if (performCovarianceTransport) {
    covarianceContributions(multipleScattering, energyLoss);
  }
}

void PointwiseMaterialInteraction::covarianceContributions(
    bool multipleScattering, bool energyLoss) {
  // Compute contributions from interactions
  if (multipleScattering) {
    // TODO use momentum before or after energy loss in backward mode?
    const float theta0 =
        computeMultipleScatteringTheta0(slab, absPdg, mass, qOverP, absQ);
    // sigmaPhi = theta0 / sin(theta)
    const auto sigmaPhi = theta0 * (dir.norm() / VectorHelpers::perp(dir));
    variancePhi = sigmaPhi * sigmaPhi;
    // sigmaTheta = theta0
    varianceTheta = theta0 * theta0;
  }
  // TODO just ionisation loss or full energy loss?
  if (energyLoss) {
    const float sigmaQoverP =
        computeEnergyLossLandauSigmaQOverP(slab, mass, qOverP, absQ);
    varianceQoverP = sigmaQoverP * sigmaQoverP;
  }
}

double PointwiseMaterialInteraction::updateVariance(
    double variance, double change, NoiseUpdateMode updateMode) const {
  // Add/Subtract the change
  // Protect the variance against becoming negative
  return std::max(0., variance + std::copysign(change, updateMode));
}

}  // namespace Acts::detail
