// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Propagator/ConstrainedStep.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/EventData/ProcessType.hpp"
#include "ActsFatras/Kernel/detail/SimulationActor.hpp"
#include "ActsFatras/Selectors/SurfaceSelectors.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <random>
#include <utility>
#include <vector>

using namespace Acts::UnitLiterals;
using namespace ActsFatras;

namespace {

constexpr auto tol = 4 * std::numeric_limits<Particle::Scalar>::epsilon();
constexpr auto inf = std::numeric_limits<Particle::Scalar>::infinity();

struct MockDecay {
  Particle::Scalar properTimeLimit = inf;

  template <typename generator_t>
  constexpr Particle::Scalar generateProperTimeLimit(
      generator_t & /*generator*/, const Particle &particle) const {
    return particle.properTime() + properTimeLimit;
  }
  template <typename generator_t>
  constexpr std::array<Particle, 0> run(generator_t & /*generator*/,
                                        const Particle & /*particle*/) const {
    return {};
  }
};

struct MockInteractionList {
  struct Selection {
    double x0Limit = std::numeric_limits<double>::infinity();
    double l0Limit = std::numeric_limits<double>::infinity();
    std::size_t x0Process = std::numeric_limits<std::size_t>::max();
    std::size_t l0Process = std::numeric_limits<std::size_t>::max();
  };

  double energyLoss = 0;

  template <typename generator_t>
  bool runContinuous(generator_t & /*generator*/,
                     const Acts::MaterialSlab & /*slab*/, Particle &particle,
                     std::vector<Particle> &generated) const {
    generated.push_back(particle);
    particle.correctEnergy(-energyLoss);
    // break if particle is not alive anymore
    return !particle.isAlive();
  }

  template <typename generator_t>
  Selection armPointLike(generator_t & /*generator*/,
                         const Particle & /*particle*/) const {
    return {};
  }

  template <typename generator_t>
  bool runPointLike(generator_t & /*generator*/, std::size_t /*processIndex*/,
                    Particle & /*particle*/,
                    std::vector<Particle> & /*generated*/) const {
    return false;
  }
};

struct MockStepperState {
  using Scalar = Acts::ActsScalar;
  using Vector3 = Acts::ActsVector<3>;

  Vector3 pos = Vector3::Zero();
  Scalar time = 0;
  Vector3 dir = Vector3::Zero();
  Scalar p = 0;
};

struct MockStepper {
  using State = MockStepperState;
  using Scalar = MockStepperState::Scalar;
  using Vector3 = MockStepperState::Vector3;

  auto position(const State &state) const { return state.pos; }
  auto time(const State &state) const { return state.time; }
  auto direction(const State &state) const { return state.dir; }
  auto absoluteMomentum(const State &state) const { return state.p; }
  void update(State &state, const Vector3 &pos, const Vector3 &dir, Scalar qop,
              Scalar time) {
    state.pos = pos;
    state.time = time;
    state.dir = dir;
    state.p = 1 / qop;
  }
  void updateStepSize(State & /*state*/, double /*stepSize*/,
                      Acts::ConstrainedStep::Type /*stype*/) const {}
};

struct MockNavigatorState {
  bool targetReached = false;
  Acts::Surface *startSurface = nullptr;
  Acts::Surface *currentSurface = nullptr;
};

struct MockNavigator {
  bool targetReached(const MockNavigatorState &state) const {
    return state.targetReached;
  }

  void targetReached(MockNavigatorState &state, bool reached) const {
    state.targetReached = reached;
  }

  const Acts::Surface *startSurface(const MockNavigatorState &state) const {
    return state.startSurface;
  }

  const Acts::Surface *currentSurface(const MockNavigatorState &state) const {
    return state.currentSurface;
  }

  const Acts::TrackingVolume *currentVolume(
      const MockNavigatorState & /*state*/) const {
    return nullptr;
  }

  bool endOfWorldReached(const MockNavigatorState & /*state*/) const {
    return false;
  }
};

struct MockPropagatorState {
  MockNavigatorState navigation;
  MockStepperState stepping;
  Acts::GeometryContext geoContext;
  Acts::PropagatorStage stage = Acts::PropagatorStage::invalid;

  struct {
    std::vector<std::uint32_t> constrainToVolumeIds;
  } options;
};

template <typename SurfaceSelector>
struct Fixture {
  using Generator = std::ranlux48;
  using Actor = typename ActsFatras::detail::SimulationActor<
      Generator, MockDecay, MockInteractionList, SurfaceSelector>;
  using Result = typename Actor::result_type;

