// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/DD4hepDetector/DD4hepDetectorOptions.hpp"
#include "ActsExamples/DD4hepDetector/DD4hepGeometryService.hpp"
#include "ActsExamples/DDG4/DDG4DetectorConstruction.hpp"
#include "ActsExamples/Geant4/Geant4Options.hpp"
#include "ActsExamples/MagneticField/MagneticFieldOptions.hpp"
#include "ActsExamples/Options/CommonOptions.hpp"
#include "ActsExamples/Simulation/CommonSimulation.hpp"

#include <boost/program_options.hpp>

#include "Geant4.hpp"

int main(int argc, char* argv[]) {
  using namespace ActsExamples;
  using namespace ActsExamples::Simulation;

  // Setup and parse options
  auto desc = Options::makeDefaultOptions();
  Options::addSequencerOptions(desc);
  Options::addDD4hepOptions(desc);
  Simulation::addInputOptions(desc);  // These are specific to the simulation
  Options::addRandomNumbersOptions(desc);
  Options::addGeant4Options(desc);
  Options::addOutputOptions(desc, OutputFormat::Root | OutputFormat::Csv);
  Options::addMagneticFieldOptions(desc);

  auto vars = Options::parse(desc, argc, argv);
  if (vars.empty()) {
    return EXIT_FAILURE;
  }

  // Setup the DD4hep detector
  auto dd4hepCfg = Options::readDD4hepConfig<po::variables_map>(vars);
  auto geometrySvc = std::make_shared<DD4hep::DD4hepGeometryService>(dd4hepCfg);
  auto magneticField = ActsExamples::Options::readMagneticField(vars);
  auto uniqueTrackingGeometry =
      geometrySvc->trackingGeometry(Acts::GeometryContext());
  auto trackingGeometry = std::shared_ptr<const Acts::TrackingGeometry>(
      uniqueTrackingGeometry.release());

  return runGeant4Simulation(
      vars,
      std::make_unique<DDG4DetectorConstructionFactory>(*geometrySvc->lcdd()),
      trackingGeometry);
}
