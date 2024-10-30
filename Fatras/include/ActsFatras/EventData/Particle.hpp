// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/ParticleOutcome.hpp"
#include "ActsFatras/EventData/ProcessType.hpp"

#include <cmath>
#include <iosfwd>
#include <optional>

namespace ActsFatras {

/// Particle identity information and kinematic state.
///
/// Also stores some simulation-specific properties.
class Particle {
 public:
  using Scalar = Acts::ActsScalar;
  using Vector3 = Acts::ActsVector<3>;
  using Vector4 = Acts::ActsVector<4>;

  struct State {
    // kinematics, i.e. things that change over the particle lifetime.
    Vector4 position4 = Vector4::Zero();
    Vector3 direction = Vector3::Zero();
    Scalar absMomentum = Scalar{0};
    /// proper time in the particle rest frame
    Scalar properTime = Scalar{0};
    // accumulated material
    Scalar pathInX0 = Scalar{0};
    Scalar pathInL0 = Scalar{0};
    /// number of hits
    std::uint32_t numberOfHits = 0;

    /// Three-position, i.e. spatial coordinates without the time.
    auto position() const { return position4.segment<3>(Acts::ePos0); }
    /// Time coordinate.
    Scalar time() const { return position4[Acts::eTime]; }
    /// Energy-momentum four-vector.
    Vector4 fourMomentum(const Particle &particle) const {
      Vector4 mom4;
      // stored direction is always normalized
      mom4[Acts::eMom0] = absMomentum * direction[Acts::ePos0];
      mom4[Acts::eMom1] = absMomentum * direction[Acts::ePos1];
      mom4[Acts::eMom2] = absMomentum * direction[Acts::ePos2];
      mom4[Acts::eEnergy] = energy(particle);
      return mom4;
    }
    /// Polar angle.
    Scalar theta() const { return Acts::VectorHelpers::theta(direction); }
    /// Azimuthal angle.
    Scalar phi() const { return Acts::VectorHelpers::phi(direction); }
    /// Absolute momentum in the x-y plane.
    Scalar transverseMomentum() const {
      return absMomentum * direction.segment<2>(Acts::eMom0).norm();
    }
    /// Absolute momentum.
    Vector3 momentum() const { return absMomentum * direction; }
    /// Total energy, i.e. norm of the four-momentum.
    Scalar energy(const Particle &particle) const {
      return Acts::fastHypot(particle.mass(), absMomentum);
    }
  };

  /// Construct a default particle with invalid identity.
  Particle() = default;
  /// Construct a particle at rest with explicit mass and charge.
  ///
  /// @param particleId Particle identifier within an event
  /// @param pdg PDG id
  /// @param charge Particle charge in native units
  /// @param mass Particle mass in native units
  ///
  /// @warning It is the users responsibility that charge and mass match
  ///          the PDG particle number.
  Particle(Barcode particleId, Acts::PdgParticle pdg, Scalar charge,
           Scalar mass)
      : m_particleId(particleId), m_pdg(pdg), m_charge(charge), m_mass(mass) {}
  /// Construct a particle at rest from a PDG particle number.
  ///
  /// @param particleId Particle identifier within an event
  /// @param pdg PDG particle number
  ///
  /// Charge and mass are retrieved from the particle data table.
  Particle(Barcode particleId, Acts::PdgParticle pdg);
  Particle(const Particle &) = default;
  Particle(Particle &&) = default;
  Particle &operator=(const Particle &) = default;
  Particle &operator=(Particle &&) = default;

  /// Construct a new particle with a new identifier but same kinematics.
  ///
  /// @note This is intentionally not a regular setter. The particle id
  ///       is used to identify the whole particle. Setting it on an existing
  ///       particle is usually a mistake.
  Particle withParticleId(Barcode particleId) const {
    Particle p = *this;
    p.m_particleId = particleId;
    return p;
  }

