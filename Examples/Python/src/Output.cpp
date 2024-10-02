// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Visualization/ViewConfig.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Io/Csv/CsvBFieldWriter.hpp"
#include "ActsExamples/Io/Csv/CsvExaTrkXGraphWriter.hpp"
#include "ActsExamples/Io/Csv/CsvMeasurementWriter.hpp"
#include "ActsExamples/Io/Csv/CsvParticleWriter.hpp"
#include "ActsExamples/Io/Csv/CsvProtoTrackWriter.hpp"
#include "ActsExamples/Io/Csv/CsvSeedWriter.hpp"
#include "ActsExamples/Io/Csv/CsvSimHitWriter.hpp"
#include "ActsExamples/Io/Csv/CsvSpacePointsBucketWriter.hpp"
#include "ActsExamples/Io/Csv/CsvSpacepointWriter.hpp"
#include "ActsExamples/Io/Csv/CsvTrackParameterWriter.hpp"
#include "ActsExamples/Io/Csv/CsvTrackWriter.hpp"
#include "ActsExamples/Io/Csv/CsvTrackingGeometryWriter.hpp"
#include "ActsExamples/Io/NuclearInteractions/RootNuclearInteractionParametersWriter.hpp"
#include "ActsExamples/Io/Performance/CKFPerformanceWriter.hpp"
#include "ActsExamples/Io/Performance/SeedingPerformanceWriter.hpp"
#include "ActsExamples/Io/Performance/TrackFinderPerformanceWriter.hpp"
#include "ActsExamples/Io/Performance/TrackFitterPerformanceWriter.hpp"
#include "ActsExamples/Io/Performance/VertexPerformanceWriter.hpp"
#include "ActsExamples/Io/Root/RootBFieldWriter.hpp"
#include "ActsExamples/Io/Root/RootMaterialTrackWriter.hpp"
#include "ActsExamples/Io/Root/RootMaterialWriter.hpp"
#include "ActsExamples/Io/Root/RootMeasurementWriter.hpp"
#include "ActsExamples/Io/Root/RootParticleWriter.hpp"
#include "ActsExamples/Io/Root/RootPropagationStepsWriter.hpp"
#include "ActsExamples/Io/Root/RootPropagationSummaryWriter.hpp"
#include "ActsExamples/Io/Root/RootSeedWriter.hpp"
#include "ActsExamples/Io/Root/RootSimHitWriter.hpp"
#include "ActsExamples/Io/Root/RootSpacepointWriter.hpp"
#include "ActsExamples/Io/Root/RootTrackParameterWriter.hpp"
#include "ActsExamples/Io/Root/RootTrackStatesWriter.hpp"
#include "ActsExamples/Io/Root/RootTrackSummaryWriter.hpp"
#include "ActsExamples/Io/Root/RootVertexWriter.hpp"
#include "ActsExamples/MaterialMapping/IMaterialWriter.hpp"
#include "ActsExamples/Plugins/Obj/ObjPropagationStepsWriter.hpp"
#include "ActsExamples/Plugins/Obj/ObjTrackingGeometryWriter.hpp"

#include <array>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace Acts {
class TrackingGeometry;
namespace detail {
struct Step;
}  // namespace detail
}  // namespace Acts
namespace ActsExamples {
class IWriter;
struct AlgorithmContext;
}  // namespace ActsExamples

namespace py = pybind11;
using namespace pybind11::literals;

using namespace ActsExamples;

