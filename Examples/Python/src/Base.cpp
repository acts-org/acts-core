// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/Any.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>

#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace Acts::Python {

void addContext(Context& ctx) {
  auto& m = ctx.get("main");

  py::class_<Acts::GeometryContext>(m, "GeometryContext").def(py::init<>());
  py::class_<Acts::MagneticFieldContext>(m, "MagneticFieldContext")
      .def(py::init<>());
  py::class_<Acts::CalibrationContext>(m, "CalibrationContext")
      .def(py::init<>());
}

void addAny(Context& ctx) {
  auto& m = ctx.get("main");

  py::class_<Acts::AnyBase<512>>(m, "AnyBase512").def(py::init<>());
}

void addUnits(Context& ctx) {
  auto& m = ctx.get("main");
  auto u = m.def_submodule("UnitConstants");

#define UNIT(x) u.attr(#x) = Acts::UnitConstants::x;

  UNIT(fm)
  UNIT(pm)
  UNIT(um)
  UNIT(nm)
  UNIT(mm)
  UNIT(cm)
  UNIT(m)
  UNIT(km)
  UNIT(mm2)
  UNIT(cm2)
  UNIT(m2)
  UNIT(mm3)
  UNIT(cm3)
  UNIT(m3)
  UNIT(s)
  UNIT(fs)
  UNIT(ps)
  UNIT(ns)
  UNIT(us)
  UNIT(ms)
  UNIT(min)
  UNIT(h)
  UNIT(mrad)
  UNIT(rad)
  UNIT(degree)
  UNIT(eV)
  UNIT(keV)
  UNIT(MeV)
  UNIT(GeV)
  UNIT(TeV)
  UNIT(J)
  UNIT(u)
  UNIT(g)
  UNIT(kg)
  UNIT(e)
  UNIT(T)
  UNIT(Gauss)
  UNIT(kGauss)
  UNIT(mol)

#undef UNIT
}

class PythonLogger {
 public:
  PythonLogger(const std::string& name, Acts::Logging::Level level)
      : m_name{name}, m_logger{Acts::getDefaultLogger(m_name, level)} {}

  void log(Acts::Logging::Level level, const std::string& message) const {
    m_logger->log(level, message);
  }

  void setLevel(Acts::Logging::Level level) {
    m_logger = Acts::getDefaultLogger(m_name, level);
  }

 private:
  std::string m_name;
  std::unique_ptr<const Logger> m_logger;
};

