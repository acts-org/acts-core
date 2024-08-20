// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/EventData/ProcessType.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <utility>
#include <vector>

namespace ActsFatras {

/// This class handles the photon conversion. It evaluates the distance
/// after which the interaction will occur and the final state due the
/// interaction itself.
class PhotonConversion {
 public:
  using Scalar = ActsFatras::Particle::Scalar;

  /// Scaling factor of children energy
  Scalar childEnergyScaleFactor = 2.;
  /// Scaling factor for photon conversion probability
  Scalar conversionProbScaleFactor = 0.98;

  /// Method for evaluating the distance after which the photon
  /// conversion will occur.
  ///
  /// @tparam generator_t Type of the random number generator
  /// @param [in, out] generator The random number generator
  /// @param [in] particle The particle
  ///
  /// @return valid X0 limit and no limit on L0
  template <typename generator_t>
  std::pair<Scalar, Scalar> generatePathLimits(generator_t& generator,
                                               const Particle& particle) const;

  /// This method evaluates the final state due to the photon conversion.
  ///
  /// @tparam generator_t Type of the random number generator
  /// @param [in, out] generator The random number generator
  /// @param [in, out] particle The interacting photon
  /// @param [out] generated List of generated particles
  ///
  /// @return True if the conversion occured, else false
  template <typename generator_t>
  bool run(generator_t& generator, Particle& particle,
           std::vector<Particle>& generated) const;

 private:
  /// This method constructs and returns the child particles.
  ///
  /// @param [in] photon The interacting photon
  /// @param [in] childEnergy The energy of one child particle
  /// @param [in] childDirection The direction of the child particle
  ///
  /// @return Array containing the produced leptons
  std::array<Particle, 2> generateChildren(
      const Particle& photon, Scalar childEnergy,
      const Particle::Vector3& childDirection) const;

  /// Generate the energy fraction of the first child particle.
  ///
  /// @tparam generator_t Type of the random number generator
  /// @param [in, out] generator The random number generator
  /// @param [in] gammaMom The momentum of the photon
  ///
  /// @return The energy of the child particle
  template <typename generator_t>
  Scalar generateFirstChildEnergyFraction(generator_t& generator,
                                          Scalar gammaMom) const;

  /// Generate the direction of the child particles.
  ///
  /// @tparam generator_t Type of the random number generator
  /// @param [in, out] generator The random number generator
  /// @param [in] particle The photon
  ///
  /// @return The direction vector of the child particle
  template <typename generator_t>
  Particle::Vector3 generateChildDirection(generator_t& generator,
                                           const Particle& particle) const;

  /// Helper methods for momentum evaluation
  /// @note These methods are taken from the Geant4 class
  /// G4PairProductionRelModel
  Scalar screenFunction1(Scalar delta) const;
  Scalar screenFunction2(Scalar delta) const;

