// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorElement.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorSurfaceFactory.hpp"
#include "Acts/Plugins/Geant4/Geant4PhysicalVolumeSelectors.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Surfaces/SurfaceVisitorConcept.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"
#include "ActsExamples/Geant4/DetectorConstructionFactory.hpp"
#include "ActsExamples/Geant4/GdmlDetectorConstruction.hpp"
#include "ActsExamples/Geant4/Geant4Manager.hpp"
#include "ActsExamples/Geant4/Geant4Simulation.hpp"
#include "ActsExamples/Geant4/RegionCreator.hpp"
#include "ActsExamples/Geant4/SensitiveSurfaceMapper.hpp"
#include "ActsExamples/Geant4Detector/Geant4Detector.hpp"
#include "ActsExamples/MuonSpectrometerMockupDetector/MockupSectorBuilder.hpp"
#include "ActsExamples/TelescopeDetector/TelescopeDetector.hpp"
#include "ActsExamples/TelescopeDetector/TelescopeG4DetectorConstruction.hpp"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <G4RunManager.hh>
#include <G4Transform3D.hh>
#include <G4UserEventAction.hh>
#include <G4UserRunAction.hh>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

class G4UserSteppingAction;
class G4VUserPhysicsList;
namespace Acts {
class MagneticFieldProvider;
class TrackingGeometry;
class Volume;
}  // namespace Acts
namespace ActsExamples {
class RandomNumbers;
}  // namespace ActsExamples

namespace py = pybind11;
using namespace pybind11::literals;

using namespace ActsExamples;
using namespace Acts;
using namespace Acts::Python;

namespace Acts::Python {
void addGeant4HepMC3(Context& ctx);
}

struct ExperimentalSensitiveCandidates : public SensitiveCandidatesBase {
  std::shared_ptr<const Experimental::Detector> detector;

  /// Find the sensitive surfaces for a given position
  std::vector<const Acts::Surface*> queryPosition(
      const Acts::GeometryContext& gctx,
      const Acts::Vector3& position) const override {
    std::vector<const Acts::Surface*> surfaces;
    // Here's the detector volume
    auto volume = detector->findDetectorVolume(gctx, position);
    if (volume != nullptr) {
      for (const auto& surface : volume->surfaces()) {
        if (surface->associatedDetectorElement() != nullptr) {
          surfaces.push_back(surface);
        }
      }
    }
    return surfaces;
  }

  std::vector<const Acts::Surface*> queryAll() const override {
    std::vector<const Acts::Surface*> surfaces;
    detector->visitSurfaces([&](const Acts::Surface* surface) {
      if (surface->associatedDetectorElement() != nullptr) {
        surfaces.push_back(surface);
      }
    });
    return surfaces;
  }
};

