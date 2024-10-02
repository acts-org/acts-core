// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/TruthTracking/ParticleSelector.hpp"
#include "ActsExamples/TruthTracking/ParticleSmearing.hpp"
#include "ActsExamples/TruthTracking/TrackModifier.hpp"
#include "ActsExamples/TruthTracking/TrackParameterSelector.hpp"
#include "ActsExamples/TruthTracking/TrackTruthMatcher.hpp"
#include "ActsExamples/TruthTracking/TruthSeedSelector.hpp"
#include "ActsExamples/TruthTracking/TruthSeedingAlgorithm.hpp"
#include "ActsExamples/TruthTracking/TruthTrackFinder.hpp"
#include "ActsExamples/TruthTracking/TruthVertexFinder.hpp"
#include "ActsExamples/Utilities/HitSelector.hpp"
#include "ActsExamples/Utilities/Range.hpp"

#include <array>
#include <cstddef>
#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace ActsExamples {
class IAlgorithm;
}  // namespace ActsExamples

namespace py = pybind11;

using namespace ActsExamples;
using namespace Acts;

namespace Acts::Python {

void addTruthTracking(Context& ctx) {
  auto mex = ctx.get("examples");

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::TruthTrackFinder, mex, "TruthTrackFinder", inputParticles,
      inputMeasurementParticlesMap, outputProtoTracks);

  {
    using Alg = ActsExamples::TruthSeedSelector;
    using Config = Alg::Config;

    auto alg = py::class_<Alg, IAlgorithm, std::shared_ptr<Alg>>(
                   mex, "TruthSeedSelector")
                   .def(py::init<const Alg::Config&, Acts::Logging::Level>(),
                        py::arg("config"), py::arg("level"))
                   .def_property_readonly("config", &Alg::config);

    auto c = py::class_<Config>(alg, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(inputParticles);
    ACTS_PYTHON_MEMBER(inputMeasurementParticlesMap);
    ACTS_PYTHON_MEMBER(outputParticles);
    ACTS_PYTHON_MEMBER(rhoMin);
    ACTS_PYTHON_MEMBER(rhoMax);
    ACTS_PYTHON_MEMBER(zMin);
    ACTS_PYTHON_MEMBER(zMax);
    ACTS_PYTHON_MEMBER(phiMin);
    ACTS_PYTHON_MEMBER(phiMax);
    ACTS_PYTHON_MEMBER(etaMin);
    ACTS_PYTHON_MEMBER(etaMax);
    ACTS_PYTHON_MEMBER(absEtaMin);
    ACTS_PYTHON_MEMBER(absEtaMax);
    ACTS_PYTHON_MEMBER(ptMin);
    ACTS_PYTHON_MEMBER(ptMax);
    ACTS_PYTHON_MEMBER(keepNeutral);
    ACTS_PYTHON_MEMBER(nHitsMin);
    ACTS_PYTHON_MEMBER(nHitsMax);
    ACTS_PYTHON_STRUCT_END();

    pythonRangeProperty(c, "rho", &Config::rhoMin, &Config::rhoMax);
    pythonRangeProperty(c, "z", &Config::zMin, &Config::zMax);
    pythonRangeProperty(c, "phi", &Config::phiMin, &Config::phiMax);
    pythonRangeProperty(c, "eta", &Config::etaMin, &Config::etaMax);
    pythonRangeProperty(c, "absEta", &Config::absEtaMin, &Config::absEtaMax);
    pythonRangeProperty(c, "pt", &Config::ptMin, &Config::ptMax);
    pythonRangeProperty(c, "nHits", &Config::nHitsMin, &Config::nHitsMax);
  }

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::ParticleSmearing, mex, "ParticleSmearing", inputParticles,
      outputTrackParameters, sigmaD0, sigmaD0PtA, sigmaD0PtB, sigmaZ0,
      sigmaZ0PtA, sigmaZ0PtB, sigmaT0, sigmaPhi, sigmaTheta, sigmaPtRel,
      initialSigmas, initialSigmaPtRel, initialVarInflation, particleHypothesis,
      randomNumbers);

