// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Generators/ParametricParticleGenerator.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/ParticleData.hpp"
#include "Acts/Utilities/AngleHelpers.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsFatras/EventData/Barcode.hpp"

#include <limits>
#include <utility>

namespace ActsExamples {

ParametricParticleGenerator::ParametricParticleGenerator(const Config& cfg)
    : m_cfg(cfg),
      m_charge(cfg.charge.value_or(Acts::findCharge(m_cfg.pdg).value_or(0))),
      m_mass(cfg.mass.value_or(Acts::findMass(m_cfg.pdg).value_or(0))) {
  m_pdgChoices = {
      m_cfg.pdg,
      static_cast<Acts::PdgParticle>(-m_cfg.pdg),
  };
  m_qChoices = {
      m_charge,
      -m_charge,
  };

  // choose between particle/anti-particle if requested
  // the upper limit of the distribution is inclusive
  m_particleTypeChoice = UniformIndex(0u, m_cfg.randomizeCharge ? 1u : 0u);
  m_phiDist = UniformReal(m_cfg.phiMin, m_cfg.phiMax);

  if (m_cfg.etaUniform) {
    double etaMin = Acts::AngleHelpers::etaFromTheta(m_cfg.thetaMin);
    double etaMax = Acts::AngleHelpers::etaFromTheta(m_cfg.thetaMax);

    UniformReal etaDist(etaMin, etaMax);

    m_sinCosThetaDist =
        [=](RandomEngine& rng) mutable -> std::pair<double, double> {
      const double eta = etaDist(rng);
      const double theta = Acts::AngleHelpers::thetaFromEta(eta);
      return {std::sin(theta), std::cos(theta)};
    };
  } else {
    // since we want to draw the direction uniform on the unit sphere, we must
    // draw from cos(theta) instead of theta. see e.g.
    // https://mathworld.wolfram.com/SpherePointPicking.html
    double cosThetaMin = std::cos(m_cfg.thetaMin);
    // ensure upper bound is included. see e.g.
    // https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
    double cosThetaMax = std::nextafter(std::cos(m_cfg.thetaMax),
                                        std::numeric_limits<double>::max());

    UniformReal cosThetaDist(cosThetaMin, cosThetaMax);

    m_sinCosThetaDist =
        [=](RandomEngine& rng) mutable -> std::pair<double, double> {
      const double cosTheta = cosThetaDist(rng);
      return {std::sqrt(1 - cosTheta * cosTheta), cosTheta};
    };
  }

  if (m_cfg.pLogUniform) {
    // distributes p or pt uniformly in log space
    UniformReal dist(std::log(m_cfg.pMin), std::log(m_cfg.pMax));

    m_somePDist = [=](RandomEngine& rng) mutable -> double {
      return std::exp(dist(rng));
    };
  } else {
    // distributes p or pt uniformly
    UniformReal dist(m_cfg.pMin, m_cfg.pMax);

    m_somePDist = [=](RandomEngine& rng) mutable -> double {
      return dist(rng);
    };
  }
}

std::pair<SimVertexContainer, SimParticleContainer>
ParametricParticleGenerator::operator()(RandomEngine& rng) {
  SimVertexContainer::sequence_type vertices;
  SimParticleContainer::sequence_type particles;

  // create the primary vertex
  auto& primaryVertex =
      vertices.emplace_back(SimVertexBarcode{0}, Acts::Vector4(0., 0., 0., 0.));

  // counter will be reused as barcode particle number which must be non-zero.
  for (std::size_t ip = 1; ip <= m_cfg.numParticles; ++ip) {
    // all particles are treated as originating from the same primary vertex
    const auto pid = SimBarcode(0u).setParticle(ip);
    primaryVertex.outgoing.insert(pid);

    // draw parameters
    const unsigned int type = m_particleTypeChoice(rng);
    const Acts::PdgParticle pdg = m_pdgChoices[type];
    const double q = m_qChoices[type];
    const double phi = m_phiDist(rng);
    const double someP = m_somePDist(rng);

    const auto [sinTheta, cosTheta] = m_sinCosThetaDist(rng);
    // we already have sin/cos theta. they can be used directly
    const Acts::Vector3 dir = {sinTheta * std::cos(phi),
                               sinTheta * std::sin(phi), cosTheta};

    const double p = someP * (m_cfg.pTransverse ? 1. / sinTheta : 1.);

    // construct the particle;
    SimParticleState particle(pid, pdg, q, m_mass);
    particle.setDirection(dir);
    particle.setAbsoluteMomentum(p);

    // generated particle ids are already ordered and should end up at the end
    particles.insert(particles.end(), SimParticle(particle, particle));
  }

  std::pair<SimVertexContainer, SimParticleContainer> out;
  out.first.insert(vertices.begin(), vertices.end());
  out.second.insert(particles.begin(), particles.end());
  return out;
}

}  // namespace ActsExamples