namespace {
template <ActsExamples::CsvBFieldWriter::CoordinateType CType, bool Grid>
void register_csv_bfield_writer_binding(
    pybind11::class_<ActsExamples::CsvBFieldWriter>& w) {
  std::string name =
      std::string(CType == ActsExamples::CsvBFieldWriter::CoordinateType::XYZ
                      ? "Xyz"
                      : "Rz") +
      std::string(Grid ? "Grid" : "Gridless");

  using Config = ActsExamples::CsvBFieldWriter::Config<CType, Grid>;
  w.def_static((std::string("run") + name).c_str(),
               [](const Config& config, Acts::Logging::Level level) {
                 ActsExamples::CsvBFieldWriter::run(config, level);
               },
               py::arg("config"), py::arg("level"));
  auto c = py::class_<Config>(w, (std::string("Config") + name).c_str())
               .def(py::init<>());
  ACTS_PYTHON_STRUCT_BEGIN(c, Config);
  ACTS_PYTHON_MEMBER(fileName);
  ACTS_PYTHON_MEMBER(bField);
  ACTS_PYTHON_MEMBER(range);
  ACTS_PYTHON_MEMBER(bins);
  ACTS_PYTHON_STRUCT_END();
}
}  // namespace

namespace Acts::Python {

void addOutput(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::ObjPropagationStepsWriter, mex,
                             "ObjPropagationStepsWriter", collection, outputDir,
                             outputScalor, outputPrecision);

  {
    auto c = py::class_<ViewConfig>(m, "ViewConfig").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, ViewConfig);
    ACTS_PYTHON_MEMBER(visible);
    ACTS_PYTHON_MEMBER(color);
    ACTS_PYTHON_MEMBER(offset);
    ACTS_PYTHON_MEMBER(lineThickness);
    ACTS_PYTHON_MEMBER(surfaceThickness);
    ACTS_PYTHON_MEMBER(nSegments);
    ACTS_PYTHON_MEMBER(triangulate);
    ACTS_PYTHON_MEMBER(outputName);
    ACTS_PYTHON_STRUCT_END();

    patchKwargsConstructor(c);

