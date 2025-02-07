// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Digitization/DigitizationAlgorithm.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/Digitization/DigitizationConfigurator.hpp"
#include "ActsExamples/Digitization/DigitizationCoordinatesConverter.hpp"
#include "ActsExamples/Io/Json/JsonDigitizationConfig.hpp"

#include <array>
#include <memory>
#include <tuple>
#include <utility>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace ActsExamples;
using namespace Acts;

namespace Acts::Python {

void addDigitization(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  mex.def("readDigiConfigFromJson", ActsExamples::readDigiConfigFromJson);
  mex.def("writeDigiConfigToJson", ActsExamples::writeDigiConfigToJson);

  {
    using Config = ActsExamples::DigitizationAlgorithm::Config;

    auto a = py::class_<ActsExamples::DigitizationAlgorithm,
                        ActsExamples::IAlgorithm,
                        std::shared_ptr<ActsExamples::DigitizationAlgorithm>>(
                 mex, "DigitizationAlgorithm")
                 .def(py::init<Config&, Acts::Logging::Level>(),
                      py::arg("config"), py::arg("level"))
                 .def_property_readonly(
                     "config", &ActsExamples::DigitizationAlgorithm::config);

    auto c = py::class_<Config>(a, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(inputSimHits);
    ACTS_PYTHON_MEMBER(outputMeasurements);
    ACTS_PYTHON_MEMBER(outputClusters);
    ACTS_PYTHON_MEMBER(outputMeasurementParticlesMap);
    ACTS_PYTHON_MEMBER(outputMeasurementSimHitsMap);
    ACTS_PYTHON_MEMBER(outputParticleMeasurementsMap);
    ACTS_PYTHON_MEMBER(outputSimHitMeasurementsMap);
    ACTS_PYTHON_MEMBER(surfaceByIdentifier);
    ACTS_PYTHON_MEMBER(randomNumbers);
    ACTS_PYTHON_MEMBER(doOutputCells);
    ACTS_PYTHON_MEMBER(doClusterization);
    ACTS_PYTHON_MEMBER(doMerge);
    ACTS_PYTHON_MEMBER(minEnergyDeposit);
    ACTS_PYTHON_MEMBER(digitizationConfigs);
    ACTS_PYTHON_STRUCT_END();

    c.def_readonly("mergeNsigma", &Config::mergeNsigma);
    c.def_readonly("mergeCommonCorner", &Config::mergeCommonCorner);

    patchKwargsConstructor(c);

    auto cc = py::class_<DigiComponentsConfig>(mex, "DigiComponentsConfig")
                  .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(cc, DigiComponentsConfig);
    ACTS_PYTHON_MEMBER(geometricDigiConfig);
    ACTS_PYTHON_MEMBER(smearingDigiConfig);
    ACTS_PYTHON_STRUCT_END();

    py::class_<DigiConfigContainer>(mex, "DigiConfigContainer")
        .def(py::init<std::vector<
                 std::pair<GeometryIdentifier, DigiComponentsConfig>>>());
  }

  {
    using DC = DigitizationConfigurator;
    auto dc = py::class_<DC>(mex, "DigitizationConfigurator").def(py::init<>());

    dc.def("__call__", &DC::operator());

    ACTS_PYTHON_STRUCT_BEGIN(dc, DC);
    ACTS_PYTHON_MEMBER(inputDigiComponents);
    ACTS_PYTHON_MEMBER(compactify);
    ACTS_PYTHON_MEMBER(volumeLayerComponents);
    ACTS_PYTHON_MEMBER(outputDigiComponents);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<ActsExamples::DigitizationCoordinatesConverter,
               std::shared_ptr<ActsExamples::DigitizationCoordinatesConverter>>(
        mex, "DigitizationCoordinatesConverter")
        .def(py::init<ActsExamples::DigitizationAlgorithm::Config&>(),
             py::arg("config"))
        .def_property_readonly(
            "config", &ActsExamples::DigitizationCoordinatesConverter::config)
        .def("globalToLocal",
             &ActsExamples::DigitizationCoordinatesConverter::globalToLocal)
        .def("localToGlobal",
             &ActsExamples::DigitizationCoordinatesConverter::localToGlobal);
  }
}

}  // namespace Acts::Python