void addLogging(Acts::Python::Context& ctx) {
  auto& m = ctx.get("main");
  auto logging = m.def_submodule("logging", "");

  auto levelEnum = py::enum_<Acts::Logging::Level>(logging, "Level")
                       .value("VERBOSE", Acts::Logging::VERBOSE)
                       .value("DEBUG", Acts::Logging::DEBUG)
                       .value("INFO", Acts::Logging::INFO)
                       .value("WARNING", Acts::Logging::WARNING)
                       .value("ERROR", Acts::Logging::ERROR)
                       .value("FATAL", Acts::Logging::FATAL)
                       .value("MAX", Acts::Logging::MAX)
                       .export_values();

  levelEnum
      .def("__lt__", [](Acts::Logging::Level self,
                        Acts::Logging::Level other) { return self < other; })
      .def("__gt__", [](Acts::Logging::Level self,
                        Acts::Logging::Level other) { return self > other; })
      .def("__le__", [](Acts::Logging::Level self,
                        Acts::Logging::Level other) { return self <= other; })
      .def("__ge__", [](Acts::Logging::Level self, Acts::Logging::Level other) {
        return self >= other;
      });

  auto makeLogFunction = [](Acts::Logging::Level level) {
    return
        [level](PythonLogger& logger, const std::string& fmt, py::args args) {
          auto locals = py::dict();
          locals["args"] = args;
          locals["fmt"] = fmt;
          py::exec(R"(
        message = fmt % args
    )",
                   py::globals(), locals);

          auto message = locals["message"].cast<std::string>();

          logger.log(level, message);
        };
  };

  auto logger =
      py::class_<PythonLogger, std::shared_ptr<PythonLogger>>(logging, "Logger")
          .def("log", &PythonLogger::log)
          .def("verbose", makeLogFunction(Acts::Logging::VERBOSE))
          .def("debug", makeLogFunction(Acts::Logging::DEBUG))
          .def("info", makeLogFunction(Acts::Logging::INFO))
          .def("warning", makeLogFunction(Acts::Logging::WARNING))
          .def("error", makeLogFunction(Acts::Logging::ERROR))
          .def("fatal", makeLogFunction(Acts::Logging::FATAL))
          .def("setLevel", &PythonLogger::setLevel);

  static std::unordered_map<std::string, std::shared_ptr<PythonLogger>>
      pythonLoggers = {{"root", std::make_shared<PythonLogger>(
                                    "Python", Acts::Logging::INFO)}};

  logging.def(
      "getLogger",
      [](const std::string& name) {
        if (!pythonLoggers.contains(name)) {
          pythonLoggers[name] =
              std::make_shared<PythonLogger>(name, Acts::Logging::INFO);
        }
        return pythonLoggers[name];
      },
      py::arg("name") = "root");

  logging.def("setLevel", [](Acts::Logging::Level level) {
    pythonLoggers.at("root")->setLevel(level);
  });

  auto makeModuleLogFunction = [](Acts::Logging::Level level) {
    return [level](const std::string& fmt, py::args args) {
      auto locals = py::dict();
      locals["args"] = args;
      locals["fmt"] = fmt;
      py::exec(R"(
        message = fmt % args
    )",
               py::globals(), locals);

      auto message = locals["message"].cast<std::string>();

      pythonLoggers.at("root")->log(level, message);
    };
  };

  logging.def("setFailureThreshold", &Logging::setFailureThreshold);
  logging.def("getFailureThreshold", &Logging::getFailureThreshold);

  static py::exception<Logging::ThresholdFailure> exc(
      logging, "ThresholdFailure", PyExc_RuntimeError);
  // NOLINTNEXTLINE(performance-unnecessary-value-param)
  py::register_exception_translator([](std::exception_ptr p) {
    try {
      if (p) {
        std::rethrow_exception(p);
      }
    } catch (const std::exception& e) {
      std::string what = e.what();
      if (what.find("ACTS_LOG_FAILURE_THRESHOLD") != std::string::npos) {
        py::set_error(exc, e.what());
      } else {
        std::rethrow_exception(p);
      }
    }
  });

  logging.def("verbose", makeModuleLogFunction(Acts::Logging::VERBOSE));
  logging.def("debug", makeModuleLogFunction(Acts::Logging::DEBUG));
  logging.def("info", makeModuleLogFunction(Acts::Logging::INFO));
  logging.def("warning", makeModuleLogFunction(Acts::Logging::WARNING));
  logging.def("error", makeModuleLogFunction(Acts::Logging::ERROR));
  logging.def("fatal", makeModuleLogFunction(Acts::Logging::FATAL));
}

void addPdgParticle(Acts::Python::Context& ctx) {
  auto& m = ctx.get("main");
  py::enum_<Acts::PdgParticle>(m, "PdgParticle")
      .value("eInvalid", Acts::PdgParticle::eInvalid)
      .value("eElectron", Acts::PdgParticle::eElectron)
      .value("eAntiElectron", Acts::PdgParticle::eAntiElectron)
      .value("ePositron", Acts::PdgParticle::ePositron)
      .value("eMuon", Acts::PdgParticle::eMuon)
      .value("eAntiMuon", Acts::PdgParticle::eAntiMuon)
      .value("eTau", Acts::PdgParticle::eTau)
      .value("eAntiTau", Acts::PdgParticle::eAntiTau)
      .value("eGamma", Acts::PdgParticle::eGamma)
      .value("ePionZero", Acts::PdgParticle::ePionZero)
      .value("ePionPlus", Acts::PdgParticle::ePionPlus)
      .value("ePionMinus", Acts::PdgParticle::ePionMinus)
      .value("eKaonPlus", Acts::PdgParticle::eKaonPlus)
      .value("eKaonMinus", Acts::PdgParticle::eKaonMinus)
      .value("eNeutron", Acts::PdgParticle::eNeutron)
      .value("eAntiNeutron", Acts::PdgParticle::eAntiNeutron)
      .value("eProton", Acts::PdgParticle::eProton)
      .value("eAntiProton", Acts::PdgParticle::eAntiProton)
      .value("eLead", Acts::PdgParticle::eLead);
}