  {
    using Alg = ActsExamples::ParticleSelector;
    using Config = Alg::Config;

    auto alg = py::class_<Alg, IAlgorithm, std::shared_ptr<Alg>>(
                   mex, "ParticleSelector")
                   .def(py::init<const Alg::Config&, Acts::Logging::Level>(),
                        py::arg("config"), py::arg("level"))
                   .def_property_readonly("config", &Alg::config);

    auto c = py::class_<Config>(alg, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(inputParticles);
    ACTS_PYTHON_MEMBER(inputMeasurementParticlesMap);
    ACTS_PYTHON_MEMBER(outputParticles);
    ACTS_PYTHON_MEMBER(rhoMin);
    ACTS_PYTHON_MEMBER(rhoMax);
    ACTS_PYTHON_MEMBER(absZMin);
    ACTS_PYTHON_MEMBER(absZMax);
    ACTS_PYTHON_MEMBER(timeMin);
    ACTS_PYTHON_MEMBER(timeMax);
    ACTS_PYTHON_MEMBER(phiMin);
    ACTS_PYTHON_MEMBER(phiMax);
    ACTS_PYTHON_MEMBER(etaMin);
    ACTS_PYTHON_MEMBER(etaMax);
    ACTS_PYTHON_MEMBER(absEtaMin);
    ACTS_PYTHON_MEMBER(absEtaMax);
    ACTS_PYTHON_MEMBER(mMin);
    ACTS_PYTHON_MEMBER(mMax);
    ACTS_PYTHON_MEMBER(ptMin);
    ACTS_PYTHON_MEMBER(ptMax);
    ACTS_PYTHON_MEMBER(measurementsMin);
    ACTS_PYTHON_MEMBER(measurementsMax);
    ACTS_PYTHON_MEMBER(removeCharged);
    ACTS_PYTHON_MEMBER(removeNeutral);
    ACTS_PYTHON_MEMBER(removeSecondaries);
    ACTS_PYTHON_MEMBER(excludeAbsPdgs);
    ACTS_PYTHON_STRUCT_END();

    pythonRangeProperty(c, "rho", &Config::rhoMin, &Config::rhoMax);
    pythonRangeProperty(c, "absZ", &Config::absZMin, &Config::absZMax);
    pythonRangeProperty(c, "time", &Config::timeMin, &Config::timeMax);
    pythonRangeProperty(c, "phi", &Config::phiMin, &Config::phiMax);
    pythonRangeProperty(c, "eta", &Config::etaMin, &Config::etaMax);
    pythonRangeProperty(c, "absEta", &Config::absEtaMin, &Config::absEtaMax);
    pythonRangeProperty(c, "m", &Config::mMin, &Config::mMax);
    pythonRangeProperty(c, "pt", &Config::ptMin, &Config::ptMax);
    pythonRangeProperty(c, "measurements", &Config::measurementsMin,
                        &Config::measurementsMax);
  }

  {
    using Alg = ActsExamples::TrackParameterSelector;
    using Config = Alg::Config;

    auto alg = py::class_<Alg, IAlgorithm, std::shared_ptr<Alg>>(
                   mex, "TrackParameterSelector")
                   .def(py::init<const Alg::Config&, Acts::Logging::Level>(),
                        py::arg("config"), py::arg("level"))
                   .def_property_readonly("config", &Alg::config);

    auto c = py::class_<Config>(alg, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(inputTrackParameters);
    ACTS_PYTHON_MEMBER(outputTrackParameters);
    ACTS_PYTHON_MEMBER(loc0Min);
    ACTS_PYTHON_MEMBER(loc0Max);
    ACTS_PYTHON_MEMBER(loc1Min);
    ACTS_PYTHON_MEMBER(loc1Max);
    ACTS_PYTHON_MEMBER(timeMin);
    ACTS_PYTHON_MEMBER(timeMax);
    ACTS_PYTHON_MEMBER(phiMin);
    ACTS_PYTHON_MEMBER(phiMax);
    ACTS_PYTHON_MEMBER(etaMin);
    ACTS_PYTHON_MEMBER(etaMax);
    ACTS_PYTHON_MEMBER(absEtaMin);
    ACTS_PYTHON_MEMBER(absEtaMax);
    ACTS_PYTHON_MEMBER(ptMin);
    ACTS_PYTHON_MEMBER(ptMax);
    ACTS_PYTHON_STRUCT_END();

    pythonRangeProperty(c, "loc0", &Config::loc0Min, &Config::loc0Max);
    pythonRangeProperty(c, "loc1", &Config::loc1Min, &Config::loc1Max);
    pythonRangeProperty(c, "time", &Config::timeMin, &Config::timeMax);
    pythonRangeProperty(c, "phi", &Config::phiMin, &Config::phiMax);
    pythonRangeProperty(c, "eta", &Config::etaMin, &Config::etaMax);
    pythonRangeProperty(c, "absEta", &Config::absEtaMin, &Config::absEtaMax);
    pythonRangeProperty(c, "pt", &Config::ptMin, &Config::ptMax);
  }

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::TruthVertexFinder, mex, "TruthVertexFinder", inputTracks,
      inputParticles, inputMeasurementParticlesMap, outputProtoVertices,
      excludeSecondaries, separateSecondaries, trackMatchingRatio);

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::TrackModifier, mex,
                                "TrackModifier", inputTracks, outputTracks,
                                dropCovariance, covScale, killTime);

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::TruthSeedingAlgorithm, mex, "TruthSeedingAlgorithm",
      inputParticles, inputMeasurementParticlesMap, inputSpacePoints,
      outputParticles, outputSeeds, outputProtoTracks, deltaRMin, deltaRMax);

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::HitSelector, mex, "HitSelector",
                                inputHits, outputHits, maxTime);

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::TrackTruthMatcher, mex, "TrackTruthMatcher", inputTracks,
      inputParticles, inputMeasurementParticlesMap, outputTrackParticleMatching,
      outputParticleTrackMatching, matchingRatio, doubleMatching);
}

}  // namespace Acts::Python
