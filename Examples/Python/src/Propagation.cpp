// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Direction.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Navigation/DetectorNavigator.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Propagator/AtlasStepper.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Propagator/SympyStepper.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Propagation/PropagationAlgorithm.hpp"
#include "ActsExamples/Propagation/PropagatorInterface.hpp"
#include "ActsExamples/Propagation/SimHitToSummaryConversion.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace Acts {
class MagneticFieldProvider;
}  // namespace Acts

namespace py = pybind11;

namespace {

template <typename stepper_t, typename navigator_t>
void addPropagator(py::module_& m, const std::string& prefix) {
  using propagator_t = Acts::Propagator<stepper_t, navigator_t>;
  py::class_<propagator_t>(m, (prefix + "Propagator").c_str())
      .def(py::init<>(
               [=](stepper_t stepper, navigator_t navigator,
                   Acts::Logging::Level level = Acts::Logging::Level::INFO) {
                 return propagator_t{
                     std::move(stepper), std::move(navigator),
                     Acts::getDefaultLogger(prefix + "Propagator", level)};
               }),
           py::arg("stepper"), py::arg("navigator"),
           py::arg("level") = Acts::Logging::INFO);

  using prop_if_t = ActsExamples::ConcretePropagator<propagator_t>;
  py::class_<prop_if_t, ActsExamples::PropagatorInterface,
             std::shared_ptr<prop_if_t>>(
      m, (prefix + "ConcretePropagator").c_str())
      .def(py::init<propagator_t>());
}

}  // namespace

namespace Acts::Python {
void addPropagation(Context& ctx) {
  auto [m, prop, mex] = ctx.get("main", "propagation", "examples");

  {
    using Config = Acts::Navigator::Config;
    auto nav =
        py::class_<Acts::Navigator, std::shared_ptr<Acts::Navigator>>(
            m, "Navigator")
            .def(py::init<>([](Config cfg,
                               Logging::Level level = Logging::INFO) {
                   return Navigator{cfg, getDefaultLogger("Navigator", level)};
                 }),
                 py::arg("cfg"), py::arg("level") = Logging::INFO);

    auto c = py::class_<Config>(nav, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(resolveMaterial);
    ACTS_PYTHON_MEMBER(resolvePassive);
    ACTS_PYTHON_MEMBER(resolveSensitive);
    ACTS_PYTHON_MEMBER(trackingGeometry);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    using Config = Acts::Experimental::DetectorNavigator::Config;
    auto nav =
        py::class_<Acts::Experimental::DetectorNavigator,
                   std::shared_ptr<Acts::Experimental::DetectorNavigator>>(
            m, "DetectorNavigator")
            .def(py::init<>(
                     [](Config cfg, Logging::Level level = Logging::INFO) {
                       return Acts::Experimental::DetectorNavigator{
                           cfg, getDefaultLogger("DetectorNavigator", level)};
                     }),
                 py::arg("cfg"), py::arg("level") = Logging::INFO);

    auto c = py::class_<Config>(nav, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(resolveMaterial);
    ACTS_PYTHON_MEMBER(resolvePassive);
    ACTS_PYTHON_MEMBER(resolveSensitive);
    ACTS_PYTHON_MEMBER(detector);
    ACTS_PYTHON_STRUCT_END();
  }

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::PropagationAlgorithm, mex, "PropagationAlgorithm",
      propagatorImpl, sterileLogger, debugOutput, energyLoss,
      multipleScattering, recordMaterialInteractions, ptLoopers, maxStepSize,
      covarianceTransport, inputTrackParameters, outputSummaryCollection,
      outputMaterialCollection);

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::SimHitToSummaryConversion, mex,
                                "SimHitToSummaryConversion", inputSimHits,
                                inputParticles, outputSummaryCollection,
                                surfaceByIdentifier);

  py::class_<ActsExamples::PropagatorInterface,
             std::shared_ptr<ActsExamples::PropagatorInterface>>(
      mex, "PropagatorInterface");

  // Eigen based stepper
  {
    auto stepper = py::class_<Acts::EigenStepper<>>(m, "EigenStepper");
    stepper.def(py::init<std::shared_ptr<const Acts::MagneticFieldProvider>>());

    addPropagator<Acts::EigenStepper<>, Acts::Navigator>(prop, "Eigen");
  }

  {
    addPropagator<Acts::EigenStepper<>, Acts::Experimental::DetectorNavigator>(
        prop, "EigenDetector");
  }

  // ATLAS based stepper
  {
    auto stepper = py::class_<Acts::AtlasStepper>(m, "AtlasStepper");
    stepper.def(py::init<std::shared_ptr<const Acts::MagneticFieldProvider>>());

    addPropagator<Acts::AtlasStepper, Acts::Navigator>(prop, "Atlas");
  }

  {
    addPropagator<Acts::AtlasStepper, Acts::Experimental::DetectorNavigator>(
        prop, "AtlasDetector");
  }

  // Sympy based stepper
  {
    auto stepper = py::class_<Acts::SympyStepper>(m, "SympyStepper");
    stepper.def(py::init<std::shared_ptr<const Acts::MagneticFieldProvider>>());

    addPropagator<Acts::SympyStepper, Acts::Navigator>(prop, "Sympy");
  }

  {
    addPropagator<Acts::SympyStepper, Acts::Experimental::DetectorNavigator>(
        prop, "SympyDetector");
  }

  // Straight line stepper
  {
    auto stepper =
        py::class_<Acts::StraightLineStepper>(m, "StraightLineStepper");
    stepper.def(py::init<>());

    addPropagator<Acts::StraightLineStepper, Acts::Navigator>(prop,
                                                              "StraightLine");
  }

  {
    addPropagator<Acts::StraightLineStepper,
                  Acts::Experimental::DetectorNavigator>(
        prop, "StraightLineDetector");
  }
}

}  // namespace Acts::Python