void addAlgebra(Acts::Python::Context& ctx) {
  auto& m = ctx.get("main");

  py::class_<Acts::Vector2>(m, "Vector2")
      .def(py::init<double, double>())
      .def(py::init([](std::array<double, 2> a) {
        Acts::Vector2 v;
        v << a[0], a[1];
        return v;
      }))
      .def("__getitem__",
           [](const Acts::Vector2& self, Eigen::Index i) { return self[i]; })
      .def("__str__", [](const Acts::Vector3& self) {
        std::stringstream ss;
        ss << self.transpose();
        return ss.str();
      });

  py::class_<Acts::Vector3>(m, "Vector3")
      .def(py::init<double, double, double>())
      .def(py::init([](std::array<double, 3> a) {
        Acts::Vector3 v;
        v << a[0], a[1], a[2];
        return v;
      }))
      .def_static("UnitX", []() -> Vector3 { return Acts::Vector3::UnitX(); })
      .def_static("UnitY", []() -> Vector3 { return Acts::Vector3::UnitY(); })
      .def_static("UnitZ", []() -> Vector3 { return Acts::Vector3::UnitZ(); })

      .def("__getitem__",
           [](const Acts::Vector3& self, Eigen::Index i) { return self[i]; })
      .def("__str__", [](const Acts::Vector3& self) {
        std::stringstream ss;
        ss << self.transpose();
        return ss.str();
      });

  py::class_<Acts::Vector4>(m, "Vector4")
      .def(py::init<double, double, double, double>())
      .def(py::init([](std::array<double, 4> a) {
        Acts::Vector4 v;
        v << a[0], a[1], a[2], a[3];
        return v;
      }))
      .def("__getitem__",
           [](const Acts::Vector4& self, Eigen::Index i) { return self[i]; });

  py::class_<Acts::Transform3>(m, "Transform3")
      .def(py::init<>())
      .def(py::init([](const Vector3& translation) -> Transform3 {
        return Transform3{Translation3{translation}};
      }))
      .def_property_readonly("translation",
                             [](const Acts::Transform3& self) -> Vector3 {
                               return self.translation();
                             })
      .def_static("Identity", &Acts::Transform3::Identity)
      .def("__mul__",
           [](const Acts::Transform3& self, const Acts::Transform3& other) {
             return self * other;
           })
      .def("__mul__",
           [](const Acts::Transform3& self, const Acts::Translation3& other) {
             return self * other;
           })
      .def("__mul__",
           [](const Acts::Transform3& self, const Acts::AngleAxis3& other) {
             return self * other;
           })
      .def("__str__", [](const Acts::Transform3& self) {
        std::stringstream ss;
        ss << self.matrix();
        return ss.str();
      });

  py::class_<Acts::Translation3>(m, "Translation3")
      .def(py::init(
          [](const Acts::Vector3& a) { return Acts::Translation3(a); }))
      .def(py::init([](std::array<double, 3> a) {
        return Acts::Translation3(Acts::Vector3(a[0], a[1], a[2]));
      }))
      .def("__str__", [](const Acts::Translation3& self) {
        std::stringstream ss;
        ss << self.translation().transpose();
        return ss.str();
      });

  py::class_<Acts::AngleAxis3>(m, "AngleAxis3")
      .def(py::init([](double angle, const Acts::Vector3& axis) {
        return Acts::AngleAxis3(angle, axis);
      }))
      .def("__str__", [](const Acts::Transform3& self) {
        std::stringstream ss;
        ss << self.matrix();
        return ss.str();
      });
}

void addBinning(Context& ctx) {
  auto& m = ctx.get("main");

  auto binningValue = py::enum_<Acts::BinningValue>(m, "BinningValue")
                          .value("binX", Acts::BinningValue::binX)
                          .value("binY", Acts::BinningValue::binY)
                          .value("binZ", Acts::BinningValue::binZ)
                          .value("binR", Acts::BinningValue::binR)
                          .value("binPhi", Acts::BinningValue::binPhi);

  auto boundaryType = py::enum_<Acts::AxisBoundaryType>(m, "AxisBoundaryType")
                          .value("bound", Acts::AxisBoundaryType::Bound)
                          .value("closed", Acts::AxisBoundaryType::Closed)
                          .value("open", Acts::AxisBoundaryType::Open);

  auto axisType = py::enum_<Acts::AxisType>(m, "AxisType")
                      .value("equidistant", Acts::AxisType::Equidistant)
                      .value("variable", Acts::AxisType::Variable);
}

}  // namespace Acts::Python