  /// Set the process type that generated this particle.
  Particle &setProcess(ProcessType proc) {
    m_process = proc;
    return *this;
  }
  /// Set the pdg.
  Particle setPdg(Acts::PdgParticle pdg) {
    m_pdg = pdg;
    return *this;
  }
  /// Set the charge.
  Particle setCharge(Scalar charge) {
    m_charge = charge;
    return *this;
  }
  /// Set the mass.
  Particle setMass(Scalar mass) {
    m_mass = mass;
    return *this;
  }
  /// Set the particle ID.
  Particle &setParticleId(Barcode barcode) {
    m_particleId = barcode;
    return *this;
  }
  /// Set the space-time position four-vector.
  Particle &setPosition4(const Vector4 &pos4) {
    m_currentState.position4 = pos4;
    return *this;
  }
  /// Set the space-time position four-vector from three-position and time.
  Particle &setPosition4(const Vector3 &position, Scalar time) {
    m_currentState.position4.segment<3>(Acts::ePos0) = position;
    m_currentState.position4[Acts::eTime] = time;
    return *this;
  }
  /// Set the space-time position four-vector from scalar components.
  Particle &setPosition4(Scalar x, Scalar y, Scalar z, Scalar time) {
    m_currentState.position4[Acts::ePos0] = x;
    m_currentState.position4[Acts::ePos1] = y;
    m_currentState.position4[Acts::ePos2] = z;
    m_currentState.position4[Acts::eTime] = time;
    return *this;
  }
  /// Set the direction three-vector
  Particle &setDirection(const Vector3 &direction) {
    m_currentState.direction = direction;
    m_currentState.direction.normalize();
    return *this;
  }
  /// Set the direction three-vector from scalar components.
  Particle &setDirection(Scalar dx, Scalar dy, Scalar dz) {
    m_currentState.direction[Acts::ePos0] = dx;
    m_currentState.direction[Acts::ePos1] = dy;
    m_currentState.direction[Acts::ePos2] = dz;
    m_currentState.direction.normalize();
    return *this;
  }
  /// Set the absolute momentum.
  Particle &setAbsoluteMomentum(Scalar absMomentum) {
    m_currentState.absMomentum = absMomentum;
    return *this;
  }

  /// Change the energy by the given amount.
  ///
  /// Energy loss corresponds to a negative change. If the updated energy
  /// would result in an unphysical value, the particle is put to rest, i.e.
  /// its absolute momentum is set to zero.
  Particle &correctEnergy(Scalar delta) {
    const auto newEnergy =
        Acts::fastHypot(m_mass, m_currentState.absMomentum) + delta;
    if (newEnergy <= m_mass) {
      m_currentState.absMomentum = Scalar{0};
    } else {
      m_currentState.absMomentum =
          std::sqrt(newEnergy * newEnergy - m_mass * m_mass);
    }
    return *this;
  }

  /// Particle identifier within an event.
  constexpr Barcode particleId() const { return m_particleId; }
  /// Which type of process generated this particle.
  constexpr ProcessType process() const { return m_process; }
  /// PDG particle number that identifies the type.
  constexpr Acts::PdgParticle pdg() const { return m_pdg; }
  /// Absolute PDG particle number that identifies the type.
  constexpr Acts::PdgParticle absolutePdg() const {
    return Acts::makeAbsolutePdgParticle(pdg());
  }
  /// Particle charge.
  constexpr Scalar charge() const { return m_charge; }
  /// Particle absolute charge.
  constexpr Scalar absoluteCharge() const { return std::abs(m_charge); }
  /// Particle mass.
  constexpr Scalar mass() const { return m_mass; }

  /// Particle hypothesis.
  constexpr Acts::ParticleHypothesis hypothesis() const {
    return Acts::ParticleHypothesis(absolutePdg(), mass(), absoluteCharge());
  }
  /// Particl qOverP.
  constexpr Scalar qOverP() const {
    return hypothesis().qOverP(absoluteMomentum(), charge());
  }

  /// Space-time position four-vector.
  constexpr const Vector4 &fourPosition() const {
    return m_currentState.position4;
  }
  /// Three-position, i.e. spatial coordinates without the time.
  auto position() const { return m_currentState.position(); }
  /// Time coordinate.
  Scalar time() const { return m_currentState.time(); }
  /// Energy-momentum four-vector.
  Vector4 fourMomentum() const { return m_currentState.fourMomentum(*this); }
  /// Unit three-direction, i.e. the normalized momentum three-vector.
  const Vector3 &direction() const { return m_currentState.direction; }
  /// Polar angle.
  Scalar theta() const { return m_currentState.theta(); }
  /// Azimuthal angle.
  Scalar phi() const { return m_currentState.phi(); }
  /// Absolute momentum in the x-y plane.
  Scalar transverseMomentum() const {
    return m_currentState.transverseMomentum();
  }
  /// Absolute momentum.
  constexpr Scalar absoluteMomentum() const {
    return m_currentState.absMomentum;
  }
  /// Absolute momentum.
  Vector3 momentum() const { return m_currentState.momentum(); }
  /// Total energy, i.e. norm of the four-momentum.
  Scalar energy() const { return m_currentState.energy(*this); }

  /// Check if the particle is alive, i.e. is not at rest.
  constexpr bool isAlive() const {
    return Scalar{0} < m_currentState.absMomentum;
  }

  constexpr bool isSecondary() const {
    return particleId().vertexSecondary() != 0 ||
           particleId().generation() != 0 || particleId().subParticle() != 0;
  }

  // simulation specific properties

  /// Set the proper time in the particle rest frame.
  ///
  /// @param properTime passed proper time in the rest frame
  constexpr Particle &setProperTime(Scalar properTime) {
    m_currentState.properTime = properTime;
    return *this;
  }
  /// Proper time in the particle rest frame.
  constexpr Scalar properTime() const { return m_currentState.properTime; }

