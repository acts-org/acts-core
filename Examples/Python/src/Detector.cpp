// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Material/IMaterialDecorator.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/ContextualDetector/AlignedDetector.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"
#include "ActsExamples/GenericDetector/GenericDetector.hpp"
#include "ActsExamples/TGeoDetector/TGeoDetector.hpp"
#include "ActsExamples/TelescopeDetector/TelescopeDetector.hpp"
#include "ActsExamples/Utilities/Options.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace ActsExamples;

namespace Acts::Python {
void addDetector(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");
  {
    py::class_<IContextDecorator, std::shared_ptr<IContextDecorator>>(
        mex, "IContextDecorator")
        .def("decorate", &IContextDecorator::decorate)
        .def("name", &IContextDecorator::name);
  }

  {
    using Config = GenericDetector::Config;

    auto gd = py::class_<GenericDetector, std::shared_ptr<GenericDetector>>(
                  mex, "GenericDetector")
                  .def(py::init<>())
                  .def("finalize",
                       py::overload_cast<
                           const Config&,
                           std::shared_ptr<const Acts::IMaterialDecorator>>(
                           &GenericDetector::finalize));

    py::class_<Config>(gd, "Config")
        .def(py::init<>())
        .def_readwrite("buildLevel", &Config::buildLevel)
        .def_readwrite("surfaceLogLevel", &Config::surfaceLogLevel)
        .def_readwrite("layerLogLevel", &Config::layerLogLevel)
        .def_readwrite("volumeLogLevel", &Config::volumeLogLevel)
        .def_readwrite("buildProto", &Config::buildProto);
  }

  {
    using TelescopeDetector = Telescope::TelescopeDetector;
    using Config = TelescopeDetector::Config;

    auto td =
        py::class_<TelescopeDetector, std::shared_ptr<TelescopeDetector>>(
            mex, "TelescopeDetector")
            .def(py::init<>())
            .def("finalize",
                 py::overload_cast<
                     const Config&,
                     const std::shared_ptr<const Acts::IMaterialDecorator>&>(
                     &TelescopeDetector::finalize));

    py::class_<Config>(td, "Config")
        .def(py::init<>())
        .def_readwrite("positions", &Config::positions)
        .def_readwrite("stereos", &Config::stereos)
        .def_readwrite("offsets", &Config::offsets)
        .def_readwrite("bounds", &Config::bounds)
        .def_readwrite("thickness", &Config::thickness)
        .def_readwrite("surfaceType", &Config::surfaceType)
        .def_readwrite("binValue", &Config::binValue);
  }

  {
    using AlignedDetector = Contextual::AlignedDetector;
    using Config = AlignedDetector::Config;

    auto d = py::class_<AlignedDetector, std::shared_ptr<AlignedDetector>>(
                 mex, "AlignedDetector")
                 .def(py::init<>())
                 .def("finalize",
                      py::overload_cast<
                          const Config&,
                          std::shared_ptr<const Acts::IMaterialDecorator>>(
                          &AlignedDetector::finalize));

    auto c = py::class_<Config, GenericDetector::Config>(d, "Config")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(seed);
    ACTS_PYTHON_MEMBER(iovSize);
    ACTS_PYTHON_MEMBER(flushSize);
    ACTS_PYTHON_MEMBER(doGarbageCollection);
    ACTS_PYTHON_MEMBER(sigmaInPlane);
    ACTS_PYTHON_MEMBER(sigmaOutPlane);
    ACTS_PYTHON_MEMBER(sigmaInRot);
    ACTS_PYTHON_MEMBER(sigmaOutRot);
    ACTS_PYTHON_MEMBER(firstIovNominal);
    ACTS_PYTHON_MEMBER(decoratorLogLevel);
    ACTS_PYTHON_MEMBER(mode);
    ACTS_PYTHON_STRUCT_END();

    py::enum_<Config::Mode>(c, "Mode")
        .value("Internal", Config::Mode::Internal)
        .value("External", Config::Mode::External);
  }

  {
    using Config = TGeoDetector::Config;

    auto d = py::class_<TGeoDetector, std::shared_ptr<TGeoDetector>>(
                 mex, "TGeoDetector")
                 .def(py::init<>())
                 .def("finalize",
                      py::overload_cast<
                          const Config&,
                          std::shared_ptr<const Acts::IMaterialDecorator>>(
                          &TGeoDetector::finalize));

    py::class_<Options::Interval>(mex, "Interval")
        .def(py::init<>())
        .def(py::init<std::optional<double>, std::optional<double>>())
        .def_readwrite("lower", &Options::Interval::lower)
        .def_readwrite("upper", &Options::Interval::upper);

    auto c = py::class_<Config>(d, "Config").def(py::init<>());

    c.def_property(
        "jsonFile", nullptr,
        [](Config& cfg, const std::string& file) { cfg.readJson(file); });

    py::enum_<Config::SubVolume>(c, "SubVolume")
        .value("Negative", Config::SubVolume::Negative)
        .value("Central", Config::SubVolume::Central)
        .value("Positive", Config::SubVolume::Positive);

    py::enum_<Acts::BinningType>(c, "BinningType")
        .value("equidistant", Acts::BinningType::equidistant)
        .value("arbitrary", Acts::BinningType::arbitrary);

    auto volume = py::class_<Config::Volume>(c, "Volume").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(volume, Config::Volume);
    ACTS_PYTHON_MEMBER(name);
    ACTS_PYTHON_MEMBER(binToleranceR);
    ACTS_PYTHON_MEMBER(binTolerancePhi);
    ACTS_PYTHON_MEMBER(binToleranceZ);
    ACTS_PYTHON_MEMBER(cylinderDiscSplit);
    ACTS_PYTHON_MEMBER(cylinderNZSegments);
    ACTS_PYTHON_MEMBER(cylinderNPhiSegments);
    ACTS_PYTHON_MEMBER(discNRSegments);
    ACTS_PYTHON_MEMBER(discNPhiSegments);
    ACTS_PYTHON_MEMBER(itkModuleSplit);
    ACTS_PYTHON_MEMBER(barrelMap);
    ACTS_PYTHON_MEMBER(discMap);
    ACTS_PYTHON_MEMBER(splitPatterns);

    ACTS_PYTHON_MEMBER(layers);
    ACTS_PYTHON_MEMBER(subVolumeName);
    ACTS_PYTHON_MEMBER(sensitiveNames);
    ACTS_PYTHON_MEMBER(sensitiveAxes);
    ACTS_PYTHON_MEMBER(rRange);
    ACTS_PYTHON_MEMBER(zRange);
    ACTS_PYTHON_MEMBER(splitTolR);
    ACTS_PYTHON_MEMBER(splitTolZ);
    ACTS_PYTHON_MEMBER(binning0);
    ACTS_PYTHON_MEMBER(binning1);
    ACTS_PYTHON_STRUCT_END();

    auto regTriplet = [&c](const std::string& name, auto v) {
      using type = decltype(v);
      py::class_<Config::LayerTriplet<type>>(c, name.c_str())
          .def(py::init<>())
          .def(py::init<type>())
          .def(py::init<type, type, type>())
          .def_readwrite("negative", &Config::LayerTriplet<type>::negative)
          .def_readwrite("central", &Config::LayerTriplet<type>::central)
          .def_readwrite("positive", &Config::LayerTriplet<type>::positive)
          .def("at", py::overload_cast<Config::SubVolume>(
                         &Config::LayerTriplet<type>::at));
    };

    regTriplet("LayerTripletBool", true);
    regTriplet("LayerTripletString", std::string{""});
    regTriplet("LayerTripletVectorString", std::vector<std::string>{});
    regTriplet("LayerTripletInterval", Options::Interval{});
    regTriplet("LayerTripletDouble", double{5.5});
    regTriplet("LayerTripletVectorBinning",
               std::vector<std::pair<int, Acts::BinningType>>{});

    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(surfaceLogLevel);
    ACTS_PYTHON_MEMBER(layerLogLevel);
    ACTS_PYTHON_MEMBER(volumeLogLevel);
    ACTS_PYTHON_MEMBER(fileName);
    ACTS_PYTHON_MEMBER(buildBeamPipe);
    ACTS_PYTHON_MEMBER(beamPipeRadius);
    ACTS_PYTHON_MEMBER(beamPipeHalflengthZ);
    ACTS_PYTHON_MEMBER(beamPipeLayerThickness);
    ACTS_PYTHON_MEMBER(beamPipeEnvelopeR);
    ACTS_PYTHON_MEMBER(layerEnvelopeR);
    ACTS_PYTHON_MEMBER(unitScalor);
    ACTS_PYTHON_MEMBER(volumes);
    ACTS_PYTHON_STRUCT_END();

    patchKwargsConstructor(c);
  }
}

}  // namespace Acts::Python
