// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsFatras/Physics/NuclearInteraction/NuclearInteraction.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>

namespace ActsFatras {

const detail::NuclearInteractionParameters& NuclearInteraction::findParameters(
    double rnd,
    const detail::NuclearInteractionParametrisation& parametrisation,
    float particleMomentum) const {
  // Return lowest/highest if momentum outside the boundary
  if (particleMomentum <= parametrisation.front().first) {
    return parametrisation.front().second;
  }
  if (particleMomentum >= parametrisation.back().first) {
    return parametrisation.back().second;
  }

  // Find the two neighbouring parametrisations
  const auto lowerBound = std::lower_bound(
      parametrisation.begin(), parametrisation.end(), particleMomentum,
      [](const std::pair<const float,
                         ActsFatras::detail::NuclearInteractionParameters>&
             params,
         const float mom) { return params.first < mom; });
  const float momentumUpperNeighbour = lowerBound->first;
  const float momentumLowerNeighbour = std::prev(lowerBound, 1)->first;

  // Pick one randomly
  const float weight = (momentumUpperNeighbour - particleMomentum) /
                       (momentumUpperNeighbour - momentumLowerNeighbour);
  return (rnd < weight) ? std::prev(lowerBound, 1)->second : lowerBound->second;
}  // namespace ActsFatras

unsigned int NuclearInteraction::sampleDiscreteValues(
    double rnd,
    const detail::NuclearInteractionParameters::CumulativeDistribution&
        distribution) const {
  // Fast exit
  if (distribution.second.empty()) {
    return 0;
  }

  // Find the bin
  const std::uint32_t int_rnd = static_cast<std::uint32_t>(
      std::numeric_limits<std::uint32_t>::max() * rnd);
  const auto it = std::upper_bound(distribution.second.begin(),
                                   distribution.second.end(), int_rnd);
  std::size_t iBin = std::min(
      static_cast<std::size_t>(std::distance(distribution.second.begin(), it)),
      distribution.second.size() - 1);

  // Return the corresponding bin
  return static_cast<unsigned int>(distribution.first[iBin]);
}

Particle::Scalar NuclearInteraction::sampleContinuousValues(
    double rnd,
    const detail::NuclearInteractionParameters::CumulativeDistribution&
        distribution,
    bool interpolate) const {
  // Fast exit
  if (distribution.second.empty()) {
    return std::numeric_limits<Scalar>::infinity();
  }

  // Find the bin
  const std::uint32_t int_rnd = static_cast<std::uint32_t>(
      std::numeric_limits<std::uint32_t>::max() * rnd);
  // Fast exit for non-normalised CDFs like interaction probability
  if (int_rnd > distribution.second.back()) {
    return std::numeric_limits<Scalar>::infinity();
  }
  const auto it = std::upper_bound(distribution.second.begin(),
                                   distribution.second.end(), int_rnd);
  std::size_t iBin = std::min(
      static_cast<std::size_t>(std::distance(distribution.second.begin(), it)),
      distribution.second.size() - 1);

  if (interpolate) {
    // Interpolate between neighbouring bins and return a diced intermediate
    // value
    const std::uint32_t basecont =
        (iBin > 0 ? distribution.second[iBin - 1] : 0);
    const std::uint32_t dcont = distribution.second[iBin] - basecont;
    return distribution.first[iBin] +
           (distribution.first[iBin + 1] - distribution.first[iBin]) *
               (dcont > 0 ? (int_rnd - basecont) / dcont : 0.5);
  } else {
    return distribution.first[iBin];
  }
}

unsigned int NuclearInteraction::finalStateMultiplicity(
    double rnd,
    const detail::NuclearInteractionParameters::CumulativeDistribution&
        distribution) const {
  return sampleDiscreteValues(rnd, distribution);
}

std::pair<ActsFatras::Particle::Scalar, ActsFatras::Particle::Scalar>
NuclearInteraction::globalAngle(ActsFatras::Particle::Scalar phi1,
                                ActsFatras::Particle::Scalar theta1, float phi2,
                                float theta2) const {
  // Rotation around the global y-axis
  Acts::SquareMatrix3 rotY = Acts::SquareMatrix3::Zero();
  rotY(0, 0) = std::cos(theta1);
  rotY(0, 2) = std::sin(theta1);
  rotY(1, 1) = 1.;
  rotY(2, 0) = -std::sin(theta1);
  rotY(2, 2) = std::cos(theta1);

  // Rotation around the global z-axis
  Acts::SquareMatrix3 rotZ = Acts::SquareMatrix3::Zero();
  rotZ(0, 0) = std::cos(phi1);
  rotZ(0, 1) = -std::sin(phi1);
  rotZ(1, 0) = std::sin(phi1);
  rotZ(1, 1) = std::cos(phi1);
  rotZ(2, 2) = 1.;

  // Rotate the direction vector of the second particle
  const Acts::Vector3 vector2(std::sin(theta2) * std::cos(phi2),
                              std::sin(theta2) * std::sin(phi2),
                              std::cos(theta2));
  const Acts::Vector3 vectorSum = rotZ * rotY * vector2;

  // Calculate the global angles
  const float theta = std::acos(vectorSum.z() / vectorSum.norm());
  const float phi = std::atan2(vectorSum.y(), vectorSum.x());

  return std::make_pair(phi, theta);
}

bool NuclearInteraction::match(const Acts::ActsDynamicVector& momenta,
                               const Acts::ActsDynamicVector& invariantMasses,
                               float parametrizedMomentum) const {
  const unsigned int size = momenta.size();
  for (unsigned int i = 0; i < size; i++) {
    // Calculate the invariant masses
    const float momentum = momenta[i];
    const float invariantMass = invariantMasses[i];
    const float p1p2 = 2. * momentum * parametrizedMomentum;
    const float costheta = 1. - invariantMass * invariantMass / p1p2;

    // Abort if an angle cannot be calculated
    if (std::abs(costheta) > 1) {
      return false;
    }
  }
  return true;
}
}  // namespace ActsFatras
