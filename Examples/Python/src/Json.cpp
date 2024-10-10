// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/ProtoDetector.hpp"
#include "Acts/Plugins/Json/DetectorJsonConverter.hpp"
#include "Acts/Plugins/Json/JsonMaterialDecorator.hpp"
#include "Acts/Plugins/Json/MaterialMapJsonConverter.hpp"
#include "Acts/Plugins/Json/ProtoDetectorJsonConverter.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Io/Json/JsonMaterialWriter.hpp"
#include "ActsExamples/Io/Json/JsonSurfacesReader.hpp"
#include "ActsExamples/Io/Json/JsonSurfacesWriter.hpp"

#include <fstream>
#include <initializer_list>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <nlohmann/json.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace Acts {
class IMaterialDecorator;
}  // namespace Acts
namespace ActsExamples {
class IMaterialWriter;
class IWriter;
}  // namespace ActsExamples

namespace py = pybind11;
using namespace pybind11::literals;

using namespace Acts;
using namespace ActsExamples;

namespace Acts::Python {
void addJson(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  {
    py::class_<JsonMaterialDecorator, Acts::IMaterialDecorator,
               std::shared_ptr<JsonMaterialDecorator>>(m,
                                                       "JsonMaterialDecorator")
        .def(py::init<const MaterialMapJsonConverter::Config&,
                      const std::string&, Acts::Logging::Level, bool, bool>(),
             py::arg("rConfig"), py::arg("jFileName"), py::arg("level"),
             py::arg("clearSurfaceMaterial") = true,
             py::arg("clearVolumeMaterial") = true);
  }

  {
    auto cls =
        py::class_<MaterialMapJsonConverter>(m, "MaterialMapJsonConverter")
            .def(py::init<const MaterialMapJsonConverter::Config&,
                          Acts::Logging::Level>(),
                 py::arg("config"), py::arg("level"));

    auto c = py::class_<MaterialMapJsonConverter::Config>(cls, "Config")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, MaterialMapJsonConverter::Config);
    ACTS_PYTHON_MEMBER(context);
    ACTS_PYTHON_MEMBER(processSensitives);
    ACTS_PYTHON_MEMBER(processApproaches);
    ACTS_PYTHON_MEMBER(processRepresenting);
    ACTS_PYTHON_MEMBER(processBoundaries);
    ACTS_PYTHON_MEMBER(processVolumes);
    ACTS_PYTHON_MEMBER(processDenseVolumes);
    ACTS_PYTHON_MEMBER(processNonMaterial);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::enum_<JsonFormat>(mex, "JsonFormat")
        .value("NoOutput", JsonFormat::NoOutput)
        .value("Json", JsonFormat::Json)
        .value("Cbor", JsonFormat::Cbor)
        .value("All", JsonFormat::All);
  }

  {
    auto cls =
        py::class_<JsonMaterialWriter, IMaterialWriter,
                   std::shared_ptr<JsonMaterialWriter>>(mex,
                                                        "JsonMaterialWriter")
            .def(py::init<const JsonMaterialWriter::Config&,
                          Acts::Logging::Level>(),
                 py::arg("config"), py::arg("level"))
            .def("writeMaterial", &JsonMaterialWriter::writeMaterial)
            .def("write", &JsonMaterialWriter::write)
            .def_property_readonly("config", &JsonMaterialWriter::config);

    auto c =
        py::class_<JsonMaterialWriter::Config>(cls, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, JsonMaterialWriter::Config);
    ACTS_PYTHON_MEMBER(converterCfg);
    ACTS_PYTHON_MEMBER(fileName);
    ACTS_PYTHON_MEMBER(writeFormat);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto cls =
        py::class_<JsonSurfacesWriter, IWriter,
                   std::shared_ptr<JsonSurfacesWriter>>(mex,
                                                        "JsonSurfacesWriter")
            .def(py::init<const JsonSurfacesWriter::Config&,
                          Acts::Logging::Level>(),
                 py::arg("config"), py::arg("level"))
            .def("write", &JsonSurfacesWriter::write)
            .def_property_readonly("config", &JsonSurfacesWriter::config);

    auto c =
        py::class_<JsonSurfacesWriter::Config>(cls, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, JsonSurfacesWriter::Config);
    ACTS_PYTHON_MEMBER(trackingGeometry);
    ACTS_PYTHON_MEMBER(outputDir);
    ACTS_PYTHON_MEMBER(outputPrecision);
    ACTS_PYTHON_MEMBER(writeLayer);
    ACTS_PYTHON_MEMBER(writeApproach);
    ACTS_PYTHON_MEMBER(writeSensitive);
    ACTS_PYTHON_MEMBER(writeBoundary);
    ACTS_PYTHON_MEMBER(writePerEvent);
    ACTS_PYTHON_MEMBER(writeOnlyNames);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<Acts::ProtoDetector>(mex, "ProtoDetector")
        .def(py::init<>([](std::string pathName) {
          nlohmann::json jDetector;
          auto in = std::ifstream(pathName, std::ifstream::in);
          if (in.good()) {
            in >> jDetector;
            in.close();
          }
          Acts::ProtoDetector pDetector = jDetector["detector"];
          return pDetector;
        }));
  }

