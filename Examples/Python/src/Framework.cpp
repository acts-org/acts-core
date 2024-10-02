// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/IReader.hpp"
#include "ActsExamples/Framework/IWriter.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/Framework/SequenceElement.hpp"
#include "ActsExamples/Framework/Sequencer.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace ActsExamples;
using namespace Acts::Python;

namespace {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
class PySequenceElement : public SequenceElement {
 public:
  using SequenceElement::SequenceElement;

  std::string name() const override {
    py::gil_scoped_acquire acquire{};
    PYBIND11_OVERRIDE_PURE(std::string, SequenceElement, name);
  }

  ProcessCode internalExecute(const AlgorithmContext& ctx) override {
    py::gil_scoped_acquire acquire{};
    PYBIND11_OVERRIDE_PURE(ProcessCode, SequenceElement, sysExecute, ctx);
  }

  ProcessCode initialize() override {
    py::gil_scoped_acquire acquire{};
    PYBIND11_OVERRIDE_PURE(ProcessCode, SequenceElement, initialize);
  }

  ProcessCode finalize() override {
    py::gil_scoped_acquire acquire{};
    PYBIND11_OVERRIDE_PURE(ProcessCode, SequenceElement, finalize);
  }
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

class PyIAlgorithm : public IAlgorithm {
 public:
  using IAlgorithm::IAlgorithm;

