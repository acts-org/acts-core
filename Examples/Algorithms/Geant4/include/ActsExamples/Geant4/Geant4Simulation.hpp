// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/DetectorCommons/Detector.hpp"
#include "ActsExamples/EventData/PropagationSummary.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/Geant4/Geant4ConstructionOptions.hpp"
#include "ActsExamples/Geant4/SensitiveSurfaceMapper.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

class G4RunManager;
class G4VUserPrimaryGeneratorAction;
class G4VUserDetectorConstruction;
class G4UserRunAction;
class G4UserEventAction;
class G4UserTrackingAction;
class G4UserSteppingAction;
class G4MagneticField;
class G4VUserPhysicsList;
class G4FieldManager;

namespace Acts {
class MagneticFieldProvider;
class Volume;
}  // namespace Acts

namespace ActsExamples {
struct Geant4Handle;

namespace Geant4 {
class SensitiveSurfaceMapper;
struct EventStore;
}  // namespace Geant4

/// Abstracts common Geant4 Acts algorithm behaviour.
class Geant4SimulationBase : public IAlgorithm {
 public:
  /// Nested configuration struct for the Geant4 simulation
  struct Config {
    // Name of the input particle collection
    std::string inputParticles = "";

    /// Random number service.
    std::shared_ptr<const RandomNumbers> randomNumbers;

    /// Geant4 construction options.
    Geant4ConstructionOptions constructionOptions;

    /// Detector instance to access Geant4 geometry construction.
    std::shared_ptr<Detector> detector;

    /// Optional Geant4 instance overwrite.
    std::shared_ptr<Geant4Handle> geant4Handle;
  };

  Geant4SimulationBase(const Config& cfg, std::string name,
                       Acts::Logging::Level level);

  ~Geant4SimulationBase() override;

  /// Initialize the algorithm
  ProcessCode initialize() final;

  /// Algorithm execute method, called once per event with context
  ///
  /// @param ctx the AlgorithmContext for this event
  ProcessCode execute(const ActsExamples::AlgorithmContext& ctx) const override;

  /// Readonly access to the configuration
  virtual const Config& config() const = 0;

  std::shared_ptr<Geant4Handle> geant4Handle() const;

 protected:
  void commonInitialization();

  G4RunManager& runManager() const;

  Geant4::EventStore& eventStore() const;

  std::unique_ptr<const Acts::Logger> m_logger;

  std::shared_ptr<Geant4::EventStore> m_eventStore;

  int m_geant4Level{};

  std::shared_ptr<Geant4Handle> m_geant4Instance;

  /// Detector construction object.
  /// G4RunManager will take care of deletion
  G4VUserDetectorConstruction* m_detectorConstruction{};

  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
};

/// Algorithm to run Geant4 simulation in the ActsExamples framework
class Geant4Simulation final : public Geant4SimulationBase {
 public:
  using SensitiveCandidates = std::function<std::vector<const Acts::Surface*>(
      const Acts::GeometryContext&, const Acts::Vector3&)>;

  struct Config : public Geant4SimulationBase::Config {
    /// Name of the output collection : hits
    std::string outputSimHits = "simhits";

    /// Name of the output collection : simulated particles
    std::string outputParticles = "particles_simulated";

    /// Name of the output collection : propagation records (debugging)
    std::string outputPropagationSummaries = "propagation_summaries";

    /// The ACTS sensitive surfaces in a mapper, used for hit creation
    std::shared_ptr<const Geant4::SensitiveSurfaceMapper>
        sensitiveSurfaceMapper = nullptr;

    /// The ACTS Magnetic field provider
    std::shared_ptr<const Acts::MagneticFieldProvider> magneticField = nullptr;

    /// If a physics list has to be instantiated this one is chosen.
    std::string physicsList = "FTFP_BERT";

    std::shared_ptr<const Acts::Volume> killVolume;
    double killAfterTime = std::numeric_limits<double>::infinity();
    bool killSecondaries = false;

    bool recordHitsOfCharged = true;

    bool recordHitsOfNeutrals = false;

    bool recordHitsOfPrimaries = true;

    bool recordHitsOfSecondaries = true;

    bool keepParticlesWithoutHits = true;

    bool recordPropagationSummaries = false;
  };

  /// Simulation constructor
  ///
  /// @param config is the configuration struct
  /// @param level is the logging level to be used
  Geant4Simulation(const Config& cfg,
                   Acts::Logging::Level level = Acts::Logging::INFO);

  ~Geant4Simulation() override;

  /// Algorithm execute method, called once per event with context
  ///
  /// @param ctx the AlgorithmContext for this event
  ProcessCode execute(const ActsExamples::AlgorithmContext& ctx) const final;

  /// Readonly access to the configuration
  const Config& config() const final { return m_cfg; }

 private:
  Config m_cfg;

  /// The (wrapped) ACTS Magnetic field provider as a Geant4 module
  std::unique_ptr<G4MagneticField> m_magneticField;
  std::unique_ptr<G4FieldManager> m_fieldManager;

  WriteDataHandle<SimParticleContainer> m_outputParticles{this,
                                                          "OutputParticles"};
  WriteDataHandle<SimHitContainer> m_outputSimHits{this, "OutputSimHIts"};

  WriteDataHandle<PropagationSummaries> m_outputPropagationSummaries{
      this, "OutputPropagationSummaries"};
};

class Geant4MaterialRecording final : public Geant4SimulationBase {
 public:
  struct Config : public Geant4SimulationBase::Config {
    /// Name of the output collection: material tracks
    std::string outputMaterialTracks = "material_tracks";

    /// Materials to exclude from the recording.
    std::vector<std::string> excludeMaterials = {"Air", "Vacuum"};
  };

  /// Material recording constructor
  ///
  /// @param config is the configuration struct
  /// @param level is the logging level to be used
  Geant4MaterialRecording(const Config& cfg,
                          Acts::Logging::Level level = Acts::Logging::INFO);

  ~Geant4MaterialRecording() override;

  /// Algorithm execute method, called once per event with context
  ///
  /// @param ctx the AlgorithmContext for this event
  ProcessCode execute(const ActsExamples::AlgorithmContext& ctx) const override;

  /// Readonly access to the configuration
  const Config& config() const final { return m_cfg; }

 private:
  Config m_cfg;

  WriteDataHandle<std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>>
      m_outputMaterialTracks{this, "OutputMaterialTracks"};
};

}  // namespace ActsExamples
