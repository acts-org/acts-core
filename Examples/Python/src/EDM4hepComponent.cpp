// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/DD4hepDetector/DD4hepDetector.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepMeasurementReader.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepMeasurementWriter.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepMultiTrajectoryWriter.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepParticleWriter.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepReader.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepSimHitWriter.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepTrackReader.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepTrackWriter.hpp"

#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

using namespace Acts;
using namespace Acts::Python;

PYBIND11_MODULE(ActsPythonBindingsEDM4hep, m) {
  ACTS_PYTHON_DECLARE_READER(
      ActsExamples::EDM4hepReader, m, "EDM4hepReader", inputPath,
      inputParticles, inputSimHits, outputParticlesInitial,
      outputParticlesFinal, outputParticlesGenerator, outputSimHits,
      graphvizOutput, dd4hepDetector, trackingGeometry, sortSimHitsInTime);

  ACTS_PYTHON_DECLARE_WRITER(
      ActsExamples::EDM4hepSimHitWriter, m, "EDM4hepSimHitWriter", inputSimHits,
      inputParticles, outputPath, outputParticles, outputSimTrackerHits);

  ACTS_PYTHON_DECLARE_READER(ActsExamples::EDM4hepMeasurementReader, m,
                             "EDM4hepMeasurementReader", inputPath,
                             outputMeasurements, outputMeasurementSimHitsMap,
                             outputSourceLinks, outputClusters);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::EDM4hepMeasurementWriter, m,
                             "EDM4hepMeasurementWriter", inputMeasurements,
                             inputClusters, outputPath);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::EDM4hepParticleWriter, m,
                             "EDM4hepParticleWriter", inputParticles,
                             outputPath, outputParticles);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::EDM4hepMultiTrajectoryWriter, m,
                             "EDM4hepMultiTrajectoryWriter", inputTrajectories,
                             inputMeasurementParticlesMap, outputPath, Bz);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::EDM4hepTrackWriter, m,
                             "EDM4hepTrackWriter", inputTracks, outputPath, Bz);

  ACTS_PYTHON_DECLARE_READER(ActsExamples::EDM4hepTrackReader, m,
                             "EDM4hepTrackReader", inputTracks, outputTracks,
                             inputPath, Bz);
}
