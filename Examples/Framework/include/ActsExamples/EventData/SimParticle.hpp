// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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

  const SimParticleState& initial() const { return m_initial; }
  const SimParticleState& final() const { return m_final; }

  SimParticleState& initial() { return m_initial; }
  SimParticleState& final() { return m_final; }

  /// Construct a new particle with a new identifier but same kinematics.
  ///
  /// @note This is intentionally not a regular setter. The particle id
  ///       is used to identify the whole particle. Setting it on an existing
  ///       particle is usually a mistake.
  SimParticle withParticleId(SimBarcode particleId) const {
    return SimParticle(initial().withParticleId(particleId),
                       final().withParticleId(particleId));
  }

  /// Set the process type that generated this particle.
  SimParticle& setProcess(ActsFatras::ProcessType proc) {
    initial().setProcess(proc);
    final().setProcess(proc);
    return *this;
  }
  /// Set the pdg.
  SimParticle& setPdg(Acts::PdgParticle pdg) {
    initial().setPdg(pdg);
    final().setPdg(pdg);
    return *this;
  }
  /// Set the charge.
  SimParticle& setCharge(double charge) {
    initial().setCharge(charge);
    final().setCharge(charge);
    return *this;
  }
  /// Set the mass.
  SimParticle& setMass(double mass) {
    initial().setMass(mass);
    final().setMass(mass);
    return *this;
  }
  /// Set the particle ID.
  SimParticle& setParticleId(SimBarcode barcode) {
    initial().setParticleId(barcode);
    final().setParticleId(barcode);
    return *this;
  }

  /// Particle identifier within an event.
  SimBarcode particleId() const { return initial().particleId(); }
  /// Which type of process generated this particle.
  ActsFatras::ProcessType process() const { return initial().process(); }
  /// PDG particle number that identifies the type.
  Acts::PdgParticle pdg() const { return initial().pdg(); }
  /// Absolute PDG particle number that identifies the type.
  Acts::PdgParticle absolutePdg() const { return initial().absolutePdg(); }
  /// Particle charge.
  double charge() const { return initial().charge(); }
  /// Particle absolute charge.
  double absoluteCharge() const { return initial().absoluteCharge(); }
  /// Particle mass.
  double mass() const { return initial().mass(); }

  /// Check if this is a secondary particle.
  bool isSecondary() const { return initial().isSecondary(); }

  /// Particle hypothesis.
  Acts::ParticleHypothesis hypothesis() const { return initial().hypothesis(); }
  /// Particl qOverP.
  double qOverP() const { return initial().qOverP(); }

  /// Space-time position four-vector.
  const Acts::Vector4& fourPosition() const { return initial().fourPosition(); }
  /// Three-position, i.e. spatial coordinates without the time.
  auto position() const { return initial().position(); }
  /// Time coordinate.
  double time() const { return initial().time(); }
  /// Energy-momentum four-vector.
  Acts::Vector4 fourMomentum() const { return initial().fourMomentum(); }
  /// Unit three-direction, i.e. the normalized momentum three-vector.
  const Acts::Vector3& direction() const { return initial().direction(); }
  /// Polar angle.
  double theta() const { return initial().theta(); }
  /// Azimuthal angle.
  double phi() const { return initial().phi(); }
  /// Absolute momentum in the x-y plane.
  double transverseMomentum() const { return initial().transverseMomentum(); }
  /// Absolute momentum.
  double absoluteMomentum() const { return initial().absoluteMomentum(); }
  /// Absolute momentum.
  Acts::Vector3 momentum() const { return initial().momentum(); }
  /// Total energy, i.e. norm of the four-momentum.
  double energy() const { return initial().energy(); }

  /// Energy loss over the particles lifetime or simulation time.
  double energyLoss() const { return initial().energy() - final().energy(); }

  /// Accumulated path within material measured in radiation lengths.
  double pathInX0() const { return final().pathInX0(); }
  /// Accumulated path within material measured in interaction lengths.
  double pathInL0() const { return final().pathInL0(); }

  /// Number of hits.
  std::uint32_t numberOfHits() const { return final().numberOfHits(); }

  /// Particle outcome.
  ActsFatras::ParticleOutcome outcome() const { return final().outcome(); }

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