  /// Electron mass. This is an static constant and not a member variable so the
  /// struct has no internal state. Otherwise, the interaction list breaks.
  static const Scalar kElectronMass;
};

inline Particle::Scalar PhotonConversion::screenFunction1(Scalar delta) const {
  // Compute the value of the screening function 3*PHI1(delta) - PHI2(delta)
  return (delta > 1.4) ? 42.038 - 8.29 * std::log(delta + 0.958)
                       : 42.184 - delta * (7.444 - 1.623 * delta);
}

inline Particle::Scalar PhotonConversion::screenFunction2(Scalar delta) const {
  // Compute the value of the screening function 1.5*PHI1(delta)
  // +0.5*PHI2(delta)
  return (delta > 1.4) ? 42.038 - 8.29 * std::log(delta + 0.958)
                       : 41.326 - delta * (5.848 - 0.902 * delta);
}

template <typename generator_t>
std::pair<Particle::Scalar, Particle::Scalar>
PhotonConversion::generatePathLimits(generator_t& generator,
                                     const Particle& particle) const {
  /// This method is based upon the Athena class PhotonConversionTool

  // Fast exit if not a photon or the energy is too low
  if (particle.pdg() != Acts::PdgParticle::eGamma ||
      particle.absoluteMomentum() < (2 * kElectronMass)) {
    return std::make_pair(std::numeric_limits<Scalar>::infinity(),
                          std::numeric_limits<Scalar>::infinity());
  }

  // Use for the moment only Al data - Yung Tsai - Rev.Mod.Particle Physics Vol.
  // 46, No.4, October 1974 optainef from a fit given in the momentum range 100
  // 10 6 2 1 0.6 0.4 0.2 0.1 GeV

  //// Quadratic background function
  //  Double_t fitFunction(Double_t *x, Double_t *par) {
  //  return par[0] + par[1]*pow(x[0],par[2]);
  // }
  // EXT PARAMETER                                   STEP         FIRST
  // NO.   NAME      VALUE            ERROR          SIZE      DERIVATIVE
  //  1  p0          -7.01612e-03   8.43478e-01   1.62766e-04   1.11914e-05
  //  2  p1           7.69040e-02   1.00059e+00   8.90718e-05  -8.41167e-07
  //  3  p2          -6.07682e-01   5.13256e+00   6.07228e-04  -9.44448e-07
  constexpr Scalar p0 = -7.01612e-03;
  constexpr Scalar p1 = 7.69040e-02;
  constexpr Scalar p2 = -6.07682e-01;

  // Calculate xi
  const Scalar xi = p0 + p1 * std::pow(particle.absoluteMomentum(), p2);

  std::uniform_real_distribution<Scalar> uniformDistribution{0., 1.};
  // This is a transformation of eq. 3.75
  return std::make_pair(-9. / 7. *
                            std::log(conversionProbScaleFactor *
                                     (1 - uniformDistribution(generator))) /
                            (1. - xi),
                        std::numeric_limits<Scalar>::infinity());
}

template <typename generator_t>
Particle::Scalar PhotonConversion::generateFirstChildEnergyFraction(
    generator_t& generator, Scalar gammaMom) const {
  /// This method is based upon the Geant4 class G4PairProductionRelModel

  /// @note This method is from the Geant4 class G4Element
  //
  //  Compute Coulomb correction factor (Phys Rev. D50 3-1 (1994) page 1254)
  constexpr Scalar k1 = 0.0083;
  constexpr Scalar k2 = 0.20206;
  constexpr Scalar k3 = 0.0020;  // This term is missing in Athena
  constexpr Scalar k4 = 0.0369;
  constexpr Scalar alphaEM = 1. / 137.;
  constexpr Scalar m_Z = 13.;  // Aluminium
  constexpr Scalar az2 = (alphaEM * m_Z) * (alphaEM * m_Z);
  constexpr Scalar az4 = az2 * az2;
  constexpr Scalar coulombFactor =
      (k1 * az4 + k2 + 1. / (1. + az2)) * az2 - (k3 * az4 + k4) * az4;

  const Scalar logZ13 = std::log(m_Z) * 1. / 3.;
  const Scalar FZ = 8. * (logZ13 + coulombFactor);
  const Scalar deltaMax = exp((42.038 - FZ) * 0.1206) - 0.958;

  const Scalar deltaPreFactor = 136. / std::pow(m_Z, 1. / 3.);
  const Scalar eps0 = kElectronMass / gammaMom;
  const Scalar deltaFactor = deltaPreFactor * eps0;
  const Scalar deltaMin = 4. * deltaFactor;

  // Compute the limits of eps
  const Scalar epsMin =
      std::max(eps0, 0.5 - 0.5 * std::sqrt(1. - deltaMin / deltaMax));
  const Scalar epsRange = 0.5 - epsMin;

  // Sample the energy rate (eps) of the created electron (or positron)
  const Scalar F10 = screenFunction1(deltaMin) - FZ;
  const Scalar F20 = screenFunction2(deltaMin) - FZ;
  const Scalar NormF1 = F10 * epsRange * epsRange;
  const Scalar NormF2 = 1.5 * F20;

  // We will need 3 uniform random number for each trial of sampling
  Scalar greject = 0.;
  Scalar eps = 0.;
  std::uniform_real_distribution<Scalar> rndmEngine;
  do {
    if (NormF1 > rndmEngine(generator) * (NormF1 + NormF2)) {
      eps = 0.5 - epsRange * std::pow(rndmEngine(generator), 1. / 3.);
      const Scalar delta = deltaFactor / (eps * (1. - eps));
      greject = (screenFunction1(delta) - FZ) / F10;
    } else {
      eps = epsMin + epsRange * rndmEngine(generator);
      const Scalar delta = deltaFactor / (eps * (1. - eps));
      greject = (screenFunction2(delta) - FZ) / F20;
    }
  } while (greject < rndmEngine(generator));
  //  End of eps sampling
  return eps * childEnergyScaleFactor;
}

template <typename generator_t>
Particle::Vector3 PhotonConversion::generateChildDirection(
    generator_t& generator, const Particle& particle) const {
  /// This method is based upon the Athena class PhotonConversionTool

  // Following the Geant4 approximation from L. Urban
  // the azimutal angle
  Scalar theta = kElectronMass / particle.energy();

  std::uniform_real_distribution<Scalar> uniformDistribution{0., 1.};
  const Scalar u = -std::log(uniformDistribution(generator) *
                             uniformDistribution(generator)) *
                   1.6;

  theta *= (uniformDistribution(generator) < 0.25)
               ? u
               : u * 1. / 3.;  // 9./(9.+27) = 0.25

  // draw the random orientation angle
  const auto psi =
      std::uniform_real_distribution<double>(-M_PI, M_PI)(generator);

  Acts::Vector3 direction = particle.direction();
  // construct the combined rotation to the scattered direction
  Acts::RotationMatrix3 rotation(
      // rotation of the scattering deflector axis relative to the reference
      Acts::AngleAxis3(psi, direction) *
      // rotation by the scattering angle around the deflector axis
      Acts::AngleAxis3(theta, Acts::makeCurvilinearUnitU(direction)));
  direction.applyOnTheLeft(rotation);
  return direction;
}

inline std::array<Particle, 2> PhotonConversion::generateChildren(
    const Particle& photon, Scalar childEnergy,
    const Particle::Vector3& childDirection) const {
  using namespace Acts::UnitLiterals;

  // Calculate the child momentum
  const Scalar massChild = kElectronMass;
  const Scalar momentum1 =
      sqrt(childEnergy * childEnergy - massChild * massChild);

  // Use energy-momentum conservation for the other child
  const Particle::Vector3 vtmp =
      photon.fourMomentum().template segment<3>(Acts::eMom0) -
      momentum1 * childDirection;
  const Scalar momentum2 = vtmp.norm();

  // The daughter particles are created with the explicit electron mass used in
  // the calculations for consistency. Using the full Particle constructor with
  // charge and mass also avoids an additional lookup in the internal data
  // tables.
  std::array<Particle, 2> children = {
      Particle(photon.particleId().makeDescendant(0), Acts::eElectron, -1_e,
               kElectronMass)
          .setPosition4(photon.fourPosition())
          .setDirection(childDirection)
          .setAbsoluteMomentum(momentum1)
          .setProcess(ProcessType::ePhotonConversion)
          .setReferenceSurface(photon.referenceSurface()),
      Particle(photon.particleId().makeDescendant(1), Acts::ePositron, 1_e,
               kElectronMass)
          .setPosition4(photon.fourPosition())
          .setDirection(childDirection)
          .setAbsoluteMomentum(momentum2)
          .setProcess(ProcessType::ePhotonConversion)
          .setReferenceSurface(photon.referenceSurface()),
  };
  return children;
}

template <typename generator_t>
bool PhotonConversion::run(generator_t& generator, Particle& particle,
                           std::vector<Particle>& generated) const {
  // Fast exit if particle is not a photon
  if (particle.pdg() != Acts::PdgParticle::eGamma) {
    return false;
  }

  // Fast exit if momentum is too low
  const Scalar p = particle.absoluteMomentum();
  if (p < (2 * kElectronMass)) {
    return false;
  }

  // Get one child energy
  const Scalar childEnergy = p * generateFirstChildEnergyFraction(generator, p);

  // Now get the deflection
  const Particle::Vector3 childDir =
      generateChildDirection(generator, particle);

  // Produce the final state
  const std::array<Particle, 2> finalState =
      generateChildren(particle, childEnergy, childDir);
  generated.insert(generated.end(), finalState.begin(), finalState.end());

  return true;
}

}  // namespace ActsFatras
