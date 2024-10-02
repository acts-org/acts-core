// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Generators/ParametricParticleGenerator.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/ParticleData.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <cstdint>
#include <limits>
#include <random>
#include <utility>

namespace ActsExamples {

ParametricParticleGenerator::ParametricParticleGenerator(const Config& cfg)
    : m_cfg(cfg),
      m_charge(cfg.charge.value_or(Acts::findCharge(m_cfg.pdg).value_or(0))),
      m_mass(cfg.mass.value_or(Acts::findMass(m_cfg.pdg).value_or(0))),
      // since we want to draw the direction uniform on the unit sphere, we must
      // draw from cos(theta) instead of theta. see e.g.
      // https://mathworld.wolfram.com/SpherePointPicking.html
      m_cosThetaMin(std::cos(m_cfg.thetaMin)),
      // ensure upper bound is included. see e.g.
      // https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
      m_cosThetaMax(std::nextafter(std::cos(m_cfg.thetaMax),
                                   std::numeric_limits<double>::max())),
      // in case we force uniform eta generation
      m_etaMin(-std::log(std::tan(0.5 * m_cfg.thetaMin))),
      m_etaMax(-std::log(std::tan(0.5 * m_cfg.thetaMax))) {}

std::pair<SimVertexContainer, SimParticleContainer>
ParametricParticleGenerator::operator()(RandomEngine& rng) {
  using UniformIndex = std::uniform_int_distribution<std::uint8_t>;
  using UniformReal = std::uniform_real_distribution<double>;

  // choose between particle/anti-particle if requested
  // the upper limit of the distribution is inclusive
  UniformIndex particleTypeChoice(0u, m_cfg.randomizeCharge ? 1u : 0u);
  // (anti-)particle choice is one random draw but defines two properties
  const Acts::PdgParticle pdgChoices[] = {
      m_cfg.pdg,
      static_cast<Acts::PdgParticle>(-m_cfg.pdg),
  };
  const double qChoices[] = {
      m_charge,
      -m_charge,
  };
  UniformReal phiDist(m_cfg.phiMin, m_cfg.phiMax);
  UniformReal cosThetaDist(m_cosThetaMin, m_cosThetaMax);
  UniformReal etaDist(m_etaMin, m_etaMax);
  UniformReal pDist(m_cfg.pMin, m_cfg.pMax);

  SimVertexContainer::sequence_type vertices;
  SimParticleContainer::sequence_type particles;

  // create the primary vertex
  auto& primaryVertex =
      vertices.emplace_back(0, SimVertex::Vector4(0., 0., 0., 0.));

  // counter will be reused as barcode particle number which must be non-zero.
  for (std::size_t ip = 1; ip <= m_cfg.numParticles; ++ip) {
    // all particles are treated as originating from the same primary vertex
    const auto pid = SimBarcode(0u).setParticle(ip);
    primaryVertex.outgoing.insert(pid);

    // draw parameters
    const unsigned int type = particleTypeChoice(rng);
    const Acts::PdgParticle pdg = pdgChoices[type];
    const double q = qChoices[type];
    const double phi = phiDist(rng);
    double p = pDist(rng);

    // we already have sin/cos theta. they can be used directly to
    Acts::Vector3 dir;
    double cosTheta = 0.;
    double sinTheta = 0.;
    if (!m_cfg.etaUniform) {
      cosTheta = cosThetaDist(rng);
      sinTheta = std::sqrt(1 - cosTheta * cosTheta);
    } else {
      const double eta = etaDist(rng);
      const double theta = 2 * std::atan(std::exp(-eta));
      sinTheta = std::sin(theta);
      cosTheta = std::cos(theta);
    }
    dir[Acts::eMom0] = sinTheta * std::cos(phi);
    dir[Acts::eMom1] = sinTheta * std::sin(phi);
    dir[Acts::eMom2] = cosTheta;

    // construct the particle;
    ActsFatras::Particle particle(pid, pdg, q, m_mass);
    particle.setDirection(dir);
    p *= m_cfg.pTransverse ? 1. / sinTheta : 1.;
    particle.setAbsoluteMomentum(p);

    // generated particle ids are already ordered and should end up at the end
    particles.insert(particles.end(), std::move(particle));
  }

  std::pair<SimVertexContainer, SimParticleContainer> out;
  out.first.insert(vertices.begin(), vertices.end());
  out.second.insert(particles.begin(), particles.end());
  return out;
}

}  // namespace ActsExamples