  // reference information for initial particle
  Barcode pid = Barcode().setVertexPrimary(12u).setParticle(3u);
  ProcessType proc = ProcessType::eUndefined;
  Acts::PdgParticle pdg = Acts::PdgParticle::eProton;
  Particle::Scalar q = 1_e;
  Particle::Scalar m = 1_GeV;
  Particle::Scalar p = 1_GeV;
  Particle::Scalar e;
  Generator generator;
  std::shared_ptr<Acts::Surface> surface;
  Actor actor;
  Result result;
  MockPropagatorState state;
  MockStepper stepper;
  MockNavigator navigator;

  Fixture(double energyLoss, std::shared_ptr<Acts::Surface> surface_)
      : e(std::hypot(m, p)), generator(42), surface(std::move(surface_)) {
    const auto particle = Particle(pid, pdg, q, m)
                              .setProcess(proc)
                              .setPosition4(1_mm, 2_mm, 3_mm, 4_ns)
                              .setDirection(1, 0, 0)
                              .setAbsoluteMomentum(p);
    actor.generator = &generator;
    actor.interactions.energyLoss = energyLoss;
    actor.initialParticle = particle;
    state.stage = Acts::PropagatorStage::postStep;
    state.navigation.currentSurface = surface.get();
    state.stepping.pos = particle.position();
    state.stepping.time = particle.time();
    state.stepping.dir = particle.direction();
    state.stepping.p = particle.absoluteMomentum();
  }
};

// make a surface without material.
std::shared_ptr<Acts::Surface> makeEmptySurface() {
  auto surface =
      Acts::CurvilinearSurface(Acts::Vector3(1, 2, 3), Acts::Vector3(1, 0, 0))
          .planeSurface();
  return surface;
}

// make a surface with 1% X0/L0 material.
std::shared_ptr<Acts::Surface> makeMaterialSurface() {
  auto surface = makeEmptySurface();
  auto slab = Acts::Test::makeUnitSlab();
  surface->assignSurfaceMaterial(
      std::make_shared<Acts::HomogeneousSurfaceMaterial>(slab));
  return surface;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(FatrasSimulationActor)

BOOST_AUTO_TEST_CASE(HitsOnEmptySurface) {
  Fixture<EverySurface> f(125_MeV, makeEmptySurface());

  // input reference check
  BOOST_CHECK_EQUAL(f.actor.initialParticle.particleId(), f.pid);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.process(), f.proc);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.pdg(), f.pdg);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.mass(), f.m);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.absoluteMomentum(), f.p);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.energy(), f.e);

  // call.actor: surface selection -> one hit, no material -> no secondary
  f.actor.act(f.state, f.stepper, f.navigator, f.result,
              Acts::getDummyLogger());
  BOOST_CHECK(f.result.isAlive);
  CHECK_CLOSE_REL(f.result.particle.energy(), f.e, tol);
  BOOST_CHECK_EQUAL(f.result.generatedParticles.size(), 0u);
  BOOST_CHECK_EQUAL(f.result.hits.size(), 1u);
  BOOST_CHECK_EQUAL(f.result.hits[0].index(), 0u);
  // proper time must be non-NaN, but is zero since no time has passed
  BOOST_CHECK_EQUAL(f.result.particle.properTime(), 0);
  // empty surfaces adds no material
  BOOST_CHECK_EQUAL(f.result.particle.pathInX0(), 0);
  BOOST_CHECK_EQUAL(f.result.particle.pathInL0(), 0);
  // no processes are configured, so none can be selected
  BOOST_CHECK_EQUAL(f.result.x0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.x0Process,
                    std::numeric_limits<std::size_t>::max());
  BOOST_CHECK_EQUAL(f.result.l0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.l0Process,
                    std::numeric_limits<std::size_t>::max());
  // check consistency between particle and stepper state
  BOOST_CHECK_EQUAL(f.state.stepping.pos, f.result.particle.position());
  BOOST_CHECK_EQUAL(f.state.stepping.time, f.result.particle.time());
  BOOST_CHECK_EQUAL(f.state.stepping.dir, f.result.particle.direction());
  BOOST_CHECK_EQUAL(f.state.stepping.p, f.result.particle.absoluteMomentum());

  // call.actor again: one more hit, still no secondary
  f.actor.act(f.state, f.stepper, f.navigator, f.result,
              Acts::getDummyLogger());
  BOOST_CHECK(f.result.isAlive);
  CHECK_CLOSE_REL(f.result.particle.energy(), f.e, tol);
  BOOST_CHECK_EQUAL(f.result.generatedParticles.size(), 0u);
  BOOST_CHECK_EQUAL(f.result.hits.size(), 2u);
  BOOST_CHECK_EQUAL(f.result.hits[0].index(), 0u);
  BOOST_CHECK_EQUAL(f.result.hits[1].index(), 1u);
  // proper time must be non-NaN, but is zero since no time
  // has passed
  BOOST_CHECK_EQUAL(f.result.particle.properTime(), 0);
  // empty surfaces adds no material
  BOOST_CHECK_EQUAL(f.result.particle.pathInX0(), 0);
  BOOST_CHECK_EQUAL(f.result.particle.pathInL0(), 0);
  // no processes are configured, so none can be selected
  BOOST_CHECK_EQUAL(f.result.x0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.x0Process,
                    std::numeric_limits<std::size_t>::max());
  BOOST_CHECK_EQUAL(f.result.l0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.l0Process,
                    std::numeric_limits<std::size_t>::max());
  // check consistency between particle and stepper state
  BOOST_CHECK_EQUAL(f.state.stepping.pos, f.result.particle.position());
  BOOST_CHECK_EQUAL(f.state.stepping.time, f.result.particle.time());
  BOOST_CHECK_EQUAL(f.state.stepping.dir, f.result.particle.direction());
  BOOST_CHECK_EQUAL(f.state.stepping.p, f.result.particle.absoluteMomentum());

  // particle identity should be the same as the initial input
  BOOST_CHECK_EQUAL(f.result.particle.particleId(), f.pid);
  BOOST_CHECK_EQUAL(f.result.particle.process(), f.proc);
  BOOST_CHECK_EQUAL(f.result.particle.pdg(), f.pdg);
  BOOST_CHECK_EQUAL(f.result.particle.charge(), f.q);
  BOOST_CHECK_EQUAL(f.result.particle.mass(), f.m);
}