  {
    auto sjOptions = py::class_<ActsExamples::JsonSurfacesReader::Options>(
                         mex, "SurfaceJsonOptions")
                         .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(sjOptions,
                             ActsExamples::JsonSurfacesReader::Options);
    ACTS_PYTHON_MEMBER(inputFile);
    ACTS_PYTHON_MEMBER(jsonEntryPath);
    ACTS_PYTHON_STRUCT_END();

    mex.def("readSurfaceHierarchyMapFromJson",
            ActsExamples::JsonSurfacesReader::readHierarchyMap);

    mex.def("readSurfaceVectorFromJson",
            ActsExamples::JsonSurfacesReader::readVector);
  }

  {
    mex.def("writeDetectorToJson",
            [](const Acts::GeometryContext& gctx,
               const Acts::Experimental::Detector& detector,
               const std::string& name) -> void {
              auto jDetector =
                  Acts::DetectorJsonConverter::toJson(gctx, detector);
              std::ofstream out;
              out.open(name + ".json");
              out << jDetector.dump(4);
              out.close();
            });
  }

  {
    mex.def("writeDetectorToJsonDetray",
            [](const Acts::GeometryContext& gctx,
               const Acts::Experimental::Detector& detector,
               const std::string& name) -> void {
              // Detray format test - manipulate for detray
              Acts::DetectorVolumeJsonConverter::Options detrayOptions;
              detrayOptions.transformOptions.writeIdentity = true;
              detrayOptions.transformOptions.transpose = true;
              detrayOptions.surfaceOptions.transformOptions =
                  detrayOptions.transformOptions;
              detrayOptions.portalOptions.surfaceOptions =
                  detrayOptions.surfaceOptions;

              auto jDetector = Acts::DetectorJsonConverter::toJsonDetray(
                  gctx, detector,
                  Acts::DetectorJsonConverter::Options{detrayOptions});

              // Write out the geometry, surface_grid, material
              auto jGeometry = jDetector["geometry"];
              auto jSurfaceGrids = jDetector["surface_grids"];
              auto jMaterial = jDetector["material"];

              std::ofstream out;
              out.open(name + "_geometry_detray.json");
              out << jGeometry.dump(4);
              out.close();

              out.open(name + "_surface_grids_detray.json");
              out << jSurfaceGrids.dump(4);
              out.close();

              out.open(name + "_material_detray.json");
              out << jMaterial.dump(4);
              out.close();
            });
  }

  {
    mex.def("readDetectorFromJson",
            [](const Acts::GeometryContext& gctx,
               const std::string& fileName) -> auto {
              auto in = std::ifstream(
                  fileName, std::ifstream::in | std::ifstream::binary);
              nlohmann::json jDetectorIn;
              in >> jDetectorIn;
              in.close();

              return Acts::DetectorJsonConverter::fromJson(gctx, jDetectorIn);
            });
  }
}
}  // namespace Acts::Python