  ProcessCode execute(const AlgorithmContext& ctx) const override {
    py::gil_scoped_acquire acquire{};
    try {
      PYBIND11_OVERRIDE_PURE(ProcessCode, IAlgorithm, execute, ctx);
    } catch (py::error_already_set& e) {
      throw;  // Error from python, handle in python.
    } catch (std::runtime_error& e) {
      throw py::type_error("Python algorithm did not conform to interface");
    }
  }
};

void trigger_divbyzero() {
  volatile float j = 0.0;
  volatile float r = 123 / j;  // MARK: divbyzero
  (void)r;
}

void trigger_overflow() {
  volatile float j = std::numeric_limits<float>::max();
  volatile float r = j * j;  // MARK: overflow
  (void)r;
}

void trigger_invalid() {
  volatile float j = -1;
  volatile float r = std::sqrt(j);  // MARK: invalid
  (void)r;
}

}  // namespace

namespace Acts::Python {
void addFramework(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  py::class_<ActsExamples::IWriter, std::shared_ptr<ActsExamples::IWriter>>(
      mex, "IWriter");

  py::class_<ActsExamples::IReader, std::shared_ptr<ActsExamples::IReader>>(
      mex, "IReader");

  py::enum_<ProcessCode>(mex, "ProcessCode")
      .value("SUCCESS", ProcessCode::SUCCESS)
      .value("ABORT", ProcessCode::ABORT)
      .value("END", ProcessCode::END);

  py::class_<WhiteBoard>(mex, "WhiteBoard")
      .def(py::init([](Acts::Logging::Level level, const std::string& name) {
             return std::make_unique<WhiteBoard>(
                 Acts::getDefaultLogger(name, level));
           }),
           py::arg("level"), py::arg("name") = "WhiteBoard")
      .def("exists", &WhiteBoard::exists);

  py::class_<AlgorithmContext>(mex, "AlgorithmContext")
      .def(py::init<std::size_t, std::size_t, WhiteBoard&>())
      .def_readonly("algorithmNumber", &AlgorithmContext::algorithmNumber)
      .def_readonly("eventNumber", &AlgorithmContext::eventNumber)
      .def_property_readonly("eventStore",
                             [](const AlgorithmContext& self) -> WhiteBoard& {
                               return self.eventStore;
                             })
      .def_readonly("magFieldContext", &AlgorithmContext::magFieldContext)
      .def_readonly("geoContext", &AlgorithmContext::geoContext)
      .def_readonly("calibContext", &AlgorithmContext::calibContext)
      .def_readwrite("fpeMonitor", &AlgorithmContext::fpeMonitor);

  auto pySequenceElement =
      py::class_<ActsExamples::SequenceElement, PySequenceElement,
                 std::shared_ptr<ActsExamples::SequenceElement>>(
          mex, "SequenceElement")
          .def(py::init_alias<>())
          .def("internalExecute", &SequenceElement::internalExecute)
          .def("name", &SequenceElement::name);

  auto bareAlgorithm =
      py::class_<ActsExamples::IAlgorithm,
                 std::shared_ptr<ActsExamples::IAlgorithm>, SequenceElement,
                 PyIAlgorithm>(mex, "IAlgorithm")
          .def(py::init_alias<const std::string&, Acts::Logging::Level>(),
               py::arg("name"), py::arg("level"))
          .def("execute", &IAlgorithm::execute);

  using ActsExamples::Sequencer;
  using Config = Sequencer::Config;
  auto sequencer =
      py::class_<Sequencer>(mex, "_Sequencer")
          .def(py::init([](Config cfg) {
            cfg.iterationCallback = []() {
              py::gil_scoped_acquire gil;
              if (PyErr_CheckSignals() != 0) {
                throw py::error_already_set{};
              }
            };
            return std::make_unique<Sequencer>(cfg);
          }))
          .def("run",
               [](Sequencer& self) {
                 py::gil_scoped_release gil;
                 int res = self.run();
                 if (res != EXIT_SUCCESS) {
                   throw std::runtime_error{"Sequencer terminated abnormally"};
                 }
               })
          .def("addContextDecorator", &Sequencer::addContextDecorator)
          .def("addAlgorithm", &Sequencer::addAlgorithm, py::keep_alive<1, 2>())
          .def("addReader", &Sequencer::addReader)
          .def("addWriter", &Sequencer::addWriter)
          .def("addWhiteboardAlias", &Sequencer::addWhiteboardAlias)
          .def_property_readonly("config", &Sequencer::config)
          .def_property_readonly("fpeResult", &Sequencer::fpeResult)
          .def_property_readonly_static(
              "_sourceLocation",
              [](py::object /*self*/) { return std::string{__FILE__}; });

  auto c = py::class_<Config>(sequencer, "Config").def(py::init<>());

  ACTS_PYTHON_STRUCT_BEGIN(c, Config);
  ACTS_PYTHON_MEMBER(skip);
  ACTS_PYTHON_MEMBER(events);
  ACTS_PYTHON_MEMBER(logLevel);
  ACTS_PYTHON_MEMBER(numThreads);
  ACTS_PYTHON_MEMBER(outputDir);
  ACTS_PYTHON_MEMBER(outputTimingFile);
  ACTS_PYTHON_MEMBER(trackFpes);
  ACTS_PYTHON_MEMBER(fpeMasks);
  ACTS_PYTHON_MEMBER(failOnFirstFpe);
  ACTS_PYTHON_MEMBER(fpeStackTraceLength);
  ACTS_PYTHON_STRUCT_END();

  auto fpem =
      py::class_<Sequencer::FpeMask>(sequencer, "_FpeMask")
          .def(py::init<>())
          .def(py::init<std::string, std::pair<unsigned int, unsigned int>,
                        Acts::FpeType, std::size_t>())
          .def("__repr__", [](const Sequencer::FpeMask& self) {
            std::stringstream ss;
            ss << self;
            return ss.str();
          });

  ACTS_PYTHON_STRUCT_BEGIN(fpem, Sequencer::FpeMask);
  ACTS_PYTHON_MEMBER(file);
  ACTS_PYTHON_MEMBER(lines);
  ACTS_PYTHON_MEMBER(type);
  ACTS_PYTHON_MEMBER(count);
  ACTS_PYTHON_STRUCT_END();

  struct FpeMonitorContext {
    std::optional<Acts::FpeMonitor> mon;
  };

  auto fpe = py::class_<Acts::FpeMonitor>(m, "FpeMonitor")
                 .def_static("_trigger_divbyzero", &trigger_divbyzero)
                 .def_static("_trigger_overflow", &trigger_overflow)
                 .def_static("_trigger_invalid", &trigger_invalid)
                 .def_static("context", []() { return FpeMonitorContext(); });

  fpe.def_property_readonly("result",
                            py::overload_cast<>(&Acts::FpeMonitor::result),
                            py::return_value_policy::reference_internal)
      .def("rearm", &Acts::FpeMonitor::rearm);

  py::class_<Acts::FpeMonitor::Result>(fpe, "Result")
      .def("merged", &Acts::FpeMonitor::Result::merged)
      .def("merge", &Acts::FpeMonitor::Result::merge)
      .def("count", &Acts::FpeMonitor::Result::count)
      .def("__str__", [](const Acts::FpeMonitor::Result& result) {
        std::stringstream os;
        result.summary(os);
        return os.str();
      });

  py::class_<FpeMonitorContext>(m, "_FpeMonitorContext")
      .def(py::init([]() { return std::make_unique<FpeMonitorContext>(); }))
      .def(
          "__enter__",
          [](FpeMonitorContext& fm) -> Acts::FpeMonitor& {
            fm.mon.emplace();
            return fm.mon.value();
          },
          py::return_value_policy::reference_internal)
      .def("__exit__", [](FpeMonitorContext& fm, py::object /*exc_type*/,
                          py::object /*exc_value*/,
                          py::object /*traceback*/) { fm.mon.reset(); });

  py::enum_<Acts::FpeType>(m, "FpeType")
      .value("INTDIV", Acts::FpeType::INTDIV)
      .value("INTOVF", Acts::FpeType::INTOVF)
      .value("FLTDIV", Acts::FpeType::FLTDIV)
      .value("FLTOVF", Acts::FpeType::FLTOVF)
      .value("FLTUND", Acts::FpeType::FLTUND)
      .value("FLTRES", Acts::FpeType::FLTRES)
      .value("FLTINV", Acts::FpeType::FLTINV)
      .value("FLTSUB", Acts::FpeType::FLTSUB)

      .def_property_readonly_static(
          "values", [](py::object /*self*/) -> const auto& {
            static const std::vector<Acts::FpeType> values = {
                Acts::FpeType::INTDIV, Acts::FpeType::INTOVF,
                Acts::FpeType::FLTDIV, Acts::FpeType::FLTOVF,
                Acts::FpeType::FLTUND, Acts::FpeType::FLTRES,
                Acts::FpeType::FLTINV, Acts::FpeType::FLTSUB};
            return values;
          });

  py::register_exception<ActsExamples::FpeFailure>(m, "FpeFailure",
                                                   PyExc_RuntimeError);

  using ActsExamples::RandomNumbers;
  auto randomNumbers =
      py::class_<RandomNumbers, std::shared_ptr<RandomNumbers>>(mex,
                                                                "RandomNumbers")
          .def(py::init<const RandomNumbers::Config&>());

  py::class_<ActsExamples::RandomEngine>(mex, "RandomEngine").def(py::init<>());

  py::class_<RandomNumbers::Config>(randomNumbers, "Config")
      .def(py::init<>())
      .def_readwrite("seed", &RandomNumbers::Config::seed);
}

}  // namespace Acts::Python
