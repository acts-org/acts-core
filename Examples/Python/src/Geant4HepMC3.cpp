// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/Geant4HepMC/EventRecording.hpp"

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
      detector, seed1, seed2, processesCombine, processSelect, processesReject);
}

}  // namespace Acts::Python
