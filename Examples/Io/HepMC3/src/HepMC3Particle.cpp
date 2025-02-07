// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/HepMC3/HepMC3Particle.hpp"

#include "Acts/Definitions/ParticleData.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Io/HepMC3/HepMC3Vertex.hpp"

namespace ActsExamples {

SimBarcode HepMC3Particle::barcode(
    const HepMC3::ConstGenParticlePtr& particle) {
  // TODO this is probably not quite right
  return particle->id();
}

SimParticle HepMC3Particle::particle(
    const HepMC3::ConstGenParticlePtr& particle) {
  SimBarcode particleId = barcode(particle);
  Acts::PdgParticle pdg = static_cast<Acts::PdgParticle>(particle->pid());
  SimParticleState fw(particleId, pdg, Acts::findCharge(pdg).value_or(0),
                      particle->generated_mass());
  fw.setDirection(particle->momentum().x(), particle->momentum().y(),
                  particle->momentum().z());
  fw.setAbsoluteMomentum(particle->momentum().p3mod());
  return SimParticle(fw, fw);
}

int HepMC3Particle::id(const std::shared_ptr<HepMC3::GenParticle>& particle) {
  return particle->id();
}

std::unique_ptr<SimVertex> HepMC3Particle::productionVertex(
    const std::shared_ptr<HepMC3::GenParticle>& particle) {
  // Return the vertex if it exists
  if (particle->production_vertex()) {
    return HepMC3Vertex::processVertex(
        std::make_shared<HepMC3::GenVertex>(*particle->production_vertex()));
  } else {
    return nullptr;
  }
}

std::unique_ptr<SimVertex> HepMC3Particle::endVertex(
    const std::shared_ptr<HepMC3::GenParticle>& particle) {
  // Return the vertex if it exists
  if (particle->end_vertex()) {
    return HepMC3Vertex::processVertex(
        std::make_shared<HepMC3::GenVertex>(*(particle->end_vertex())));
  } else {
    return nullptr;
  }
}

int HepMC3Particle::pdgID(
    const std::shared_ptr<HepMC3::GenParticle>& particle) {
  return particle->pid();
}

Acts::Vector3 HepMC3Particle::momentum(
    const std::shared_ptr<HepMC3::GenParticle>& particle) {
  Acts::Vector3 mom;
  mom(0) = particle->momentum().x();
  mom(1) = particle->momentum().y();
  mom(2) = particle->momentum().z();
  return mom;
}

double HepMC3Particle::energy(
    const std::shared_ptr<HepMC3::GenParticle>& particle) {
  return particle->momentum().e();
}

double HepMC3Particle::mass(
    const std::shared_ptr<HepMC3::GenParticle>& particle) {
  return particle->generated_mass();
}

double HepMC3Particle::charge(
    const std::shared_ptr<HepMC3::GenParticle>& particle) {
  return Acts::findCharge(static_cast<Acts::PdgParticle>(particle->pid()))
      .value_or(0);
}

void HepMC3Particle::pdgID(const std::shared_ptr<HepMC3::GenParticle>& particle,
                           const int pid) {
  particle->set_pid(pid);
}

void HepMC3Particle::momentum(
    const std::shared_ptr<HepMC3::GenParticle>& particle,
    const Acts::Vector3& mom) {
  HepMC3::FourVector fVec(mom(0), mom(1), mom(2), particle->momentum().e());
  particle->set_momentum(fVec);
}

void HepMC3Particle::energy(
    const std::shared_ptr<HepMC3::GenParticle>& particle, const double energy) {
  HepMC3::FourVector fVec(particle->momentum().x(), particle->momentum().y(),
                          particle->momentum().z(), energy);
  particle->set_momentum(fVec);
}

void HepMC3Particle::mass(const std::shared_ptr<HepMC3::GenParticle>& particle,
                          const double mass) {
  particle->set_generated_mass(mass);
}

}  // namespace ActsExamples
