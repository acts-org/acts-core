// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Fatras/FatrasSimulation.hpp"

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Propagator/SympyStepper.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsFatras/EventData/Hit.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Kernel/InteractionList.hpp"
#include "ActsFatras/Kernel/Simulation.hpp"
#include "ActsFatras/Physics/Decay/NoDecay.hpp"
#include "ActsFatras/Physics/ElectroMagnetic/PhotonConversion.hpp"
#include "ActsFatras/Physics/StandardInteractions.hpp"
#include "ActsFatras/Selectors/SelectorHelpers.hpp"
#include "ActsFatras/Selectors/SurfaceSelectors.hpp"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

#include <boost/version.hpp>

namespace {

/// Simple struct to select surfaces where hits should be generated.
struct HitSurfaceSelector {
  bool sensitive = false;
  bool material = false;
  bool passive = false;

  /// Check if the surface should be used.
  bool operator()(const Acts::Surface &surface) const {
    // sensitive/material are not mutually exclusive
    bool isSensitive = surface.associatedDetectorElement() != nullptr;
    bool isMaterial = surface.surfaceMaterial() != nullptr;
    // passive should be an orthogonal category
    bool isPassive = !(isSensitive || isMaterial);
    return (isSensitive && sensitive) || (isMaterial && material) ||
           (isPassive && passive);
  }
};

}  // namespace

// Same interface as `ActsFatras::Simulation` but with concrete types.
struct ActsExamples::detail::FatrasSimulation {
  virtual ~FatrasSimulation() = default;
  virtual Acts::Result<std::vector<ActsFatras::FailedParticle>> simulate(
      const Acts::GeometryContext &, const Acts::MagneticFieldContext &,
      ActsExamples::RandomEngine &, const std::vector<ActsFatras::Particle> &,
      std::vector<ActsFatras::Particle> &, std::vector<ActsFatras::Particle> &,
      std::vector<ActsFatras::Hit> &) const = 0;
};

namespace {

// Magnetic-field specific PIMPL implementation.
//
// This always uses the SympyStepper for charged particle propagation and is
// thus limited to propagation in vacuum at the moment.
struct FatrasSimulationT final : ActsExamples::detail::FatrasSimulation {
  using CutPMin = ActsFatras::Min<ActsFatras::Casts::P>;

  // typedefs for charge particle simulation
  // propagate charged particles numerically in the given magnetic field
  using ChargedStepper = Acts::SympyStepper;
  using ChargedPropagator = Acts::Propagator<ChargedStepper, Acts::Navigator>;
  // charged particles w/ standard em physics list and selectable hits
  using ChargedSelector = CutPMin;
  using ChargedSimulation = ActsFatras::SingleParticleSimulation<
      ChargedPropagator, ActsFatras::StandardChargedElectroMagneticInteractions,
      HitSurfaceSelector, ActsFatras::NoDecay>;

  // typedefs for neutral particle simulation
  // propagate neutral particles with just straight lines
  using NeutralStepper = Acts::StraightLineStepper;
  using NeutralPropagator = Acts::Propagator<NeutralStepper, Acts::Navigator>;
  // neutral particles w/ photon conversion and no hits
  using NeutralSelector = CutPMin;
  using NeutralInteractions =
      ActsFatras::InteractionList<ActsFatras::PhotonConversion>;
  using NeutralSimulation = ActsFatras::SingleParticleSimulation<
      NeutralPropagator, NeutralInteractions, ActsFatras::NoSurface,
      ActsFatras::NoDecay>;

  // combined simulation type
  using Simulation = ActsFatras::Simulation<ChargedSelector, ChargedSimulation,
                                            NeutralSelector, NeutralSimulation>;

  Simulation simulation;