PYBIND11_MODULE(ActsPythonBindingsGeant4, mod) {
  py::class_<DetectorConstructionFactory,
             std::shared_ptr<DetectorConstructionFactory>>(
      mod, "DetectorConstructionFactory");

  py::class_<Geant4Manager, std::unique_ptr<Geant4Manager, py::nodelete>>(
      mod, "Geant4Manager")
      .def_static("instance", &Geant4Manager::instance,
                  py::return_value_policy::reference)
      .def("currentHandle", &Geant4Manager::currentHandle);

  py::class_<Geant4Handle, std::shared_ptr<Geant4Handle>>(mod, "Geant4Handle")
      .def("tweakLogging", &Geant4Handle::tweakLogging);

  {
    using Algorithm = Geant4SimulationBase;
    using Config = Algorithm::Config;
    auto alg =
        py::class_<Algorithm, IAlgorithm, std::shared_ptr<Algorithm>>(
            mod, "Geant4SimulationBase")
            .def_property_readonly("geant4Handle", &Algorithm::geant4Handle);

    auto c1 = py::class_<Config, std::shared_ptr<Config>>(alg, "Config")
                  .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c1, Config);
    ACTS_PYTHON_MEMBER(inputParticles);
    ACTS_PYTHON_MEMBER(randomNumbers);
    ACTS_PYTHON_MEMBER(detectorConstructionFactory);
    ACTS_PYTHON_MEMBER(geant4Handle);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    using Config = SensitiveSurfaceMapper::Config;
    using State = SensitiveSurfaceMapper::State;
    auto sm =
        py::class_<SensitiveSurfaceMapper,
                   std::shared_ptr<SensitiveSurfaceMapper>>(
            mod, "SensitiveSurfaceMapper")
            .def(py::init([](const Config& cfg, Acts::Logging::Level level) {
              return std::make_shared<SensitiveSurfaceMapper>(
                  cfg, getDefaultLogger("SensitiveSurfaceMapper", level));
            }));

    py::class_<State>(sm, "State").def(py::init<>());

    auto c = py::class_<Config>(sm, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(materialMappings);
    ACTS_PYTHON_MEMBER(volumeMappings);
    ACTS_PYTHON_MEMBER(candidateSurfaces);
    ACTS_PYTHON_STRUCT_END();

    sm.def("create",
           [](const Config& cfg, Acts::Logging::Level level,
              const std::shared_ptr<const TrackingGeometry> tGeometry) {
             // Set a new surface finder
             Config ccfg = cfg;
             auto candidateSurfaces =
                 std::make_shared<ActsExamples::SensitiveCandidates>();
             candidateSurfaces->trackingGeometry = tGeometry;
             ccfg.candidateSurfaces = candidateSurfaces;
             return std::make_shared<SensitiveSurfaceMapper>(
                 ccfg, getDefaultLogger("SensitiveSurfaceMapper", level));
           });

    sm.def("create",
           [](const Config& cfg, Acts::Logging::Level level,
              const std::shared_ptr<const Experimental::Detector>& detector) {
             // Helper struct to find the sensitive surface candidates

             // Set a new surface finder
             Config ccfg = cfg;
             auto candidateSurfaces =
                 std::make_shared<ExperimentalSensitiveCandidates>();
             candidateSurfaces->detector = detector;
             ccfg.candidateSurfaces = candidateSurfaces;
             return std::make_shared<SensitiveSurfaceMapper>(
                 ccfg, getDefaultLogger("SensitiveSurfaceMapper", level));
           });

    sm.def(
        "remapSensitiveNames",
        [](SensitiveSurfaceMapper& self, State& state, GeometryContext& gctx,
           DetectorConstructionFactory& factory, Transform3& transform) {
          return self.remapSensitiveNames(
              state, gctx, factory.factorize()->Construct(), transform);
        },
        "state"_a, "gctx"_a, "g4physicalVolume"_a, "motherTransform"_a);
    sm.def("checkMapping", &SensitiveSurfaceMapper::checkMapping, "state"_a,
           "gctx"_a, "writeMappedAsObj"_a, "writeMissingAsObj"_a);
  }

  {
    using Algorithm = Geant4Simulation;
    using Config = Algorithm::Config;
    auto alg =
        py::class_<Algorithm, Geant4SimulationBase, std::shared_ptr<Algorithm>>(
            mod, "Geant4Simulation")
            .def(py::init<const Config&, Acts::Logging::Level>(),
                 py::arg("config"), py::arg("level"))
            .def_property_readonly("config", &Algorithm::config);

    auto c1 = py::class_<Config, Geant4SimulationBase::Config,
                         std::shared_ptr<Config>>(alg, "Config")
                  .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c1, Config);
    ACTS_PYTHON_MEMBER(outputSimHits);
    ACTS_PYTHON_MEMBER(outputParticlesInitial);
    ACTS_PYTHON_MEMBER(outputParticlesFinal);
    ACTS_PYTHON_MEMBER(sensitiveSurfaceMapper);
    ACTS_PYTHON_MEMBER(magneticField);
    ACTS_PYTHON_MEMBER(physicsList);
    ACTS_PYTHON_MEMBER(killVolume);
    ACTS_PYTHON_MEMBER(killAfterTime);
    ACTS_PYTHON_MEMBER(killSecondaries);
    ACTS_PYTHON_MEMBER(recordHitsOfNeutrals);
    ACTS_PYTHON_MEMBER(recordHitsOfSecondaries);
    ACTS_PYTHON_MEMBER(keepParticlesWithoutHits);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    using Algorithm = Geant4MaterialRecording;
    using Config = Algorithm::Config;
    auto alg =
        py::class_<Algorithm, Geant4SimulationBase, std::shared_ptr<Algorithm>>(
            mod, "Geant4MaterialRecording")
            .def(py::init<const Config&, Acts::Logging::Level>(),
                 py::arg("config"), py::arg("level"))
            .def_property_readonly("config", &Algorithm::config);

    auto c = py::class_<Config, Geant4SimulationBase::Config,
                        std::shared_ptr<Config>>(alg, "Config")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(outputMaterialTracks);
    ACTS_PYTHON_MEMBER(excludeMaterials);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<GdmlDetectorConstructionFactory, DetectorConstructionFactory,
               std::shared_ptr<GdmlDetectorConstructionFactory>>(
        mod, "GdmlDetectorConstructionFactory")
        .def(py::init<std::string,
                      std::vector<std::shared_ptr<RegionCreator>>>(),
             py::arg("path"),
             py::arg("regionCreators") =
                 std::vector<std::shared_ptr<RegionCreator>>());
  }

  {
    py::class_<
        Telescope::TelescopeG4DetectorConstructionFactory,
        DetectorConstructionFactory,
        std::shared_ptr<Telescope::TelescopeG4DetectorConstructionFactory>>(
        mod, "TelescopeG4DetectorConstructionFactory")
        .def(py::init<const Telescope::TelescopeDetector::Config&,
                      std::vector<std::shared_ptr<RegionCreator>>>(),
             py::arg("cfg"),
             py::arg("regionCreators") =
                 std::vector<std::shared_ptr<RegionCreator>>());
  }

  {
    using ISelector = Acts::IGeant4PhysicalVolumeSelector;
    auto is = py::class_<ISelector, std::shared_ptr<ISelector>>(
        mod, "IVolumeSelector");

    using NameSelector = Acts::Geant4PhysicalVolumeSelectors::NameSelector;
    auto ns = py::class_<NameSelector, std::shared_ptr<NameSelector>>(
                  mod, "VolumeNameSelector", is)
                  .def(py::init<const std::vector<std::string>&, bool>());

    using Factory = Acts::Geant4DetectorSurfaceFactory;
    auto o = py::class_<Factory::Options>(mod, "SurfaceFactoryOptions")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(o, Factory::Options);
    ACTS_PYTHON_MEMBER(scaleConversion);
    ACTS_PYTHON_MEMBER(convertMaterial);
    ACTS_PYTHON_MEMBER(convertedMaterialThickness);
    ACTS_PYTHON_MEMBER(sensitiveSurfaceSelector);
    ACTS_PYTHON_MEMBER(passiveSurfaceSelector);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<Acts::Geant4DetectorElement,
               std::shared_ptr<Acts::Geant4DetectorElement>>(
        mod, "Geant4DetectorElement");

    using Geant4Detector = Geant4::Geant4Detector;

    auto g =
        py::class_<Geant4Detector, std::shared_ptr<Geant4Detector>>(
            mod, "Geant4Detector")
            .def(py::init<>())
            .def(
                "constructDetector",
                [](Geant4Detector& self, const Geant4Detector::Config& cfg,
                   Logging::Level logLevel) {
                  auto logger = getDefaultLogger("Geant4Detector", logLevel);
                  return self.constructDetector(cfg, *logger);
                },
                py::arg("cfg"), py::arg("logLevel") = Logging::INFO)
            .def(
                "constructTrackingGeometry",
                [](Geant4Detector& self, const Geant4Detector::Config& cfg,
                   Logging::Level logLevel) {
                  auto logger = getDefaultLogger("Geant4Detector", logLevel);
                  return self.constructTrackingGeometry(cfg, *logger);
                },
                py::arg("cfg"), py::arg("logLevel") = Logging::INFO);

    auto c = py::class_<Geant4Detector::Config>(g, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Geant4Detector::Config);
    ACTS_PYTHON_MEMBER(name);
    ACTS_PYTHON_MEMBER(g4World);
    ACTS_PYTHON_MEMBER(g4SurfaceOptions);
    ACTS_PYTHON_MEMBER(protoDetector);
    ACTS_PYTHON_MEMBER(geometryIdentifierHook);
    ACTS_PYTHON_MEMBER(logLevel);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    /// Helper function to test if the automatic geometry conversion works
    ///
    /// @param gdmlFileName is the name of the GDML file
    /// @param sensitiveMatches is a list of strings to match sensitive volumes
    /// @param passiveMatches is a list of strings to match passive volumes
    mod.def("convertSurfaces", [](const std::string& gdmlFileName,
                                  const std::vector<std::string>&
                                      sensitiveMatches,
                                  const std::vector<std::string>&
                                      passiveMatches) {
      // Initiate the detector construction & retrieve world
      ActsExamples::GdmlDetectorConstruction gdmlContruction(gdmlFileName);
      const auto* world = gdmlContruction.Construct();

      // Create the selectors
      auto sensitiveSelectors =
          std::make_shared<Acts::Geant4PhysicalVolumeSelectors::NameSelector>(
              sensitiveMatches, false);
      auto passiveSelectors =
          std::make_shared<Acts::Geant4PhysicalVolumeSelectors::NameSelector>(
              passiveMatches, false);

      Acts::Geant4DetectorSurfaceFactory::Cache cache;
      Acts::Geant4DetectorSurfaceFactory::Options options;
      options.sensitiveSurfaceSelector = sensitiveSelectors;
      options.passiveSurfaceSelector = passiveSelectors;

      G4Transform3D nominal;
      Acts::Geant4DetectorSurfaceFactory factory;
      factory.construct(cache, nominal, *world, options);

      // Capture the sensitive elements and the surfaces
      using Elements =
          std::vector<std::shared_ptr<Acts::Geant4DetectorElement>>;
      Elements detectorElements;
      detectorElements.reserve(cache.sensitiveSurfaces.size());
      using Surfaces = std::vector<std::shared_ptr<Acts::Surface>>;
      Surfaces surfaces;
      surfaces.reserve(cache.sensitiveSurfaces.size());
      std::for_each(cache.sensitiveSurfaces.begin(),
                    cache.sensitiveSurfaces.end(), [&](const auto& sensitive) {
                      detectorElements.push_back(std::get<0>(sensitive));
                      surfaces.push_back(std::get<1>(sensitive));
                    });

      // Capture the passive surfaces
      Surfaces passiveSurfaces;
      passiveSurfaces.reserve(cache.passiveSurfaces.size());
      for (const auto& passive : cache.passiveSurfaces) {
        passiveSurfaces.push_back(passive);
      }

      // Return a convenient tuple for drawing
      return std::tuple<Elements, Surfaces, Surfaces>(
          std::move(detectorElements), std::move(surfaces),
          std::move(passiveSurfaces));
    });
  }

  {
    using MockupSectorBuilder = MockupSectorBuilder;
    using Config = MockupSectorBuilder::Config;
    using ChamberConfig = MockupSectorBuilder::ChamberConfig;

    auto ms =
        py::class_<MockupSectorBuilder, std::shared_ptr<MockupSectorBuilder>>(
            mod, "MockupSectorBuilder")
            .def(py::init<const Config&>())
            .def("buildChamber", &MockupSectorBuilder::buildChamber)
            .def("buildSector", &MockupSectorBuilder::buildSector)
            .def("drawSector", &MockupSectorBuilder::drawSector);

    auto c = py::class_<Config>(ms, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(gdmlPath);
    ACTS_PYTHON_MEMBER(NumberOfSectors);
    ACTS_PYTHON_MEMBER(toleranceOverlap);
    ACTS_PYTHON_STRUCT_END();

    auto cch = py::class_<ChamberConfig>(ms, "ChamberConfig").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(cch, ChamberConfig);
    ACTS_PYTHON_MEMBER(name);
    ACTS_PYTHON_MEMBER(SensitiveNames);
    ACTS_PYTHON_MEMBER(PassiveNames);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    using Tool = RegionCreator;
    using Config = Tool::Config;
    auto tool = py::class_<Tool, std::shared_ptr<Tool>>(mod, "RegionCreator")
                    .def(py::init<const Config&, std::string, Logging::Level>(),
                         py::arg("config"), py::arg("name"),
                         py::arg("logLevel") = Logging::INFO)
                    .def_property_readonly("config", &Tool::config);

    auto c = py::class_<Config>(tool, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(gammaCut);
    ACTS_PYTHON_MEMBER(electronCut);
    ACTS_PYTHON_MEMBER(positronCut);
    ACTS_PYTHON_MEMBER(protonCut);
    ACTS_PYTHON_MEMBER(volumes);
    ACTS_PYTHON_STRUCT_END();
  }

  Acts::Python::Context ctx;
  ctx.modules["geant4"] = mod;

  addGeant4HepMC3(ctx);
}
