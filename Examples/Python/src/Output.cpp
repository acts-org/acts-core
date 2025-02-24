// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"
#include "Acts/Visualization/ViewConfig.hpp"
#include "ActsExamples/Io/Csv/CsvBFieldWriter.hpp"
#include "ActsExamples/Io/Csv/CsvExaTrkXGraphWriter.hpp"
#include "ActsExamples/Io/Csv/CsvMeasurementWriter.hpp"
#include "ActsExamples/Io/Csv/CsvParticleWriter.hpp"
#include "ActsExamples/Io/Csv/CsvProtoTrackWriter.hpp"
#include "ActsExamples/Io/Csv/CsvSeedWriter.hpp"
#include "ActsExamples/Io/Csv/CsvSimHitWriter.hpp"
#include "ActsExamples/Io/Csv/CsvSpacePointWriter.hpp"
#include "ActsExamples/Io/Csv/CsvSpacePointsBucketWriter.hpp"
#include "ActsExamples/Io/Csv/CsvTrackParameterWriter.hpp"
#include "ActsExamples/Io/Csv/CsvTrackWriter.hpp"
#include "ActsExamples/Io/Csv/CsvTrackingGeometryWriter.hpp"
#include "ActsExamples/Io/Obj/ObjPropagationStepsWriter.hpp"
#include "ActsExamples/Io/Obj/ObjSimHitWriter.hpp"
#include "ActsExamples/Io/Obj/ObjTrackingGeometryWriter.hpp"
#include "ActsExamples/MaterialMapping/IMaterialWriter.hpp"
#include "ActsExamples/TrackFinding/ITrackParamsLookupWriter.hpp"

#include <memory>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

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

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::ObjSimHitWriter, mex,
                             "ObjSimHitWriter", inputSimHits, outputDir,
                             outputStem, outputPrecision, drawConnections,
                             momentumThreshold, momentumThresholdTraj,
                             nInterpolatedPoints, keepOriginalHits);

  {
    auto c = py::class_<ViewConfig>(m, "ViewConfig").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, ViewConfig);
    ACTS_PYTHON_MEMBER(visible);
    ACTS_PYTHON_MEMBER(color);
    ACTS_PYTHON_MEMBER(offset);
    ACTS_PYTHON_MEMBER(lineThickness);
    ACTS_PYTHON_MEMBER(surfaceThickness);
    ACTS_PYTHON_MEMBER(quarterSegments);
    ACTS_PYTHON_MEMBER(triangulate);
    ACTS_PYTHON_MEMBER(outputName);
    ACTS_PYTHON_STRUCT_END();

    patchKwargsConstructor(c);

    py::class_<Color>(m, "Color")
        .def(py::init<>())
        .def(py::init<int, int, int>())
        .def(py::init<double, double, double>())
        .def(py::init<std::string_view>())
        .def_readonly("rgb", &Color::rgb);
  }

  py::class_<IVisualization3D>(m, "IVisualization3D")
      .def("write", py::overload_cast<const std::filesystem::path&>(
                        &IVisualization3D::write, py::const_));

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

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::CsvSpacePointWriter, mex,
                             "CsvSpacePointWriter", inputSpacepoints, outputDir,
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

  py::class_<IMaterialWriter, std::shared_ptr<IMaterialWriter>>(
      mex, "IMaterialWriter");

  py::class_<ActsExamples::ITrackParamsLookupWriter,
             std::shared_ptr<ActsExamples::ITrackParamsLookupWriter>>(
      mex, "ITrackParamsLookupWriter");
}

}  // namespace Acts::Python