  /// Set the accumulated material measured in radiation/interaction lengths.
  ///
  /// @param pathInX0 accumulated material measured in radiation lengths
  /// @param pathInL0 accumulated material measured in interaction lengths
  constexpr Particle &setMaterialPassed(Scalar pathInX0, Scalar pathInL0) {
    m_currentState.pathInX0 = pathInX0;
    m_currentState.pathInL0 = pathInL0;
    return *this;
  }
  /// Accumulated path within material measured in radiation lengths.
  constexpr Scalar pathInX0() const { return m_currentState.pathInX0; }
  /// Accumulated path within material measured in interaction lengths.
  constexpr Scalar pathInL0() const { return m_currentState.pathInL0; }

  /// Set the reference surface.
  ///
  /// @param surface reference surface
  Particle &setReferenceSurface(const Acts::Surface *surface) {
    m_referenceSurface = surface;
    return *this;
  }

  /// Reference surface.
  const Acts::Surface *referenceSurface() const { return m_referenceSurface; }

  /// Check if the particle has a reference surface.
  bool hasReferenceSurface() const { return m_referenceSurface != nullptr; }

  /// Bound track parameters.
  Acts::Result<Acts::BoundTrackParameters> boundParameters(
      const Acts::GeometryContext &gctx) const {
    if (!hasReferenceSurface()) {
      return Acts::Result<Acts::BoundTrackParameters>::failure(
          std::error_code());
    }
    Acts::Result<Acts::Vector2> localResult =
        m_referenceSurface->globalToLocal(gctx, position(), direction());
    if (!localResult.ok()) {
      return localResult.error();
    }
    Acts::BoundVector params;
    params << localResult.value(), phi(), theta(), qOverP(), time();
    return Acts::BoundTrackParameters(referenceSurface()->getSharedPtr(),
                                      params, std::nullopt, hypothesis());
  }

  Acts::CurvilinearTrackParameters curvilinearParameters() const {
    return Acts::CurvilinearTrackParameters(
        fourPosition(), direction(), qOverP(), std::nullopt, hypothesis());
  }

  /// Set the number of hits.
  ///
  /// @param nHits number of hits
  constexpr Particle &setNumberOfHits(std::uint32_t nHits) {
    m_currentState.numberOfHits = nHits;
    return *this;
  }

  /// Number of hits.
  constexpr std::uint32_t numberOfHits() const {
    return m_currentState.numberOfHits;
  }

  /// Set the outcome of particle.
  ///
  /// @param outcome outcome code
  constexpr Particle &setOutcome(ParticleOutcome outcome) {
    m_outcome = outcome;
    return *this;
  }

  /// Particle outcome.
  constexpr ParticleOutcome outcome() const { return m_outcome; }

  /// Get the initial state of the particle.
  constexpr const State &initialState() const { return m_initialState; }
  /// Get the current state of the particle.
  constexpr const State &currentState() const { return m_currentState; }
  /// Get the final state of the particle.
  constexpr const State &finalState() const { return m_finalState; }

  /// Get mutable access to the initial state of the particle.
  constexpr State &initialState() { return m_initialState; }
  /// Get mutable access to the current state of the particle.
  constexpr State &currentState() { return m_currentState; }
  /// Get mutable access to the final state of the particle.
  constexpr State &finalState() { return m_finalState; }

  /// Store the current state as the initial state.
  constexpr void storeInitialState() { m_initialState = m_currentState; }
  /// Store the current state as the final state.
  constexpr void storeFinalState() { m_finalState = m_currentState; }

  constexpr double energyLoss() const {
    return m_initialState.energy(*this) - m_finalState.energy(*this);
  }
  constexpr double finalPathInX0() const { return m_finalState.pathInX0; }
  constexpr double finalPathInL0() const { return m_finalState.pathInL0; }
  constexpr std::uint32_t finalNumberOfHits() const {
    return m_finalState.numberOfHits;
  }

 private:
  // identity, i.e. things that do not change over the particle lifetime.
  /// Particle identifier within the event.
  Barcode m_particleId;
  /// Process type specifier.
  ProcessType m_process = ProcessType::eUndefined;
  /// PDG particle number.
  Acts::PdgParticle m_pdg = Acts::PdgParticle::eInvalid;
  // Particle charge and mass.
  Scalar m_charge = Scalar{0};
  Scalar m_mass = Scalar{0};

  State m_initialState{};
  State m_currentState{};
  State m_finalState{};

  // additional initial information
  /// reference surface
  const Acts::Surface *m_referenceSurface{nullptr};

  // additional final information
  /// outcome
  ParticleOutcome m_outcome = ParticleOutcome::Unknown;
};

std::ostream &operator<<(std::ostream &os, const Particle &particle);

}  // namespace ActsFatras
