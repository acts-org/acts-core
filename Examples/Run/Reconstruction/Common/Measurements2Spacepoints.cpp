// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <Acts/Utilities/Logger.hpp>
#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/Options/CommonOptions.hpp"
#include "ActsExamples/Utilities/Options.hpp"
#include "ActsExamples/Utilities/Paths.hpp"

#include "ActsExamples/Io/Csv/CsvOptionsReader.hpp"
#include "ActsExamples/Io/Csv/CsvSimHitReader.hpp"
#include "ActsExamples/Io/Csv/CsvParticleReader.hpp" // for evaluating performance
#include "ActsExamples/Io/Csv/CsvSpacepointWriter.hpp"

#include "ActsExamples/TruthTracking/TruthSeedSelector.hpp" // for evaluating performance

#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/Framework/Sequencer.hpp"

#include "ActsExamples/Geometry/CommonGeometry.hpp"
#include "ActsExamples/Detector/IBaseDetector.hpp"
#include "ActsExamples/MagneticField/MagneticFieldOptions.hpp"
#include "ActsExamples/Io/Json/JsonDigitizationConfig.hpp" // to read digi config

#include "ActsExamples/TrackFinding/SpacePointMaker.hpp"
#include "ActsExamples/Reconstruction/ReconstructionBase.hpp"

#include <iostream>
#include <boost/program_options.hpp>

using namespace Acts::UnitLiterals;
using namespace ActsExamples;


static std::unique_ptr<const Acts::Logger> m_logger;
const Acts::Logger& logger() { return *m_logger; }
int runMeasurements2SP(int argc, char* argv[],
    std::shared_ptr<ActsExamples::IBaseDetector> detector) {
  std::cout<<"Welcome to TrackFindingMLBased example." << std::endl;

  // Setup and parse options
  auto desc = Options::makeDefaultOptions();
  Options::addSequencerOptions(desc);
  Options::addRandomNumbersOptions(desc);
  Options::addGeometryOptions(desc);
  Options::addMaterialOptions(desc);
  Options::addOutputOptions(desc, OutputFormat::Csv | OutputFormat::Root);
  Options::addInputOptions(desc);
  Options::addMagneticFieldOptions(desc);

  // Add specific options for this geometry
  // <TODO> make it as an argument.
  // auto detector = std::make_shared<TGeoDetector>();
  detector->addOptions(desc);

  std::cout<<"before parsing options" << std::endl;
  auto vm = Options::parse(desc, argc, argv);
  if (vm.empty()) {
    return EXIT_FAILURE;
  }
  std::cout<<"after  parsing options" << std::endl;

  Sequencer sequencer(Options::readSequencerConfig(vm));

  // Now read the standard options
  auto logLevel = Options::readLogLevel(vm);
  auto outputDir = ensureWritableDirectory(vm["output-dir"].as<std::string>());

  m_logger = Acts::getDefaultLogger("MLBasedTrackFinding", logLevel);
  ACTS_INFO("after parsing input options");

  // The geometry, material and decoration
  // build the detector
  auto geometry = Geometry::build(vm, *detector);
  auto tGeometry = geometry.first;
  auto contextDecorators = geometry.second;
  auto randomNumbers =
      std::make_shared<RandomNumbers>(Options::readRandomNumbersConfig(vm));

  ACTS_INFO("after building geometry");

  // Add the decorator to the sequencer
  for (auto cdr : contextDecorators) {
    sequencer.addContextDecorator(cdr);
  }

  ACTS_INFO("after adding context decorator");

  // Setup the magnetic field
  Options::setupMagneticFieldServices(vm, sequencer);
  auto magneticField = Options::readMagneticField(vm);

  ACTS_INFO("after setting magnetic field");

  // Read the inputs
  auto simHitReaderCfg = setupSimHitReading(vm, sequencer);
  auto particleReader = setupParticleReading(vm, sequencer);
  auto measurementsReader = setupMeasurementReading(vm, sequencer);

  ACTS_INFO("after reading SimHits and particles");

  // Now measurements --> SpacePoints
  SpacePointMaker::Config spCfg;
  spCfg.inputSourceLinks = measurementsReader.outputSourceLinks;
  spCfg.inputMeasurements = measurementsReader.outputMeasurements;
  spCfg.outputSpacePoints = "spacepoints";
  spCfg.trackingGeometry = tGeometry;
  spCfg.geometrySelection = {
    Acts::GeometryIdentifier().setVolume(0)
  };
  sequencer.addAlgorithm(std::make_shared<SpacePointMaker>(spCfg, logLevel));

  // // write out spacepoints...
  CsvSpacepointWriter::Config spWriterCfg;
  spWriterCfg.inputSpacepoints = spCfg.outputSpacePoints;
  spWriterCfg.outputDir = outputDir;
  sequencer.addWriter(std::make_shared<CsvSpacepointWriter>(spWriterCfg, logLevel));

  return sequencer.run();
}