  FatrasSimulationT(const ActsExamples::FatrasSimulation::Config &cfg,
                    Acts::Logging::Level lvl)
      : simulation(
            ChargedSimulation(
                ChargedPropagator(
                    ChargedStepper(cfg.magneticField),
                    Acts::Navigator({cfg.trackingGeometry},
                                    Acts::getDefaultLogger("SimNav", lvl)),
                    Acts::getDefaultLogger("SimProp", lvl)),
                Acts::getDefaultLogger("Simulation", lvl)),
            NeutralSimulation(
                NeutralPropagator(
                    NeutralStepper(),
                    Acts::Navigator({cfg.trackingGeometry},
                                    Acts::getDefaultLogger("SimNav", lvl)),
                    Acts::getDefaultLogger("SimProp", lvl)),
                Acts::getDefaultLogger("Simulation", lvl))) {
    using namespace ActsFatras;
    using namespace ActsFatras::detail;
    // apply the configuration

    // minimal p cut on input particles and as is-alive check for interactions
    simulation.selectCharged.valMin = cfg.pMin;
    simulation.selectNeutral.valMin = cfg.pMin;
    simulation.charged.interactions =
        makeStandardChargedElectroMagneticInteractions(cfg.pMin);

    // processes are enabled by default
    if (!cfg.emScattering) {
      simulation.charged.interactions.template disable<StandardScattering>();
    }
    if (!cfg.emEnergyLossIonisation) {
      simulation.charged.interactions.template disable<StandardBetheBloch>();
    }
    if (!cfg.emEnergyLossRadiation) {
      simulation.charged.interactions.template disable<StandardBetheHeitler>();
    }
    if (!cfg.emPhotonConversion) {
      simulation.neutral.interactions.template disable<PhotonConversion>();
    }

    // configure hit surfaces for charged particles
    simulation.charged.selectHitSurface.sensitive = cfg.generateHitsOnSensitive;
    simulation.charged.selectHitSurface.material = cfg.generateHitsOnMaterial;
    simulation.charged.selectHitSurface.passive = cfg.generateHitsOnPassive;

    simulation.charged.maxStepSize = cfg.maxStepSize;
    simulation.charged.pathLimit = cfg.pathLimit;
    simulation.neutral.maxStepSize = cfg.maxStepSize;
    simulation.neutral.pathLimit = cfg.pathLimit;
  }
  ~FatrasSimulationT() final = default;

  Acts::Result<std::vector<ActsFatras::FailedParticle>> simulate(
      const Acts::GeometryContext &geoCtx,
      const Acts::MagneticFieldContext &magCtx, ActsExamples::RandomEngine &rng,
      const std::vector<ActsFatras::Particle> &inputParticles,
      std::vector<ActsFatras::Particle> &simulatedParticlesInitial,
      std::vector<ActsFatras::Particle> &simulatedParticlesFinal,
      std::vector<ActsFatras::Hit> &simHits) const final {
    return simulation.simulate(geoCtx, magCtx, rng, inputParticles,
                               simulatedParticlesInitial,
                               simulatedParticlesFinal, simHits);
  }
};

}  // namespace