    py::class_<Color>(m, "Color")
        .def(py::init<>())
        .def(py::init<int, int, int>())
        .def(py::init<double, double, double>())
        .def(py::init<std::string>())
        .def_readonly("rgb", &Color::rgb);
  }

  {
    using Writer = ActsExamples::ObjTrackingGeometryWriter;
    auto w = py::class_<Writer, std::shared_ptr<Writer>>(
                 mex, "ObjTrackingGeometryWriter")
                 .def(py::init<const Writer::Config&, Acts::Logging::Level>(),
                      py::arg("config"), py::arg("level"))
                 .def("write", py::overload_cast<const AlgorithmContext&,
                                                 const Acts::TrackingGeometry&>(
                                   &Writer::write));

    auto c = py::class_<Writer::Config>(w, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Writer::Config);
    ACTS_PYTHON_MEMBER(outputScalor);
    ACTS_PYTHON_MEMBER(outputPrecision);
    ACTS_PYTHON_MEMBER(outputDir);
    ACTS_PYTHON_MEMBER(containerView);
    ACTS_PYTHON_MEMBER(volumeView);
    ACTS_PYTHON_MEMBER(sensitiveView);
    ACTS_PYTHON_MEMBER(passiveView);
    ACTS_PYTHON_MEMBER(gridView);
    ACTS_PYTHON_STRUCT_END();
  }

  // Bindings for the binning in e.g., CKFPerformanceWriter
  {
    py::class_<PlotHelpers::Binning>(mex, "Binning")
        .def(py::init<std::string, int, double, double>(), "title"_a, "bins"_a,
             "bMin"_a, "bMax"_a)
        .def(py::init<std::string, std::vector<double>>(), "title"_a, "bins"_a);

    py::class_<EffPlotTool::Config>(mex, "EffPlotToolConfig")
        .def(py::init<std::map<std::string, PlotHelpers::Binning>>(),
             "varBinning"_a);

    py::class_<FakeRatePlotTool::Config>(mex, "FakeRatePlotToolConfig")
        .def(py::init<std::map<std::string, PlotHelpers::Binning>>(),
             "varBinning"_a);

    py::class_<DuplicationPlotTool::Config>(mex, "DuplicationPlotToolConfig")
        .def(py::init<std::map<std::string, PlotHelpers::Binning>>(),
             "varBinning"_a);
  }

  // ROOT WRITERS
  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::RootPropagationStepsWriter, mex,
                             "RootPropagationStepsWriter", collection, filePath,
                             fileMode);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::RootPropagationSummaryWriter, mex,
                             "RootPropagationSummaryWriter",
                             inputSummaryCollection, filePath, fileMode);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::RootParticleWriter, mex,
                             "RootParticleWriter", inputParticles,
                             inputFinalParticles, filePath, fileMode, treeName);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::RootVertexWriter, mex,
                             "RootVertexWriter", inputVertices, filePath,
                             fileMode, treeName);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::TrackFinderPerformanceWriter, mex,
                             "TrackFinderPerformanceWriter", inputTracks,
                             inputParticles, inputMeasurementParticlesMap,
                             inputTrackParticleMatching, filePath, fileMode,
                             treeNameTracks, treeNameParticles);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::TrackFitterPerformanceWriter, mex,
                             "TrackFitterPerformanceWriter", inputTracks,
                             inputParticles, inputTrackParticleMatching,
                             filePath, resPlotToolConfig, effPlotToolConfig,
                             trackSummaryPlotToolConfig);

  ACTS_PYTHON_DECLARE_WRITER(
      ActsExamples::SeedingPerformanceWriter, mex, "SeedingPerformanceWriter",
      inputSeeds, inputMeasurementParticlesMap, inputParticles, filePath,
      fileMode, effPlotToolConfig, duplicationPlotToolConfig);

  ACTS_PYTHON_DECLARE_WRITER(
      ActsExamples::RootTrackParameterWriter, mex, "RootTrackParameterWriter",
      inputTrackParameters, inputProtoTracks, inputParticles, inputSimHits,
      inputMeasurementParticlesMap, inputMeasurementSimHitsMap, filePath,
      treeName, fileMode);

  ACTS_PYTHON_DECLARE_WRITER(
      ActsExamples::RootMaterialTrackWriter, mex, "RootMaterialTrackWriter",
      inputMaterialTracks, filePath, fileMode, treeName, recalculateTotals,
      prePostStep, storeSurface, storeVolume, collapseInteractions);

  {
    using Writer = ActsExamples::RootBFieldWriter;
    auto w =
        py::class_<Writer>(mex, "RootBFieldWriter")
            .def_static(
                "run",
                [](const Writer::Config& config, Acts::Logging::Level level) {
                  Writer::run(config, Acts::getDefaultLogger("RootBFieldWriter",
                                                             level));
                },
                py::arg("config"), py::arg("level"));

    py::enum_<Writer::GridType>(w, "GridType")
        .value("rz", Writer::GridType::rz)
        .value("xyz", Writer::GridType::xyz);

    auto c = py::class_<Writer::Config>(w, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Writer::Config);
    ACTS_PYTHON_MEMBER(treeName);
    ACTS_PYTHON_MEMBER(fileName);
    ACTS_PYTHON_MEMBER(fileMode);
    ACTS_PYTHON_MEMBER(bField);
    ACTS_PYTHON_MEMBER(gridType);
    ACTS_PYTHON_MEMBER(rBounds);
    ACTS_PYTHON_MEMBER(zBounds);
    ACTS_PYTHON_MEMBER(rBins);
    ACTS_PYTHON_MEMBER(zBins);
    ACTS_PYTHON_MEMBER(phiBins);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    using Writer = ActsExamples::RootMeasurementWriter;
    auto w = py::class_<Writer, IWriter, std::shared_ptr<Writer>>(
                 mex, "RootMeasurementWriter")
                 .def(py::init<const Writer::Config&, Acts::Logging::Level>(),
                      py::arg("config"), py::arg("level"));

    auto c = py::class_<Writer::Config>(w, "Config").def(py::init<>());

    c.def("addBoundIndicesFromDigiConfig",
          [](Writer::Config& self, const DigitizationConfig& digiCfg) {
            self.boundIndices =
                Acts::GeometryHierarchyMap<std::vector<Acts::BoundIndices>>(
                    digiCfg.getBoundIndices());
          });

    ACTS_PYTHON_STRUCT_BEGIN(c, Writer::Config);
    ACTS_PYTHON_MEMBER(inputMeasurements);
    ACTS_PYTHON_MEMBER(inputClusters);
    ACTS_PYTHON_MEMBER(inputSimHits);
    ACTS_PYTHON_MEMBER(inputMeasurementSimHitsMap);
    ACTS_PYTHON_MEMBER(filePath);
    ACTS_PYTHON_MEMBER(fileMode);
    ACTS_PYTHON_MEMBER(boundIndices);
    ACTS_PYTHON_MEMBER(surfaceByIdentifier);
    ACTS_PYTHON_STRUCT_END();
  }

  py::class_<IMaterialWriter, std::shared_ptr<IMaterialWriter>>(
      mex, "IMaterialWriter");

  {
    using Writer = ActsExamples::RootMaterialWriter;
    auto w = py::class_<Writer, IMaterialWriter, std::shared_ptr<Writer>>(
                 mex, "RootMaterialWriter")
                 .def(py::init<const Writer::Config&, Acts::Logging::Level>(),
                      py::arg("config"), py::arg("level"))
                 .def("write", py::overload_cast<const Acts::TrackingGeometry&>(
                                   &Writer::write));

    auto c = py::class_<Writer::Config>(w, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, Writer::Config);
    ACTS_PYTHON_MEMBER(processSensitives);
    ACTS_PYTHON_MEMBER(processApproaches);
    ACTS_PYTHON_MEMBER(processRepresenting);
    ACTS_PYTHON_MEMBER(processBoundaries);
    ACTS_PYTHON_MEMBER(processVolumes);
    ACTS_PYTHON_MEMBER(folderSurfaceNameBase);
    ACTS_PYTHON_MEMBER(folderVolumeNameBase);
    ACTS_PYTHON_MEMBER(voltag);
    ACTS_PYTHON_MEMBER(boutag);
    ACTS_PYTHON_MEMBER(laytag);
    ACTS_PYTHON_MEMBER(apptag);
    ACTS_PYTHON_MEMBER(sentag);
    ACTS_PYTHON_MEMBER(ntag);
    ACTS_PYTHON_MEMBER(vtag);
    ACTS_PYTHON_MEMBER(otag);
    ACTS_PYTHON_MEMBER(mintag);
    ACTS_PYTHON_MEMBER(maxtag);
    ACTS_PYTHON_MEMBER(ttag);
    ACTS_PYTHON_MEMBER(x0tag);
    ACTS_PYTHON_MEMBER(l0tag);
    ACTS_PYTHON_MEMBER(atag);
    ACTS_PYTHON_MEMBER(ztag);
    ACTS_PYTHON_MEMBER(rhotag);
    ACTS_PYTHON_MEMBER(filePath);
    ACTS_PYTHON_MEMBER(fileMode);
    ACTS_PYTHON_STRUCT_END();
  }

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::RootSeedWriter, mex,
                             "RootSeedWriter", inputSeeds, writingMode,
                             filePath, fileMode, treeName);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::RootSimHitWriter, mex,
                             "RootSimHitWriter", inputSimHits, filePath,
                             fileMode, treeName);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::RootSpacepointWriter, mex,
                             "RootSpacepointWriter", inputSpacepoints, filePath,
                             fileMode, treeName);

  ACTS_PYTHON_DECLARE_WRITER(
      ActsExamples::RootTrackStatesWriter, mex, "RootTrackStatesWriter",
      inputTracks, inputParticles, inputTrackParticleMatching, inputSimHits,
      inputMeasurementSimHitsMap, filePath, treeName, fileMode);

  ACTS_PYTHON_DECLARE_WRITER(
      ActsExamples::RootTrackSummaryWriter, mex, "RootTrackSummaryWriter",
      inputTracks, inputParticles, inputTrackParticleMatching, filePath,
      treeName, fileMode, writeCovMat, writeGsfSpecific, writeGx2fSpecific);

  ACTS_PYTHON_DECLARE_WRITER(
      ActsExamples::VertexPerformanceWriter, mex, "VertexPerformanceWriter",
      inputVertices, inputTracks, inputTruthVertices, inputParticles,
      inputSelectedParticles, inputTrackParticleMatching, bField, filePath,
      treeName, fileMode, vertexMatchThreshold, trackMatchThreshold, useTracks);

  // CSV WRITERS
  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::CsvParticleWriter, mex,
                             "CsvParticleWriter", inputParticles, outputDir,
                             outputStem, outputPrecision);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::CsvMeasurementWriter, mex,
                             "CsvMeasurementWriter", inputMeasurements,
                             inputClusters, inputMeasurementSimHitsMap,
                             outputDir, outputPrecision);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::CsvSimHitWriter, mex,
                             "CsvSimHitWriter", inputSimHits, outputDir,
                             outputStem, outputPrecision);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::CsvSpacepointWriter, mex,
                             "CsvSpacepointWriter", inputSpacepoints, outputDir,
                             outputPrecision);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::CsvSpacePointsBucketWriter, mex,
                             "CsvSpacePointsBucketWriter", inputBuckets,
                             outputDir, outputPrecision);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::CsvTrackWriter, mex,
                             "CsvTrackWriter", inputTracks, outputDir, fileName,
                             inputMeasurementParticlesMap, outputPrecision,
                             nMeasurementsMin, truthMatchProbMin, ptMin);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::CsvSeedWriter, mex, "CsvSeedWriter",
                             inputTrackParameters, inputSimSeeds, inputSimHits,
                             inputMeasurementParticlesMap,
                             inputMeasurementSimHitsMap, fileName, outputDir);

  ACTS_PYTHON_DECLARE_WRITER(
      ActsExamples::CsvTrackingGeometryWriter, mex, "CsvTrackingGeometryWriter",
      trackingGeometry, outputDir, outputPrecision, writeSensitive,
      writeBoundary, writeSurfaceGrid, writeLayerVolume, writePerEvent);

  ACTS_PYTHON_DECLARE_WRITER(
      ActsExamples::CKFPerformanceWriter, mex, "CKFPerformanceWriter",
      inputTracks, inputParticles, inputTrackParticleMatching,
      inputParticleTrackMatching, filePath, fileMode, effPlotToolConfig,
      fakeRatePlotToolConfig, duplicationPlotToolConfig,
      trackSummaryPlotToolConfig, writeMatchingDetails);

  ACTS_PYTHON_DECLARE_WRITER(
      ActsExamples::RootNuclearInteractionParametersWriter, mex,
      "RootNuclearInteractionParametersWriter", inputSimulationProcesses,
      filePath, fileMode, interactionProbabilityBins, momentumBins,
      invariantMassBins, multiplicityMax, writeOptionalHistograms,
      nSimulatedEvents);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::CsvTrackParameterWriter, mex,
                             "CsvTrackParameterWriter", inputTrackParameters,
                             inputTracks, outputDir, outputStem,
                             outputPrecision);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::CsvProtoTrackWriter, mex,
                             "CsvProtoTrackWriter", inputSpacepoints,
                             inputPrototracks, outputDir);

  {
    using Writer = ActsExamples::CsvBFieldWriter;

    auto w = py::class_<Writer>(mex, "CsvBFieldWriter");

    py::enum_<Writer::CoordinateType>(w, "CoordinateType")
        .value("rz", Writer::CoordinateType::RZ)
        .value("xyz", Writer::CoordinateType::XYZ);

    register_csv_bfield_writer_binding<Writer::CoordinateType::XYZ, true>(w);
    register_csv_bfield_writer_binding<Writer::CoordinateType::XYZ, false>(w);
    register_csv_bfield_writer_binding<Writer::CoordinateType::RZ, true>(w);
    register_csv_bfield_writer_binding<Writer::CoordinateType::RZ, false>(w);
  }

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::CsvExaTrkXGraphWriter, mex,
                             "CsvExaTrkXGraphWriter", inputGraph, outputDir,
                             outputStem);
}

}  // namespace Acts::Python
