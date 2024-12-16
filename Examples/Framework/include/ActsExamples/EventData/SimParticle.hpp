// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Utilities/GroupBy.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/EventData/ParticleOutcome.hpp"

#include <boost/container/flat_set.hpp>

namespace ActsExamples {

using SimBarcode = ::ActsFatras::Barcode;
using SimBarcodeContainer = ::boost::container::flat_set<SimBarcode>;

using SimParticleState = ::ActsFatras::Particle;

class SimParticle final {
 public:
  /// Construct a default particle with invalid identity.
  SimParticle() = default;

  /// Construct a particle at rest with explicit mass and charge.
  ///
  /// @param particleId Particle identifier within an event
  /// @param pdg PDG id
  /// @param charge Particle charge in native units
  /// @param mass Particle mass in native units
  ///
  /// @warning It is the users responsibility that charge and mass match
  ///          the PDG particle number.
  SimParticle(SimBarcode particleId, Acts::PdgParticle pdg, double charge,
              double mass)
      : m_initial(particleId, pdg, charge, mass),
        m_final(particleId, pdg, charge, mass) {}

  /// Construct a particle at rest from a PDG particle number.
  ///
  /// @param particleId Particle identifier within an event
  /// @param pdg PDG particle number
  ///
  /// Charge and mass are retrieved from the particle data table.
  SimParticle(SimBarcode particleId, Acts::PdgParticle pdg)
      : m_initial(particleId, pdg), m_final(particleId, pdg) {}

  SimParticle(const SimParticleState& initial, const SimParticleState& final)
      : m_initial(initial), m_final(final) {
    if (m_initial.particleId() != m_final.particleId()) {
      throw std::invalid_argument("Particle id mismatch");
    }
  }

  /// Const reference access to the initial particle state
  const SimParticleState& initialState() const { return m_initial; }
  /// Const reference access to the final particle state
  const SimParticleState& finalState() const { return m_final; }

  /// Non-const reference access to the initial particle state
  SimParticleState& initialState() { return m_initial; }

  /// Non-const reference access to the final particle state
  SimParticleState& finalState() { return m_final; }

  /// Construct a new particle with a new identifier but same kinematics.
  ///
  /// @note This is intentionally not a regular setter. The particle id
  ///       is used to identify the whole particle. Setting it on an existing
  ///       particle is usually a mistake.
  SimParticle withParticleId(SimBarcode particleId) const {
    return SimParticle(initialState().withParticleId(particleId),
                       finalState().withParticleId(particleId));
  }

  /// Set the process type that generated this particle.
  SimParticle& setProcess(ActsFatras::ProcessType proc) {
    initialState().setProcess(proc);
    finalState().setProcess(proc);
    return *this;
  }
  /// Set the pdg.
  SimParticle& setPdg(Acts::PdgParticle pdg) {
    initialState().setPdg(pdg);
    finalState().setPdg(pdg);
    return *this;
  }
  /// Set the charge.
  SimParticle& setCharge(double charge) {
    initialState().setCharge(charge);
    finalState().setCharge(charge);
    return *this;
  }
  /// Set the mass.
  SimParticle& setMass(double mass) {
    initialState().setMass(mass);
    finalState().setMass(mass);
    return *this;
  }
  /// Set the particle ID.
  SimParticle& setParticleId(SimBarcode barcode) {
    initialState().setParticleId(barcode);
    finalState().setParticleId(barcode);
    return *this;
  }

  /// Particle identifier within an event.
  SimBarcode particleId() const { return initialState().particleId(); }
  /// Which type of process generated this particle.
  ActsFatras::ProcessType process() const { return initialState().process(); }
  /// PDG particle number that identifies the type.
  Acts::PdgParticle pdg() const { return initialState().pdg(); }
  /// Absolute PDG particle number that identifies the type.
  Acts::PdgParticle absolutePdg() const { return initialState().absolutePdg(); }
  /// Particle charge.
  double charge() const { return initialState().charge(); }
  /// Particle absolute charge.
  double absoluteCharge() const { return initialState().absoluteCharge(); }
  /// Particle mass.
  double mass() const { return initialState().mass(); }

  /// Check if this is a secondary particle.
  bool isSecondary() const { return initialState().isSecondary(); }

  /// Particle hypothesis.
  Acts::ParticleHypothesis hypothesis() const {
    return initialState().hypothesis();
  }
  /// Particl qOverP.
  double qOverP() const { return initialState().qOverP(); }