BOOST_AUTO_TEST_CASE(HitsOnMaterialSurface) {
  Fixture<EverySurface> f(125_MeV, makeMaterialSurface());

  // input reference check
  BOOST_CHECK_EQUAL(f.actor.initialParticle.particleId(), f.pid);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.process(), f.proc);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.pdg(), f.pdg);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.mass(), f.m);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.absoluteMomentum(), f.p);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.energy(), f.e);

  // call.actor: surface selection -> one hit, material -> one secondary
  f.actor.act(f.state, f.stepper, f.navigator, f.result,
              Acts::getDummyLogger());
  BOOST_CHECK(f.result.isAlive);
  CHECK_CLOSE_REL(f.result.particle.energy(), f.e - 125_MeV, tol);
  BOOST_CHECK_EQUAL(f.result.generatedParticles.size(), 1u);
  BOOST_CHECK_EQUAL(f.result.hits.size(), 1u);
  BOOST_CHECK_EQUAL(f.result.hits[0].index(), 0u);
  // proper time must be non-NaN, but is zero since no time
  // has passed
  BOOST_CHECK_EQUAL(f.result.particle.properTime(), 0);
  // test material is a unit slab
  BOOST_CHECK_EQUAL(f.result.particle.pathInX0(), 1);
  BOOST_CHECK_EQUAL(f.result.particle.pathInL0(), 1);
  // no processes are configured, so none can be selected
  BOOST_CHECK_EQUAL(f.result.x0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.x0Process,
                    std::numeric_limits<std::size_t>::max());
  BOOST_CHECK_EQUAL(f.result.l0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.l0Process,
                    std::numeric_limits<std::size_t>::max());
  // check consistency between particle and stepper state
  BOOST_CHECK_EQUAL(f.state.stepping.pos, f.result.particle.position());
  BOOST_CHECK_EQUAL(f.state.stepping.time, f.result.particle.time());
  BOOST_CHECK_EQUAL(f.state.stepping.dir, f.result.particle.direction());
  CHECK_CLOSE_REL(f.state.stepping.p, f.result.particle.absoluteMomentum(),
                  tol);

  // call.actor again: one more hit, one more secondary
  f.actor.act(f.state, f.stepper, f.navigator, f.result,
              Acts::getDummyLogger());
  BOOST_CHECK(f.result.isAlive);
  CHECK_CLOSE_REL(f.result.particle.energy(), f.e - 250_MeV, tol);
  BOOST_CHECK_EQUAL(f.result.generatedParticles.size(), 2u);
  BOOST_CHECK_EQUAL(f.result.hits.size(), 2u);
  BOOST_CHECK_EQUAL(f.result.hits[0].index(), 0u);
  BOOST_CHECK_EQUAL(f.result.hits[1].index(), 1u);
  // proper time must be non-NaN, but is zero since no time
  // has passed
  BOOST_CHECK_EQUAL(f.result.particle.properTime(), 0);
  // test material is a unit slab that was passed twice
  BOOST_CHECK_EQUAL(f.result.particle.pathInX0(), 2);
  BOOST_CHECK_EQUAL(f.result.particle.pathInL0(), 2);
  // no processes are configured, so none can be selected
  BOOST_CHECK_EQUAL(f.result.x0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.x0Process,
                    std::numeric_limits<std::size_t>::max());
  BOOST_CHECK_EQUAL(f.result.l0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.l0Process,
                    std::numeric_limits<std::size_t>::max());
  // check consistency between particle and stepper state
  BOOST_CHECK_EQUAL(f.state.stepping.pos, f.result.particle.position());
  BOOST_CHECK_EQUAL(f.state.stepping.time, f.result.particle.time());
  BOOST_CHECK_EQUAL(f.state.stepping.dir, f.result.particle.direction());
  BOOST_CHECK_EQUAL(f.state.stepping.p, f.result.particle.absoluteMomentum());

  // particle identity should be the same as the initial input
  BOOST_CHECK_EQUAL(f.result.particle.particleId(), f.pid);
  BOOST_CHECK_EQUAL(f.result.particle.process(), f.proc);
  BOOST_CHECK_EQUAL(f.result.particle.pdg(), f.pdg);
  BOOST_CHECK_EQUAL(f.result.particle.charge(), f.q);
  BOOST_CHECK_EQUAL(f.result.particle.mass(), f.m);
}

