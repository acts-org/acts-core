// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/GaussianTrackDensity.hpp"

#include "Acts/Vertexing/VertexingError.hpp"

#include <math.h>

namespace Acts {

Result<std::optional<std::pair<double, double>>>
Acts::GaussianTrackDensity::globalMaximumWithWidth(
    State& state, const std::vector<InputTrack>& trackList) const {
  auto result = addTracks(state, trackList);
  if (!result.ok()) {
    return result.error();
  }

  double maxPosition = 0.;
  double maxDensity = 0.;
  double maxSecondDerivative = 0.;

  for (const auto& track : state.trackEntries) {
    double trialZ = track.z;

    auto [density, firstDerivative, secondDerivative] =
        trackDensityAndDerivatives(state, trialZ);
    if (secondDerivative >= 0. || density <= 0.) {
      continue;
    }
    std::tie(maxPosition, maxDensity, maxSecondDerivative) =
        updateMaximum(trialZ, density, secondDerivative, maxPosition,
                      maxDensity, maxSecondDerivative);

    trialZ += stepSize(density, firstDerivative, secondDerivative);
    std::tie(density, firstDerivative, secondDerivative) =
        trackDensityAndDerivatives(state, trialZ);

    if (secondDerivative >= 0. || density <= 0.) {
      continue;
    }
    std::tie(maxPosition, maxDensity, maxSecondDerivative) =
        updateMaximum(trialZ, density, secondDerivative, maxPosition,
                      maxDensity, maxSecondDerivative);
    trialZ += stepSize(density, firstDerivative, secondDerivative);
    std::tie(density, firstDerivative, secondDerivative) =
        trackDensityAndDerivatives(state, trialZ);
    if (secondDerivative >= 0. || density <= 0.) {
      continue;
    }
    std::tie(maxPosition, maxDensity, maxSecondDerivative) =
        updateMaximum(trialZ, density, secondDerivative, maxPosition,
                      maxDensity, maxSecondDerivative);
  }

  if (maxSecondDerivative == 0.) {
    return std::nullopt;
  }

  return std::pair{maxPosition, std::sqrt(-(maxDensity / maxSecondDerivative))};
}

Result<std::optional<double>> Acts::GaussianTrackDensity::globalMaximum(
    State& state, const std::vector<InputTrack>& trackList) const {
  auto maxRes = globalMaximumWithWidth(state, trackList);
  if (!maxRes.ok()) {
    return maxRes.error();
  }
  const auto& maxOpt = *maxRes;
  if (!maxOpt.has_value()) {
    return std::nullopt;
  }
  return maxOpt->first;
}

Result<void> Acts::GaussianTrackDensity::addTracks(
    State& state, const std::vector<InputTrack>& trackList) const {
  for (auto trk : trackList) {
    const BoundTrackParameters& boundParams = m_cfg.extractParameters(trk);
    // Get required track parameters
    const double d0 = boundParams.parameters()[BoundIndices::eBoundLoc0];
    const double z0 = boundParams.parameters()[BoundIndices::eBoundLoc1];
    // Get track covariance
    if (!boundParams.covariance().has_value()) {
      return VertexingError::NoCovariance;
    }
    const auto perigeeCov = *(boundParams.covariance());
    const double covDD =
        perigeeCov(BoundIndices::eBoundLoc0, BoundIndices::eBoundLoc0);
    const double covZZ =
        perigeeCov(BoundIndices::eBoundLoc1, BoundIndices::eBoundLoc1);
    const double covDZ =
        perigeeCov(BoundIndices::eBoundLoc0, BoundIndices::eBoundLoc1);
    const double covDeterminant = (perigeeCov.block<2, 2>(0, 0)).determinant();

    // Do track selection based on track cov matrix and m_cfg.d0SignificanceCut
    if ((covDD <= 0) || (d0 * d0 / covDD > m_cfg.d0SignificanceCut) ||
        (covZZ <= 0) || (covDeterminant <= 0)) {
      continue;
    }

    // Calculate track density quantities
    double constantTerm =
        -(d0 * d0 * covZZ + z0 * z0 * covDD + 2. * d0 * z0 * covDZ) /
        (2. * covDeterminant);
    const double linearTerm =
        (d0 * covDZ + z0 * covDD) /
        covDeterminant;  // minus signs and factors of 2 cancel...
    const double quadraticTerm = -covDD / (2. * covDeterminant);
    double discriminant =
        linearTerm * linearTerm -
        4. * quadraticTerm * (constantTerm + 2. * m_cfg.z0SignificanceCut);
    if (discriminant < 0) {
      continue;
    }

    // Add the track to the current maps in the state
    discriminant = std::sqrt(discriminant);
    const double zMax = (-linearTerm - discriminant) / (2. * quadraticTerm);
    const double zMin = (-linearTerm + discriminant) / (2. * quadraticTerm);
    constantTerm -= std::log(2. * M_PI * std::sqrt(covDeterminant));

    state.trackEntries.emplace_back(z0, constantTerm, linearTerm, quadraticTerm,
                                    zMin, zMax);
  }
  return Result<void>::success();
}

std::tuple<double, double, double>
Acts::GaussianTrackDensity::trackDensityAndDerivatives(State& state,
                                                       double z) const {
  GaussianTrackDensityStore densityResult(z);
  for (const auto& trackEntry : state.trackEntries) {
    densityResult.addTrackToDensity(trackEntry);
  }
  return densityResult.densityAndDerivatives();
}

std::tuple<double, double, double> Acts::GaussianTrackDensity::updateMaximum(
    double newZ, double newValue, double newSecondDerivative, double maxZ,
    double maxValue, double maxSecondDerivative) const {
  if (newValue > maxValue) {
    maxZ = newZ;
    maxValue = newValue;
    maxSecondDerivative = newSecondDerivative;
  }
  return {maxZ, maxValue, maxSecondDerivative};
}

double Acts::GaussianTrackDensity::stepSize(double y, double dy,
                                            double ddy) const {
  return (m_cfg.isGaussianShaped ? (y * dy) / (dy * dy - y * ddy) : -dy / ddy);
}

void Acts::GaussianTrackDensity::GaussianTrackDensityStore::addTrackToDensity(
    const TrackEntry& entry) {
  // Take track only if it's within bounds
  if (entry.lowerBound < m_z && m_z < entry.upperBound) {
    double delta = std::exp(entry.c0 + m_z * (entry.c1 + m_z * entry.c2));
    double qPrime = entry.c1 + 2. * m_z * entry.c2;
    double deltaPrime = delta * qPrime;
    m_density += delta;
    m_firstDerivative += deltaPrime;
    m_secondDerivative += 2. * entry.c2 * delta + qPrime * deltaPrime;
  }
}

}  // namespace Acts