  /// Space-time position four-vector.
  const Acts::Vector4& fourPosition() const {
    return initialState().fourPosition();
  }
  /// Three-position, i.e. spatial coordinates without the time.
  auto position() const { return initialState().position(); }
  /// Time coordinate.
  double time() const { return initialState().time(); }
  /// Energy-momentum four-vector.
  Acts::Vector4 fourMomentum() const { return initialState().fourMomentum(); }
  /// Unit three-direction, i.e. the normalized momentum three-vector.
  const Acts::Vector3& direction() const { return initialState().direction(); }
  /// Polar angle.
  double theta() const { return initialState().theta(); }
  /// Azimuthal angle.
  double phi() const { return initialState().phi(); }
  /// Absolute momentum in the x-y plane.
  double transverseMomentum() const {
    return initialState().transverseMomentum();
  }
  /// Absolute momentum.
  double absoluteMomentum() const { return initialState().absoluteMomentum(); }
  /// Absolute momentum.
  Acts::Vector3 momentum() const { return initialState().momentum(); }
  /// Total energy, i.e. norm of the four-momentum.
  double energy() const { return initialState().energy(); }

  /// Energy loss over the particles lifetime or simulation time.
  double energyLoss() const {
    return initialState().energy() - finalState().energy();
  }

  /// Accumulated path within material measured in radiation lengths.
  double pathInX0() const { return finalState().pathInX0(); }
  /// Accumulated path within material measured in interaction lengths.
  double pathInL0() const { return finalState().pathInL0(); }

  /// Number of hits.
  std::uint32_t numberOfHits() const { return finalState().numberOfHits(); }

  /// Particle outcome.
  ActsFatras::ParticleOutcome outcome() const { return finalState().outcome(); }

 private:
  SimParticleState m_initial;
  SimParticleState m_final;
};

std::ostream& operator<<(std::ostream& os, const SimParticle& particle);

namespace detail {
struct CompareParticleId {
  using is_transparent = void;
  bool operator()(const SimParticleState& lhs,
                  const SimParticleState& rhs) const {
    return lhs.particleId() < rhs.particleId();
  }
  bool operator()(const SimParticle& lhs, const SimParticle& rhs) const {
    return lhs.particleId() < rhs.particleId();
  }
  bool operator()(SimBarcode lhs, const SimParticleState& rhs) const {
    return lhs < rhs.particleId();
  }
  bool operator()(SimBarcode lhs, const SimParticle& rhs) const {
    return lhs < rhs.particleId();
  }
  bool operator()(const SimParticleState& lhs, SimBarcode rhs) const {
    return lhs.particleId() < rhs;
  }
  bool operator()(const SimParticle& lhs, SimBarcode rhs) const {
    return lhs.particleId() < rhs;
  }
};
struct PrimaryVertexIdGetter {
  SimBarcode operator()(const SimParticleState& particle) const {
    return SimBarcode(0u).setVertexPrimary(
        particle.particleId().vertexPrimary());
  }
  SimBarcode operator()(const SimParticle& particle) const {
    return SimBarcode(0u).setVertexPrimary(
        particle.particleId().vertexPrimary());
  }
};
struct SecondaryVertexIdGetter {
  SimBarcode operator()(const SimParticleState& particle) const {
    return SimBarcode(0u)
        .setVertexPrimary(particle.particleId().vertexPrimary())
        .setVertexSecondary(particle.particleId().vertexSecondary());
  }
  SimBarcode operator()(const SimParticle& particle) const {
    return SimBarcode(0u)
        .setVertexPrimary(particle.particleId().vertexPrimary())
        .setVertexSecondary(particle.particleId().vertexSecondary());
  }
};
}  // namespace detail

using SimParticleStateContainer =
    ::boost::container::flat_set<SimParticleState, detail::CompareParticleId>;

/// Store particles ordered by particle identifier.
using SimParticleContainer =
    ::boost::container::flat_set<SimParticle, detail::CompareParticleId>;

/// Iterate over groups of particles belonging to the same primary vertex.
inline GroupBy<SimParticleContainer::const_iterator,
               detail::PrimaryVertexIdGetter>
groupByPrimaryVertex(const SimParticleContainer& container) {
  return makeGroupBy(container, detail::PrimaryVertexIdGetter());
}

/// Iterate over groups of particles belonging to the same secondary vertex.
///
/// For each primary vertex, this yields one group of particles belonging
/// directly to the primary vertex (secondary vertex id 0) and a group for
/// each secondary vertex.
inline GroupBy<SimParticleContainer::const_iterator,
               detail::SecondaryVertexIdGetter>
groupBySecondaryVertex(const SimParticleContainer& container) {
  return makeGroupBy(container, detail::SecondaryVertexIdGetter());
}

}  // namespace ActsExamples
