// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/HepMC/HepMCProcessExtractor.hpp"
#include "ActsExamples/Io/HepMC3/HepMC3Reader.hpp"
#include "ActsExamples/Io/HepMC3/HepMC3Writer.hpp"

#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

using namespace Acts;

namespace Acts::Python {
void addHepMC3(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  auto hepmc3 = mex.def_submodule("_hepmc3");

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::HepMCProcessExtractor, hepmc3,
                                "HepMCProcessExtractor", inputEvents,
                                outputSimulationProcesses, extractionProcess,
                                absPdgMin, absPdgMax, pMin);

  ACTS_PYTHON_DECLARE_WRITER(ActsExamples::HepMC3AsciiWriter, hepmc3,
                             "HepMC3AsciiWriter", outputDir, outputStem,
                             inputEvents);

  ACTS_PYTHON_DECLARE_READER(ActsExamples::HepMC3AsciiReader, hepmc3,
                             "HepMC3AsciiReader", inputDir, inputStem,
                             outputEvents);
}
}  // namespace Acts::Python