BOOST_AUTO_TEST_CASE(NoHitsEmptySurface) {
  Fixture<NoSurface> f(125_MeV, makeEmptySurface());

  // input reference check
  BOOST_CHECK_EQUAL(f.actor.initialParticle.particleId(), f.pid);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.process(), f.proc);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.pdg(), f.pdg);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.mass(), f.m);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.absoluteMomentum(), f.p);
  BOOST_CHECK_EQUAL(f.actor.initialParticle.energy(), f.e);

  // call.actor: no surface sel. -> no hit, no material -> no secondary
  f.actor.act(f.state, f.stepper, f.navigator, f.result,
              Acts::getDummyLogger());
  BOOST_CHECK(f.result.isAlive);
  CHECK_CLOSE_REL(f.result.particle.energy(), f.e, tol);
  BOOST_CHECK_EQUAL(f.result.generatedParticles.size(), 0u);
  BOOST_CHECK_EQUAL(f.result.hits.size(), 0u);
  // proper time must be non-NaN, but is zero since no time
  // has passed
  BOOST_CHECK_EQUAL(f.result.particle.properTime(), 0);
  // empty surfaces adds no material
  BOOST_CHECK_EQUAL(f.result.particle.pathInX0(), 0);
  BOOST_CHECK_EQUAL(f.result.particle.pathInL0(), 0);
  // no processes are configured, so none can be selected
  BOOST_CHECK_EQUAL(f.result.x0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.x0Process,
                    std::numeric_limits<std::size_t>::max());
  BOOST_CHECK_EQUAL(f.result.l0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.l0Process,
                    std::numeric_limits<std::size_t>::max());
  // check consistency between particle and stepper state
  BOOST_CHECK_EQUAL(f.state.stepping.pos, f.result.particle.position());
  BOOST_CHECK_EQUAL(f.state.stepping.time, f.result.particle.time());
  BOOST_CHECK_EQUAL(f.state.stepping.dir, f.result.particle.direction());
  BOOST_CHECK_EQUAL(f.state.stepping.p, f.result.particle.absoluteMomentum());

  // call.actor again: no hit, still no secondary
  f.actor.act(f.state, f.stepper, f.navigator, f.result,
              Acts::getDummyLogger());
  BOOST_CHECK(f.result.isAlive);
  CHECK_CLOSE_REL(f.result.particle.energy(), f.e, tol);
  BOOST_CHECK_EQUAL(f.result.generatedParticles.size(), 0u);
  BOOST_CHECK_EQUAL(f.result.hits.size(), 0u);
  // proper time must be non-NaN, but is zero since no time
  // has passed
  BOOST_CHECK_EQUAL(f.result.particle.properTime(), 0);
  // empty surfaces adds no material
  BOOST_CHECK_EQUAL(f.result.particle.pathInX0(), 0);
  BOOST_CHECK_EQUAL(f.result.particle.pathInL0(), 0);
  // no processes are configured, so none can be selected
  BOOST_CHECK_EQUAL(f.result.x0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.x0Process,
                    std::numeric_limits<std::size_t>::max());
  BOOST_CHECK_EQUAL(f.result.l0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.l0Process,
                    std::numeric_limits<std::size_t>::max());
  // check consistency between particle and stepper state
  BOOST_CHECK_EQUAL(f.state.stepping.pos, f.result.particle.position());
  BOOST_CHECK_EQUAL(f.state.stepping.time, f.result.particle.time());
  BOOST_CHECK_EQUAL(f.state.stepping.dir, f.result.particle.direction());
  BOOST_CHECK_EQUAL(f.state.stepping.p, f.result.particle.absoluteMomentum());

  // particle identity should be the same as the initial input
  BOOST_CHECK_EQUAL(f.result.particle.particleId(), f.pid);
  BOOST_CHECK_EQUAL(f.result.particle.process(), f.proc);
  BOOST_CHECK_EQUAL(f.result.particle.pdg(), f.pdg);
  BOOST_CHECK_EQUAL(f.result.particle.charge(), f.q);
  BOOST_CHECK_EQUAL(f.result.particle.mass(), f.m);
}

