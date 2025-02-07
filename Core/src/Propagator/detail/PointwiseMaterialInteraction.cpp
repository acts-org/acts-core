// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
