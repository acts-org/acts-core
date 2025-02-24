// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/Framework/BufferedReader.hpp"
#include "ActsExamples/Io/Csv/CsvDriftCircleReader.hpp"
#include "ActsExamples/Io/Csv/CsvExaTrkXGraphReader.hpp"
#include "ActsExamples/Io/Csv/CsvMeasurementReader.hpp"
#include "ActsExamples/Io/Csv/CsvMuonSimHitReader.hpp"
#include "ActsExamples/Io/Csv/CsvParticleReader.hpp"
#include "ActsExamples/Io/Csv/CsvSimHitReader.hpp"
#include "ActsExamples/Io/Csv/CsvSpacePointReader.hpp"
#include "ActsExamples/Io/Csv/CsvTrackParameterReader.hpp"
#include "ActsExamples/TrackFinding/ITrackParamsLookupReader.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

using namespace ActsExamples;

namespace Acts::Python {

void addInput(Context& ctx) {
  auto mex = ctx.get("examples");

  // Buffered reader
  ACTS_PYTHON_DECLARE_READER(ActsExamples::BufferedReader, mex,
                             "BufferedReader", upstreamReader, selectionSeed,
                             bufferSize);

  ACTS_PYTHON_DECLARE_READER(ActsExamples::CsvParticleReader, mex,
                             "CsvParticleReader", inputDir, inputStem,
                             outputParticles);

  ACTS_PYTHON_DECLARE_READER(
      ActsExamples::CsvMeasurementReader, mex, "CsvMeasurementReader", inputDir,
      outputMeasurements, outputMeasurementSimHitsMap, outputClusters,
      outputMeasurementParticlesMap, inputSimHits);

  ACTS_PYTHON_DECLARE_READER(ActsExamples::CsvSimHitReader, mex,
                             "CsvSimHitReader", inputDir, inputStem,
                             outputSimHits);
  ACTS_PYTHON_DECLARE_READER(ActsExamples::CsvMuonSimHitReader, mex,
                             "CsvMuonSimHitReader", inputDir, inputStem,
                             outputSimHits);
  ACTS_PYTHON_DECLARE_READER(ActsExamples::CsvDriftCircleReader, mex,
                             "CsvDriftCircleReader", inputDir, inputStem,
                             outputDriftCircles);

  ACTS_PYTHON_DECLARE_READER(
      ActsExamples::CsvSpacePointReader, mex, "CsvSpacePointReader", inputDir,
      inputStem, inputCollection, outputSpacePoints, extendCollection);

  ACTS_PYTHON_DECLARE_READER(ActsExamples::CsvTrackParameterReader, mex,
                             "CsvTrackParameterReader", inputDir, inputStem,
                             outputTrackParameters, beamspot);

  ACTS_PYTHON_DECLARE_READER(ActsExamples::CsvExaTrkXGraphReader, mex,
                             "CsvExaTrkXGraphReader", inputDir, inputStem,
                             outputGraph);

  py::class_<ActsExamples::ITrackParamsLookupReader,
             std::shared_ptr<ActsExamples::ITrackParamsLookupReader>>(
      mex, "ITrackParamsLookupReader");
}

}  // namespace Acts::Python