BOOST_AUTO_TEST_CASE(NoHitsMaterialSurface) {
  Fixture<NoSurface> f(125_MeV, makeMaterialSurface());

  // call.actor: no surface sel. -> no hit, material -> one secondary
  f.actor.act(f.state, f.stepper, f.navigator, f.result,
              Acts::getDummyLogger());
  BOOST_CHECK(f.result.isAlive);
  CHECK_CLOSE_REL(f.result.particle.energy(), f.e - 125_MeV, tol);
  BOOST_CHECK_EQUAL(f.result.generatedParticles.size(), 1u);
  BOOST_CHECK_EQUAL(f.result.hits.size(), 0u);
  // proper time must be non-NaN, but is zero since no time
  // has passed
  BOOST_CHECK_EQUAL(f.result.particle.properTime(), 0);
  // test material is a unit slab
  BOOST_CHECK_EQUAL(f.result.particle.pathInX0(), 1);
  BOOST_CHECK_EQUAL(f.result.particle.pathInL0(), 1);
  // no processes are configured, so none can be selected
  BOOST_CHECK_EQUAL(f.result.x0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.x0Process,
                    std::numeric_limits<std::size_t>::max());
  BOOST_CHECK_EQUAL(f.result.l0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.l0Process,
                    std::numeric_limits<std::size_t>::max());
  // check consistency between particle and stepper state
  BOOST_CHECK_EQUAL(f.state.stepping.pos, f.result.particle.position());
  BOOST_CHECK_EQUAL(f.state.stepping.time, f.result.particle.time());
  BOOST_CHECK_EQUAL(f.state.stepping.dir, f.result.particle.direction());
  CHECK_CLOSE_REL(f.state.stepping.p, f.result.particle.absoluteMomentum(),
                  tol);

  // call.actor again: still no hit, one more secondary
  f.actor.act(f.state, f.stepper, f.navigator, f.result,
              Acts::getDummyLogger());
  BOOST_CHECK(f.result.isAlive);
  CHECK_CLOSE_REL(f.result.particle.energy(), f.e - 250_MeV, tol);
  BOOST_CHECK_EQUAL(f.result.generatedParticles.size(), 2u);
  BOOST_CHECK_EQUAL(f.result.hits.size(), 0u);
  // proper time must be non-NaN, but is zero since no time
  // has passed
  BOOST_CHECK_EQUAL(f.result.particle.properTime(), 0);
  // test material is a unit slab that was passed twice
  BOOST_CHECK_EQUAL(f.result.particle.pathInX0(), 2);
  BOOST_CHECK_EQUAL(f.result.particle.pathInL0(), 2);
  // no processes are configured, so none can be selected
  BOOST_CHECK_EQUAL(f.result.x0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.x0Process,
                    std::numeric_limits<std::size_t>::max());
  BOOST_CHECK_EQUAL(f.result.l0Limit, inf);
  BOOST_CHECK_EQUAL(f.result.l0Process,
                    std::numeric_limits<std::size_t>::max());
  // check consistency between particle and stepper state
  BOOST_CHECK_EQUAL(f.state.stepping.pos, f.result.particle.position());
  BOOST_CHECK_EQUAL(f.state.stepping.time, f.result.particle.time());
  BOOST_CHECK_EQUAL(f.state.stepping.dir, f.result.particle.direction());
  BOOST_CHECK_EQUAL(f.state.stepping.p, f.result.particle.absoluteMomentum());

  // particle identity should be the same as the initial input
  BOOST_CHECK_EQUAL(f.result.particle.particleId(), f.pid);
  BOOST_CHECK_EQUAL(f.result.particle.process(), f.proc);
  BOOST_CHECK_EQUAL(f.result.particle.pdg(), f.pdg);
  BOOST_CHECK_EQUAL(f.result.particle.charge(), f.q);
  BOOST_CHECK_EQUAL(f.result.particle.mass(), f.m);
}

