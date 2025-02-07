// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsFatras/Physics/ElectroMagnetic/BetheHeitler.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/ProcessType.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

ActsFatras::Particle ActsFatras::BetheHeitler::bremPhoton(
    const Particle &particle, double gammaE, double rndPsi, double rndTheta1,
    double rndTheta2, double rndTheta3) const {
  // ------------------------------------------------------
  // simple approach
  // (a) simulate theta uniform within the opening angle of the relativistic
  // Hertz dipole
  //      theta_max = 1/gamma
  // (b)Following the Geant4 approximation from L. Urban -> encapsulate that
  // later
  //      the azimutal angle

  double psi = 2. * std::numbers::pi * rndPsi;

  // the start of the equation
  double theta = 0.;
  if (uniformHertzDipoleAngle) {
    // the simplest simulation
    theta = particle.mass() / particle.energy() * rndTheta1;
  } else {
    // ----->
    theta = particle.mass() / particle.energy();
    // follow
    constexpr double a = 0.625;  // 5/8
    double u = -log(rndTheta2 * rndTheta3) / a;
    theta *= (rndTheta1 < 0.25) ? u : u / 3.;  // 9./(9.+27) = 0.25
  }

  Acts::Vector3 particleDirection = particle.direction();
  Acts::Vector3 photonDirection = particleDirection;

  // construct the combined rotation to the scattered direction
  Acts::RotationMatrix3 rotation(
      // rotation of the scattering deflector axis relative to the reference
      Acts::AngleAxis3(psi, particleDirection) *
      // rotation by the scattering angle around the deflector axis
      Acts::AngleAxis3(theta, Acts::makeCurvilinearUnitU(particleDirection)));
  photonDirection.applyOnTheLeft(rotation);

  Particle photon(particle.particleId().makeDescendant(0),
                  Acts::PdgParticle::eGamma);
  photon.setProcess(ActsFatras::ProcessType::eBremsstrahlung)
      .setPosition4(particle.fourPosition())
      .setDirection(photonDirection)
      .setAbsoluteMomentum(gammaE)
      .setReferenceSurface(particle.referenceSurface());
  return photon;
}
