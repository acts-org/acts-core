// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Must be on top to avoid some conflict between forward declare and typedef
// Needed until https://gitlab.cern.ch/GeoModelDev/GeoModel/-/merge_requests/351
// is deployed
// clang-format off
#include <GeoModelRead/ReadGeoModel.h>
// clang-format on

#include "Acts/Detector/CylindricalContainerBuilder.hpp"
#include "Acts/Plugins/GeoModel/GeoModelBlueprintCreater.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConverters.hpp"
#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"
#include "Acts/Plugins/GeoModel/GeoModelDetectorSurfaceFactory.hpp"
#include "Acts/Plugins/GeoModel/GeoModelReader.hpp"
#include "Acts/Plugins/GeoModel/GeoModelTree.hpp"
#include "Acts/Plugins/GeoModel/IGeoShapeConverter.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <string>

#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace Acts::Python {
void addGeoModel(Context& ctx) {
  auto m = ctx.get("main");

  auto gm = m.def_submodule("geomodel");

  py::class_<Acts::GeoModelTree>(gm, "GeoModelTree").def(py::init<>());

  gm.def("readFromDb", &Acts::GeoModelReader::readFromDb);

  py::class_<Acts::GeoModelDetectorElement,
             std::shared_ptr<Acts::GeoModelDetectorElement>>(
      gm, "GeoModelDetectorElement");

  // Shape converters
  {
    py::class_<Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::IGeoShapeConverter>>(gm,
                                                          "IGeoShapeConverter");

    py::class_<Acts::GeoBoxConverter, Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::GeoBoxConverter>>(gm, "GeoBoxConverter")
        .def(py::init<>())
        .def("toSensitiveSurface", &Acts::GeoBoxConverter::toSensitiveSurface)
        .def("toPassiveSurface", &Acts::GeoBoxConverter::toPassiveSurface);

    py::class_<Acts::GeoTrdConverter, Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::GeoTrdConverter>>(gm, "GeoTrdConverter")
        .def(py::init<>())
        .def("toSensitiveSurface", &Acts::GeoTrdConverter::toSensitiveSurface)
        .def("toPassiveSurface", &Acts::GeoTrdConverter::toPassiveSurface);

    py::class_<Acts::GeoTubeConverter, Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::GeoTubeConverter>>(gm, "GeoTubeConverter")
        .def(py::init<>())
        .def("toSensitiveSurface", &Acts::GeoTubeConverter::toSensitiveSurface)
        .def("toPassiveSurface", &Acts::GeoTubeConverter::toPassiveSurface);

    py::class_<Acts::GeoUnionDoubleTrdConverter, Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::GeoUnionDoubleTrdConverter>>(
        gm, "GeoUnionDoubleTrdConverter")
        .def(py::init<>())
        .def("toSensitiveSurface",
             &Acts::GeoUnionDoubleTrdConverter::toSensitiveSurface)
        .def("toPassiveSurface",
             &Acts::GeoUnionDoubleTrdConverter::toPassiveSurface);

    py::class_<Acts::GeoIntersectionAnnulusConverter, Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::GeoIntersectionAnnulusConverter>>(
        gm, "GeoIntersectionAnnulusConverter")
        .def(py::init<>())
        .def("toSensitiveSurface",
             &Acts::GeoIntersectionAnnulusConverter::toSensitiveSurface)
        .def("toPassiveSurface",
             &Acts::GeoIntersectionAnnulusConverter::toPassiveSurface);

    py::class_<Acts::GeoShiftConverter, Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::GeoShiftConverter>>(gm,
                                                         "GeoShiftConverter")
        .def(py::init<>())
        .def("toSensitiveSurface", &Acts::GeoShiftConverter::toSensitiveSurface)
        .def("toPassiveSurface", &Acts::GeoShiftConverter::toPassiveSurface);
  }

  // Surface factory
  {
    auto f =
        py::class_<Acts::GeoModelDetectorSurfaceFactory,
                   std::shared_ptr<Acts::GeoModelDetectorSurfaceFactory>>(
            gm, "GeoModelDetectorSurfaceFactory")
            .def(py::init(
                [](const Acts::GeoModelDetectorSurfaceFactory::Config& cfg,
                   Acts::Logging::Level level) {
                  return std::make_shared<Acts::GeoModelDetectorSurfaceFactory>(
                      cfg, Acts::getDefaultLogger(
                               "GeoModelDetectorSurfaceFactory", level));
                }))
            .def("construct", &Acts::GeoModelDetectorSurfaceFactory::construct);

    py::class_<Acts::GeoModelDetectorSurfaceFactory::Config>(f, "Config")
        .def(py::init<>())
        .def_readwrite(
            "shapeConverters",
            &Acts::GeoModelDetectorSurfaceFactory::Config::shapeConverters)
        .def_readwrite("nameList",
                       &Acts::GeoModelDetectorSurfaceFactory::Config::nameList)
        .def_readwrite(
            "materialList",
            &Acts::GeoModelDetectorSurfaceFactory::Config::materialList);

    py::class_<Acts::GeoModelDetectorSurfaceFactory::Cache>(f, "Cache")
        .def(py::init<>())
        .def_readwrite(
            "sensitiveSurfaces",
            &Acts::GeoModelDetectorSurfaceFactory::Cache::sensitiveSurfaces);

    py::class_<Acts::GeoModelDetectorSurfaceFactory::Options>(f, "Options")
        .def(py::init<>())
        .def_readwrite("queries",
                       &Acts::GeoModelDetectorSurfaceFactory::Options::queries);
  }

  {
    py::class_<Acts::GeoModelBlueprintCreater::Blueprint,
               std::shared_ptr<Acts::GeoModelBlueprintCreater::Blueprint>>(
        gm, "Blueprint")
        .def("convertToBuilder",
             [](Acts::GeoModelBlueprintCreater::Blueprint& self,
                Acts::Logging::Level level) {
               // It's a container builder
               return std::make_shared<
                   Acts::Experimental::CylindricalContainerBuilder>(self.node(),
                                                                    level);
             });

    auto bpc =
        py::class_<Acts::GeoModelBlueprintCreater,
                   std::shared_ptr<Acts::GeoModelBlueprintCreater>>(
            gm, "GeoModelBlueprintCreater")
            .def(py::init([](const Acts::GeoModelBlueprintCreater::Config& cfg,
                             Acts::Logging::Level level) {
              return std::make_shared<Acts::GeoModelBlueprintCreater>(
                  cfg,
                  Acts::getDefaultLogger("GeoModelBlueprintCreater", level));
            }))
            .def("create", &Acts::GeoModelBlueprintCreater::create);

    py::class_<Acts::GeoModelBlueprintCreater::Config>(bpc, "Config")
        .def(py::init<>())
        .def_readwrite(
            "detectorSurfaces",
            &Acts::GeoModelBlueprintCreater::Config::detectorSurfaces)
        .def_readwrite("kdtBinning",
                       &Acts::GeoModelBlueprintCreater::Config::kdtBinning);

    py::class_<Acts::GeoModelBlueprintCreater::Options>(bpc, "Options")
        .def(py::init<>())
        .def_readwrite("topEntry",
                       &Acts::GeoModelBlueprintCreater::Options::topEntry)
        .def_readwrite(
            "topBoundsOverride",
            &Acts::GeoModelBlueprintCreater::Options::topBoundsOverride)
        .def_readwrite("table",
                       &Acts::GeoModelBlueprintCreater::Options::table);
  }
}
}  // namespace Acts::Python
