// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/Geant4/DetectorConstructionFactory.hpp"
#include "ActsExamples/Geant4HepMC/EventRecording.hpp"

#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

using namespace Acts;
using namespace ActsExamples;

namespace Acts::Python {
void addGeant4HepMC3(Context& ctx) {
  auto m = ctx.get("geant4");

  auto h3 = m.def_submodule("hepmc3");

  ACTS_PYTHON_DECLARE_ALGORITHM(
      EventRecording, h3, "EventRecording", inputParticles, outputHepMcTracks,
      detectorConstructionFactory, seed1, seed2, processesCombine,
      processSelect, processesReject);
}

}  // namespace Acts::Python
