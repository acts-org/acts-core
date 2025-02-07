// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/AngleHelpers.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Generators/EventGenerator.hpp"
#include "ActsExamples/Generators/MultiplicityGenerators.hpp"
#include "ActsExamples/Generators/ParametricParticleGenerator.hpp"
#include "ActsExamples/Generators/VertexGenerators.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace ActsExamples {
class IReader;
}  // namespace ActsExamples

namespace py = pybind11;

namespace Acts::Python {

void addGenerators(Context& ctx) {
  auto mex = ctx.get("examples");

  {
    using Config = ActsExamples::EventGenerator::Config;
    auto gen = py::class_<ActsExamples::EventGenerator, ActsExamples::IReader,
                          std::shared_ptr<ActsExamples::EventGenerator>>(
                   mex, "EventGenerator")
                   .def(py::init<const Config&, Acts::Logging::Level>(),
                        py::arg("config"), py::arg("level"))
                   .def_property_readonly(
                       "config", &ActsExamples::EventGenerator::config);

    py::class_<
        ActsExamples::EventGenerator::PrimaryVertexPositionGenerator,
        std::shared_ptr<
            ActsExamples::EventGenerator::PrimaryVertexPositionGenerator>>(
        gen, "VertexGenerator");
    py::class_<
        ActsExamples::EventGenerator::ParticlesGenerator,
        std::shared_ptr<ActsExamples::EventGenerator::ParticlesGenerator>>(
        gen, "ParticlesGenerator");
    py::class_<
        ActsExamples::EventGenerator::MultiplicityGenerator,
        std::shared_ptr<ActsExamples::EventGenerator::MultiplicityGenerator>>(
        gen, "MultiplicityGenerator");

    using EventGenerator = ActsExamples::EventGenerator;
    using Generator = EventGenerator::Generator;
    py::class_<Generator>(gen, "Generator")
        .def(py::init<>())
        .def(
            py::init<
                std::shared_ptr<EventGenerator::MultiplicityGenerator>,
                std::shared_ptr<EventGenerator::PrimaryVertexPositionGenerator>,
                std::shared_ptr<EventGenerator::ParticlesGenerator>>(),
            py::arg("multiplicity"), py::arg("vertex"), py::arg("particles"))
        .def_readwrite("multiplicity", &Generator::multiplicity)
        .def_readwrite("vertex", &Generator::vertex)
        .def_readwrite("particles", &Generator::particles);

    py::class_<Config>(gen, "Config")
        .def(py::init<>())
        .def_readwrite("outputParticles", &Config::outputParticles)
        .def_readwrite("outputVertices", &Config::outputVertices)
        .def_readwrite("generators", &Config::generators)
        .def_readwrite("randomNumbers", &Config::randomNumbers);
  }

  py::class_<
      ActsExamples::GaussianPrimaryVertexPositionGenerator,
      ActsExamples::EventGenerator::PrimaryVertexPositionGenerator,
      std::shared_ptr<ActsExamples::GaussianPrimaryVertexPositionGenerator>>(
      mex, "GaussianVertexGenerator")
      .def(py::init<>())
      .def(py::init([](const Acts::Vector4& stddev, const Acts::Vector4& mean) {
             ActsExamples::GaussianPrimaryVertexPositionGenerator g;
             g.stddev = stddev;
             g.mean = mean;
             return g;
           }),
           py::arg("stddev"), py::arg("mean"))
      .def_readwrite(
          "stddev",
          &ActsExamples::GaussianPrimaryVertexPositionGenerator::stddev)
      .def_readwrite(
          "mean", &ActsExamples::GaussianPrimaryVertexPositionGenerator::mean);
  py::class_<
      ActsExamples::GaussianDisplacedVertexPositionGenerator,
      ActsExamples::EventGenerator::PrimaryVertexPositionGenerator,
      std::shared_ptr<ActsExamples::GaussianDisplacedVertexPositionGenerator>>(
      mex, "GaussianDisplacedVertexPositionGenerator")
      .def(py::init<>())
      .def(py::init([](double rMean, double rStdDev, double zMean,
                       double zStdDev, double tMean, double tStdDev) {
             ActsExamples::GaussianDisplacedVertexPositionGenerator g;
             g.rMean = rMean;
             g.rStdDev = rStdDev;
             g.zMean = zMean;
             g.zStdDev = zStdDev;
             g.tMean = tMean;
             g.tStdDev = tStdDev;
             return g;
           }),
           py::arg("rMean"), py::arg("rStdDev"), py::arg("zMean"),
           py::arg("zStdDev"), py::arg("tMean"), py::arg("tStdDev"))
      .def_readwrite(
          "rMean",
          &ActsExamples::GaussianDisplacedVertexPositionGenerator::rMean)
      .def_readwrite(
          "rStdDev",
          &ActsExamples::GaussianDisplacedVertexPositionGenerator::rStdDev)
      .def_readwrite(
          "zMean",
          &ActsExamples::GaussianDisplacedVertexPositionGenerator::zMean)
      .def_readwrite(
          "zStdDev",
          &ActsExamples::GaussianDisplacedVertexPositionGenerator::zStdDev)
      .def_readwrite(
          "tMean",
          &ActsExamples::GaussianDisplacedVertexPositionGenerator::tMean)
      .def_readwrite(
          "tStdDev",
          &ActsExamples::GaussianDisplacedVertexPositionGenerator::tStdDev);

  py::class_<
      ActsExamples::FixedPrimaryVertexPositionGenerator,
      ActsExamples::EventGenerator::PrimaryVertexPositionGenerator,
      std::shared_ptr<ActsExamples::FixedPrimaryVertexPositionGenerator>>(
      mex, "FixedVertexGenerator")
      .def(py::init<>())
      .def(py::init([](const Acts::Vector4& v) {
             ActsExamples::FixedPrimaryVertexPositionGenerator g;
             g.fixed = v;
             return g;
           }),
           py::arg("fixed"))
      .def_readwrite("fixed",
                     &ActsExamples::FixedPrimaryVertexPositionGenerator::fixed);

  py::class_<ActsExamples::SimParticle>(mex, "SimParticle");
  py::class_<ActsExamples::SimParticleContainer>(mex, "SimParticleContainer");

  {
    using Config = ActsExamples::ParametricParticleGenerator::Config;
    auto gen =
        py::class_<ActsExamples::ParametricParticleGenerator,
                   ActsExamples::EventGenerator::ParticlesGenerator,
                   std::shared_ptr<ActsExamples::ParametricParticleGenerator>>(
            mex, "ParametricParticleGenerator")
            .def(py::init<const Config&>());

    py::class_<Config>(gen, "Config")
        .def(py::init<>())
        .def_readwrite("phiMin", &Config::phiMin)
        .def_readwrite("phiMax", &Config::phiMax)
        .def_readwrite("thetaMin", &Config::thetaMin)
        .def_readwrite("thetaMax", &Config::thetaMax)
        .def_readwrite("etaUniform", &Config::etaUniform)
        .def_readwrite("pMin", &Config::pMin)
        .def_readwrite("pMax", &Config::pMax)
        .def_readwrite("pTransverse", &Config::pTransverse)
        .def_readwrite("pLogUniform", &Config::pLogUniform)
        .def_readwrite("pdg", &Config::pdg)
        .def_readwrite("randomizeCharge", &Config::randomizeCharge)
        .def_readwrite("numParticles", &Config::numParticles)
        .def_readwrite("mass", &Config::mass)
        .def_readwrite("charge", &Config::charge)
        .def_property(
            "p", [](Config& cfg) { return std::pair{cfg.pMin, cfg.pMax}; },
            [](Config& cfg, std::pair<double, double> value) {
              cfg.pMin = value.first;
              cfg.pMax = value.second;
            })
        .def_property(
            "phi",
            [](Config& cfg) { return std::pair{cfg.phiMin, cfg.phiMax}; },
            [](Config& cfg, std::pair<double, double> value) {
              cfg.phiMin = value.first;
              cfg.phiMax = value.second;
            })
        .def_property(
            "theta",
            [](Config& cfg) { return std::pair{cfg.thetaMin, cfg.thetaMax}; },
            [](Config& cfg, std::pair<double, double> value) {
              cfg.thetaMin = value.first;
              cfg.thetaMax = value.second;
            })
        .def_property(
            "eta",
            [](Config& cfg) {
              return std::pair{Acts::AngleHelpers::etaFromTheta(cfg.thetaMin),
                               Acts::AngleHelpers::etaFromTheta(cfg.thetaMax)};
            },
            [](Config& cfg, std::pair<double, double> value) {
              cfg.thetaMin = Acts::AngleHelpers::thetaFromEta(value.first);
              cfg.thetaMax = Acts::AngleHelpers::thetaFromEta(value.second);
            });
  }

  py::class_<ActsExamples::FixedMultiplicityGenerator,
             ActsExamples::EventGenerator::MultiplicityGenerator,
             std::shared_ptr<ActsExamples::FixedMultiplicityGenerator>>(
      mex, "FixedMultiplicityGenerator")
      .def(py::init<>())
      .def(py::init([](std::size_t n) {
             ActsExamples::FixedMultiplicityGenerator g;
             g.n = n;
             return g;
           }),
           py::arg("n"))
      .def_readwrite("n", &ActsExamples::FixedMultiplicityGenerator::n);

  py::class_<ActsExamples::PoissonMultiplicityGenerator,
             ActsExamples::EventGenerator::MultiplicityGenerator,
             std::shared_ptr<ActsExamples::PoissonMultiplicityGenerator>>(
      mex, "PoissonMultiplicityGenerator")
      .def(py::init<>())
      .def(py::init([](double mean) {
             ActsExamples::PoissonMultiplicityGenerator g;
             g.mean = mean;
             return g;
           }),
           py::arg("mean"))
      .def_readwrite("mean", &ActsExamples::PoissonMultiplicityGenerator::mean);
}

}  // namespace Acts::Python
