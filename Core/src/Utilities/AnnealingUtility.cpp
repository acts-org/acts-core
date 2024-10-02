// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Utilities/AnnealingUtility.hpp"

#include "Acts/Utilities/AlgebraHelpers.hpp"

namespace {
/// @brief Gaussian-like function for weight calculation
/// Note: Factor 2 in denominator is included in inverse temperature
///
/// @param chi2 Chi^2 value
/// @param invTemp Denominator 1/(2 * temperature)
///
/// @return exp(-chi2 * invTemp)
double computeAnnealingWeight(double chi2, double invTemp) {
  return Acts::safeExp(-chi2 * invTemp);
}
}  // namespace

Acts::AnnealingUtility::Config::Config() = default;

void Acts::AnnealingUtility::anneal(State& state) const {
  if (state.currentTemperatureIndex < m_cfg.setOfTemperatures.size() - 1) {
    ++state.currentTemperatureIndex;
  } else {
    state.equilibriumReached = true;
  }
}

double Acts::AnnealingUtility::getWeight(
    State& state, double chi2, const std::vector<double>& allChi2) const {
  unsigned int idx = state.currentTemperatureIndex;
  // Calculate 1/denominator in exp function already here
  const double currentInvTemp = 1. / (2. * m_cfg.setOfTemperatures[idx]);

  double num = computeAnnealingWeight(chi2, currentInvTemp);

  double denom = m_gaussCutTempVec[idx];

  for (double val : allChi2) {
    denom += computeAnnealingWeight(val, currentInvTemp);
  }

  return num / denom;
}

double Acts::AnnealingUtility::getWeight(State& state, double chi2) const {
  // Calculate 1/denominator in exp function
  const double currentInvTemp =
      1. / (2 * m_cfg.setOfTemperatures[state.currentTemperatureIndex]);

  return 1. /
         (1. + computeAnnealingWeight(m_cfg.cutOff - chi2, currentInvTemp));
}