BOOST_AUTO_TEST_CASE(Decay) {
  // configure no energy loss for the decay tests
  Fixture<NoSurface> f(0_GeV, makeEmptySurface());

  // inverse Lorentz factor for proper time dilation: 1/gamma = m/E
  const auto gammaInv = f.m / f.e;

  // first step w/ defaults leaves particle alive
  f.actor.act(f.state, f.stepper, f.navigator, f.result,
              Acts::getDummyLogger());
  BOOST_CHECK(f.result.isAlive);
  BOOST_CHECK_EQUAL(f.result.particle.particleId(), f.pid);
  BOOST_CHECK_EQUAL(f.result.particle.process(), f.proc);
  BOOST_CHECK_EQUAL(f.result.particle.pdg(), f.pdg);
  BOOST_CHECK_EQUAL(f.result.particle.charge(), f.q);
  BOOST_CHECK_EQUAL(f.result.particle.mass(), f.m);
  CHECK_CLOSE_REL(f.result.particle.energy(), f.e, tol);
  BOOST_CHECK_EQUAL(f.result.particle.properTime(), 0_ns);

  // second step w/ defaults increases proper time
  f.state.stepping.time += 1_ns;
  f.actor.act(f.state, f.stepper, f.navigator, f.result,
              Acts::getDummyLogger());
  BOOST_CHECK(f.result.isAlive);
  BOOST_CHECK_EQUAL(f.result.particle.particleId(), f.pid);
  BOOST_CHECK_EQUAL(f.result.particle.process(), f.proc);
  BOOST_CHECK_EQUAL(f.result.particle.pdg(), f.pdg);
  BOOST_CHECK_EQUAL(f.result.particle.charge(), f.q);
  BOOST_CHECK_EQUAL(f.result.particle.mass(), f.m);
  CHECK_CLOSE_REL(f.result.particle.energy(), f.e, tol);
  CHECK_CLOSE_REL(f.result.particle.properTime(), gammaInv * 1_ns, tol);

  // third step w/ proper time limit decays the particle
  f.state.stepping.time += 1_ns;
  f.result.properTimeLimit = f.result.particle.properTime() + gammaInv * 0.5_ns;
  f.actor.act(f.state, f.stepper, f.navigator, f.result,
              Acts::getDummyLogger());
  BOOST_CHECK(!f.result.isAlive);
  BOOST_CHECK_EQUAL(f.result.particle.particleId(), f.pid);
  BOOST_CHECK_EQUAL(f.result.particle.process(), f.proc);
  BOOST_CHECK_EQUAL(f.result.particle.pdg(), f.pdg);
  BOOST_CHECK_EQUAL(f.result.particle.charge(), f.q);
  BOOST_CHECK_EQUAL(f.result.particle.mass(), f.m);
  CHECK_CLOSE_REL(f.result.particle.energy(), f.e, tol);
  CHECK_CLOSE_REL(f.result.particle.properTime(), gammaInv * 2_ns, tol);
}

BOOST_AUTO_TEST_SUITE_END()