ActsExamples::FatrasSimulation::FatrasSimulation(Config cfg,
                                                 Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("FatrasSimulation", lvl), m_cfg(std::move(cfg)) {
  ACTS_DEBUG("hits on sensitive surfaces: " << m_cfg.generateHitsOnSensitive);
  ACTS_DEBUG("hits on material surfaces: " << m_cfg.generateHitsOnMaterial);
  ACTS_DEBUG("hits on passive surfaces: " << m_cfg.generateHitsOnPassive);

  if (!m_cfg.generateHitsOnSensitive && !m_cfg.generateHitsOnMaterial &&
      !m_cfg.generateHitsOnPassive) {
    ACTS_WARNING("FatrasSimulation not configured to generate any hits!");
  }

  if (!m_cfg.trackingGeometry) {
    throw std::invalid_argument{"Missing tracking geometry"};
  }
  if (!m_cfg.magneticField) {
    throw std::invalid_argument{"Missing magnetic field"};
  }
  if (!m_cfg.randomNumbers) {
    throw std::invalid_argument("Missing random numbers tool");
  }

  // construct the simulation for the specific magnetic field
  m_sim = std::make_unique<FatrasSimulationT>(m_cfg, lvl);

  m_inputParticles.initialize(m_cfg.inputParticles);
  m_outputParticles.initialize(m_cfg.outputParticles);
  m_outputSimHits.initialize(m_cfg.outputSimHits);
}

// explicit destructor needed for the PIMPL implementation to work
ActsExamples::FatrasSimulation::~FatrasSimulation() = default;

ActsExamples::ProcessCode ActsExamples::FatrasSimulation::execute(
    const AlgorithmContext &ctx) const {
  // read input containers
  const auto &inputParticles = m_inputParticles(ctx);

  ACTS_DEBUG(inputParticles.size() << " input particles");

  // prepare input container
  std::vector<ActsFatras::Particle> particlesInput;
  particlesInput.reserve(inputParticles.size());
  for (const auto &p : inputParticles) {
    particlesInput.push_back(p.initial());
  }

  // prepare output containers
  std::vector<ActsFatras::Particle> particlesInitialUnordered;
  std::vector<ActsFatras::Particle> particlesFinalUnordered;
  std::vector<ActsFatras::Hit> simHitsUnordered;
  // reserve appropriate resources
  particlesInitialUnordered.reserve(inputParticles.size());
  particlesFinalUnordered.reserve(inputParticles.size());
  simHitsUnordered.reserve(inputParticles.size() *
                           m_cfg.averageHitsPerParticle);

  // run the simulation w/ a local random generator
  auto rng = m_cfg.randomNumbers->spawnGenerator(ctx);
  auto ret = m_sim->simulate(ctx.geoContext, ctx.magFieldContext, rng,
                             particlesInput, particlesInitialUnordered,
                             particlesFinalUnordered, simHitsUnordered);
  // fatal error leads to panic
  if (!ret.ok()) {
    ACTS_FATAL("event " << ctx.eventNumber << " simulation failed with error "
                        << ret.error());
    return ProcessCode::ABORT;
  }
  // failed particles are just logged. assumes that failed particles are due
  // to edge-cases representing a tiny fraction of the event; not due to a
  // fundamental issue.
  for (const auto &failed : ret.value()) {
    ACTS_ERROR("event " << ctx.eventNumber << " particle " << failed.particle
                        << " failed to simulate with error " << failed.error
                        << ": " << failed.error.message());
  }

  ACTS_DEBUG(particlesInitialUnordered.size()
             << " simulated particles (initial state)");
  ACTS_DEBUG(particlesFinalUnordered.size()
             << " simulated particles (final state)");
  ACTS_DEBUG(simHitsUnordered.size() << " simulated hits");

  if (particlesInitialUnordered.size() != particlesFinalUnordered.size()) {
    ACTS_ERROR("number of initial and final state particles differ");
  }

  // order output containers
#if BOOST_VERSION >= 107800
  SimParticleStateContainer particlesInitial(particlesInitialUnordered.begin(),
                                             particlesInitialUnordered.end());
  SimParticleStateContainer particlesFinal(particlesFinalUnordered.begin(),
                                           particlesFinalUnordered.end());
  SimHitContainer simHits(simHitsUnordered.begin(), simHitsUnordered.end());
#else
  // working around a nasty boost bug
  // https://github.com/boostorg/container/issues/244

  SimParticleStateContainer particlesInitial;
  SimParticleStateContainer particlesFinal;
  SimHitContainer simHits;

  particlesInitial.reserve(particlesInitialUnordered.size());
  particlesFinal.reserve(particlesFinalUnordered.size());
  simHits.reserve(simHitsUnordered.size());

  for (const auto &p : particlesInitialUnordered) {
    particlesInitial.insert(p);
  }
  for (const auto &p : particlesFinalUnordered) {
    particlesFinal.insert(p);
  }
  for (const auto &h : simHitsUnordered) {
    simHits.insert(h);
  }
#endif

  SimParticleContainer particlesSimulated;
  particlesSimulated.reserve(particlesInitial.size());
  for (const auto &particleInitial : particlesInitial) {
    SimParticle particleSimulated(particleInitial, particleInitial);

    if (auto it = particlesFinal.find(particleInitial.particleId());
        it != particlesFinal.end()) {
      particleSimulated.final() = *it;
    } else {
      ACTS_ERROR("particle " << particleInitial.particleId()
                             << " has no final state");
    }

    particlesSimulated.insert(particleSimulated);
  }

  // store ordered output containers
  m_outputParticles(ctx, std::move(particlesSimulated));
  m_outputSimHits(ctx, std::move(simHits));

  return ActsExamples::ProcessCode::SUCCESS;
}
