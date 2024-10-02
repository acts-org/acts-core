// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/DD4hepDetector/DD4hepDetector.hpp"
#include "ActsExamples/DDG4/DDG4DetectorConstruction.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Geant4/RegionCreator.hpp"

#include <G4VUserDetectorConstruction.hh>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

class G4VUserDetectorConstruction;

namespace py = pybind11;

using namespace ActsExamples;
using namespace Acts;

PYBIND11_MODULE(ActsPythonBindingsDDG4, m) {
  py::module_::import("acts.ActsPythonBindingsGeant4");

  py::class_<DDG4DetectorConstructionFactory, DetectorConstructionFactory,
             std::shared_ptr<DDG4DetectorConstructionFactory>>(
      m, "DDG4DetectorConstructionFactory")
      .def(py::init<std::shared_ptr<DD4hep::DD4hepDetector>,
                    std::vector<std::shared_ptr<RegionCreator>>>(),
           py::arg("detector"),
           py::arg("regionCreators") =
               std::vector<std::shared_ptr<RegionCreator>>());
}
