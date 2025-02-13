// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorStructure.hpp"
#include "Acts/Plugins/DD4hep/DD4hepFieldAdapter.hpp"
#include "Acts/Plugins/DD4hep/DD4hepIdentifierMapper.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/DD4hepDetector/DD4hepDetector.hpp"

#include <memory>
#include <utility>

#include <DD4hep/Fields.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace ActsExamples;
using namespace Acts::Python;

PYBIND11_MODULE(ActsPythonBindingsDD4hep, m) {
  {
    py::class_<Acts::DD4hepDetectorElement, Acts::DetectorElementBase,
               std::shared_ptr<Acts::DD4hepDetectorElement>>(
        m, "DD4hepDetectorElement");
  }

  {
    auto f =
        py::class_<DD4hepDetector, Detector, std::shared_ptr<DD4hepDetector>>(
            m, "DD4hepDetector")
            .def(py::init<const DD4hepDetector::Config&>())
            .def_property_readonly("field", &DD4hepDetector::field);

    auto c = py::class_<DD4hepDetector::Config>(f, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, DD4hepDetector::Config);
    ACTS_PYTHON_MEMBER(logLevel);
    ACTS_PYTHON_MEMBER(dd4hepLogLevel);
    ACTS_PYTHON_MEMBER(xmlFileNames);
    ACTS_PYTHON_MEMBER(name);
    ACTS_PYTHON_MEMBER(bTypePhi);
    ACTS_PYTHON_MEMBER(bTypeR);
    ACTS_PYTHON_MEMBER(bTypeZ);
    ACTS_PYTHON_MEMBER(envelopeR);
    ACTS_PYTHON_MEMBER(envelopeZ);
    ACTS_PYTHON_MEMBER(defaultLayerThickness);
    ACTS_PYTHON_MEMBER(materialDecorator);
    ACTS_PYTHON_MEMBER(geometryIdentifierHook);
    ACTS_PYTHON_STRUCT_END();

    patchKwargsConstructor(c);
  }

  {
    py::class_<Acts::DD4hepFieldAdapter, Acts::MagneticFieldProvider,
               std::shared_ptr<Acts::DD4hepFieldAdapter>>(m,
                                                          "DD4hepFieldAdapter");
  }

  {
    m.def("createDD4hepIdGeoIdMap",
          [](const Acts::TrackingGeometry& tGeometry)
              -> std::map<Acts::DD4hepDetectorElement::DD4hepVolumeID,
                          Acts::GeometryIdentifier> {
            // The surface visitor
            struct DD4hepIdGrabber {
              std::map<Acts::DD4hepDetectorElement::DD4hepVolumeID,
                       Acts::GeometryIdentifier>
                  dd4hepIdGeoIdMap;

              void operator()(const Acts::Surface* surface) {
                const auto* dde = surface->associatedDetectorElement();
                const auto* dd4hepDetElement =
                    dynamic_cast<const Acts::DD4hepDetectorElement*>(dde);
                // Check if it is valid
                if (dd4hepDetElement != nullptr) {
                  dd4hep::DDSegmentation::VolumeID dd4hepID =
                      dd4hepDetElement->sourceElement().volumeID();
                  auto geoID = surface->geometryId();
                  dd4hepIdGeoIdMap[dd4hepID] = geoID;
                }
              }
            };

            // Create an instance
            DD4hepIdGrabber dd4hepIdGrabber;
            // Visit the surfaces & return what you have
            tGeometry.visitSurfaces(dd4hepIdGrabber);
            return dd4hepIdGrabber.dd4hepIdGeoIdMap;
          });
  }

  {
    using Options = Acts::Experimental::DD4hepDetectorStructure::Options;
    auto o = py::class_<Options>(m, "DD4hepDetectorOptions").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(o, Options);
    ACTS_PYTHON_MEMBER(logLevel);
    ACTS_PYTHON_MEMBER(emulateToGraph);
    ACTS_PYTHON_MEMBER(geoIdGenerator);
    ACTS_PYTHON_MEMBER(materialDecorator);
    ACTS_PYTHON_STRUCT_END();

    patchKwargsConstructor(o);

    m.def(
        "attachDD4hepGeoIdMapper",
        [](Acts::Experimental::DD4hepDetectorStructure::Options& options,
           const std::map<Acts::DD4hepDetectorElement::DD4hepVolumeID,
                          Acts::GeometryIdentifier>& dd4hepIdGeoIdMap) {
          // The Geo mapper
          auto geoIdMapper =
              std::make_shared<const Acts::DD4hepIdentifierMapper>(
                  Acts::DD4hepIdentifierMapper::Config{dd4hepIdGeoIdMap},
                  Acts::getDefaultLogger("GeometryIdMapper", options.logLevel));

          // A remaining recursive logger
          auto geoIdGenerator =
              std::make_shared<const Acts::Experimental::GeometryIdGenerator>(
                  Acts::Experimental::GeometryIdGenerator::Config{},
                  Acts::getDefaultLogger("GeometryIdGenerator",
                                         options.logLevel));

          std::tuple<
              std::shared_ptr<const Acts::Experimental::GeometryIdGenerator>,
              std::shared_ptr<const Acts::DD4hepIdentifierMapper>>
              chainedGenerators = {geoIdGenerator, geoIdMapper};

          auto chainedGeoIdGenerator = std::make_shared<
              const Acts::Experimental::ChainedGeometryIdGenerator<
                  std::shared_ptr<
                      const Acts::Experimental::GeometryIdGenerator>,
                  std::shared_ptr<const Acts::DD4hepIdentifierMapper>>>(
              std::move(chainedGenerators),
              Acts::getDefaultLogger("ChainedGeometryIdGenerator",
                                     options.logLevel));

          options.geoIdGenerator = chainedGeoIdGenerator;
        });
  }
}
