// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"

#include "ActsExamples/Utilities/MeasurementMapSelector.hpp"
#include "ActsExamples/Utilities/ProtoTracksToTracks.hpp"
#include "ActsExamples/Utilities/PrototracksToSeeds.hpp"
#include "ActsExamples/Utilities/SeedsToPrototracks.hpp"
#include "ActsExamples/Utilities/TracksToParameters.hpp"
#include "ActsExamples/Utilities/TracksToTrajectories.hpp"
#include "ActsExamples/Utilities/TrajectoriesToPrototracks.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace ActsExamples;
using namespace Acts;

namespace Acts::Python {

void addUtilities(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::TrajectoriesToPrototracks, mex,
                                "TrajectoriesToPrototracks", inputTrajectories,
                                outputProtoTracks);

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::TracksToTrajectories, mex,
                                "TracksToTrajectories", inputTracks,
                                outputTrajectories);

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::TracksToParameters, mex,
                                "TracksToParameters", inputTracks,
                                outputTrackParameters);

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::SeedsToPrototracks, mex,
                                "SeedsToPrototracks", inputSeeds,
                                outputProtoTracks);

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::PrototracksToSeeds, mex, "PrototracksToSeeds",
      inputProtoTracks, inputSpacePoints, outputSeeds, outputProtoTracks);

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::MeasurementMapSelector, mex, "MeasurementMapSelector",
      inputMeasurementParticleMap, inputSourceLinks,
      outputMeasurementParticleMap, geometrySelection);

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::PrototracksToTracks, mex,
                                "PrototracksToTracks", inputMeasurements,
                                inputProtoTracks, outputTracks);
}

}  // namespace Acts::Python
