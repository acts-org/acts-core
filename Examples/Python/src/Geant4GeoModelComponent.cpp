// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Geant4/DetectorConstructionFactory.hpp"
#include "ActsExamples/Geant4/RegionCreator.hpp"
#include "ActsExamples/GeoModelG4/GeoModelDetectorConstruction.hpp"

#include <G4VUserDetectorConstruction.hh>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

class GeoVPhysVol;

namespace py = pybind11;

using namespace ActsExamples;
using namespace Acts;

PYBIND11_MODULE(ActsPythonBindingsGeoModelG4, m) {
  py::module_::import("acts.ActsPythonBindingsGeant4");

  py::class_<GeoModelDetectorConstructionFactory, DetectorConstructionFactory,
             std::shared_ptr<GeoModelDetectorConstructionFactory>>(
      m, "GeoModelDetectorConstructionFactory")
      .def(py::init<const Acts::GeoModelTree&,
                    std::vector<std::shared_ptr<RegionCreator>>>(),
           py::arg("geoModelTree"),
           py::arg("regionCreators") =
               std::vector<std::shared_ptr<RegionCreator>>());
}
