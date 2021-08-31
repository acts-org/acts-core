// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/ContextualDetector/InternallyAlignedDetector.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/ContextualDetector/AlignmentDecorator.hpp"
#include "ActsExamples/ContextualDetector/InternallyAlignedDetectorElement.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"
#include "ActsExamples/GenericDetector/BuildGenericDetector.hpp"
#include "ActsExamples/GenericDetector/GenericDetectorOptions.hpp"

#include <boost/program_options.hpp>

using namespace Acts::UnitLiterals;

void InternallyAlignedDetector::addOptions(
    boost::program_options::options_description& opt) const {
  // Add the generic geometry options
  ActsExamples::Options::addGenericGeometryOptions(opt);
  // specify the rotation setp
  opt.add_options()(
      "align-seed",
      boost::program_options::value<size_t>()->default_value(1324354657),
      "Seed for the decorator random numbers.")(
      "align-iovsize",
      boost::program_options::value<size_t>()->default_value(100),
      "Size of a valid IOV.")(
      "align-flushsize",
      boost::program_options::value<size_t>()->default_value(200),
      "Span until garbage collection is active.")(
      "align-sigma-iplane",
      boost::program_options::value<double>()->default_value(100.),
      "Sigma of the in-plane misalignment in [um]")(
      "align-sigma-oplane",
      boost::program_options::value<double>()->default_value(50.),
      "Sigma of the out-of-plane misalignment in [um]")(
      "align-sigma-irot",
      boost::program_options::value<double>()->default_value(20.),
      "Sigma of the in-plane rotation misalignment in [mrad]")(
      "align-sigma-orot",
      boost::program_options::value<double>()->default_value(0.),
      "Sigma of the out-of-plane rotation misalignment in [mrad]")(
      "align-loglevel",
      boost::program_options::value<size_t>()->default_value(3),
      "Output log level of the alignment decorator.")(
      "align-firstnominal",
      boost::program_options::value<bool>()->default_value(false),
      "Keep the first iov batch nominal.");
}

auto InternallyAlignedDetector::finalize(
    const boost::program_options::variables_map& vm,
    std::shared_ptr<const Acts::IMaterialDecorator> mdecorator)
    -> std::pair<TrackingGeometryPtr, ContextDecorators> {
  // --------------------------------------------------------------------------------

  Config cfg;

  cfg.buildLevel = vm["geo-generic-buildlevel"].template as<size_t>();
  // set geometry building logging level
  cfg.surfaceLogLevel =
      Acts::Logging::Level(vm["geo-surface-loglevel"].template as<size_t>());
  cfg.layerLogLevel =
      Acts::Logging::Level(vm["geo-layer-loglevel"].template as<size_t>());
  cfg.volumeLogLevel =
      Acts::Logging::Level(vm["geo-volume-loglevel"].template as<size_t>());

  cfg.buildProto = (vm["mat-input-type"].template as<std::string>() == "proto");

  cfg.decoratorLogLevel =
      Acts::Logging::Level(vm["align-loglevel"].template as<size_t>());

  cfg.seed = vm["align-seed"].template as<size_t>();
  cfg.iovSize = vm["align-iovsize"].template as<size_t>();
  cfg.flushSize = vm["align-flushsize"].template as<size_t>();

  // The misalingments
  cfg.sigmaInPlane = vm["align-sigma-iplane"].template as<double>() * 1_um;
  cfg.sigmaOutPlane = vm["align-sigma-oplane"].template as<double>() * 1_um;
  cfg.sigmaInRot = vm["align-sigma-irot"].template as<double>() * 0.001;
  cfg.sigmaOutRot = vm["align-sigma-orot"].template as<double>() * 0.001;
  cfg.firstIovNominal = vm["align-firstnominal"].template as<bool>();

  return finalize(cfg, mdecorator);
}

auto InternallyAlignedDetector::finalize(
    const Config& cfg,
    std::shared_ptr<const Acts::IMaterialDecorator> mdecorator)
    -> std::pair<ActsExamples::IBaseDetector::TrackingGeometryPtr,
                 ContextDecorators> {
  DetectorElement::ContextType nominalContext;

  /// return the generic detector - with aligned context decorator
  TrackingGeometryPtr aTrackingGeometry =
      ActsExamples::Generic::buildDetector<DetectorElement>(
          nominalContext, detectorStore, cfg.buildLevel, std::move(mdecorator),
          cfg.buildProto, cfg.surfaceLogLevel, cfg.layerLogLevel,
          cfg.volumeLogLevel);

  // Let's create a reandom number service
  ActsExamples::RandomNumbers::Config randomNumberConfig;
  randomNumberConfig.seed = cfg.seed;
  auto randomNumberSvc =
      std::make_shared<ActsExamples::RandomNumbers>(randomNumberConfig);

  // Alignment decorator service
  Decorator::Config agcsConfig;
  agcsConfig.detectorStore = detectorStore;
  agcsConfig.iovSize = cfg.iovSize;
  agcsConfig.flushSize = cfg.flushSize;

  // The misalingments
  agcsConfig.gSigmaX = cfg.sigmaInPlane;
  agcsConfig.gSigmaY = cfg.sigmaInPlane;
  agcsConfig.gSigmaZ = cfg.sigmaOutPlane;
  agcsConfig.aSigmaX = cfg.sigmaOutRot;
  agcsConfig.aSigmaY = cfg.sigmaOutRot;
  agcsConfig.aSigmaZ = cfg.sigmaInRot;
  agcsConfig.randomNumberSvc = randomNumberSvc;
  agcsConfig.firstIovNominal = cfg.firstIovNominal;

  // Now create the alignment decorator
  ContextDecorators aContextDecorators = {std::make_shared<Decorator>(
      agcsConfig,
      Acts::getDefaultLogger("AlignmentDecorator", cfg.decoratorLogLevel))};

  // return the pair of geometry and the alignment decorator(s)
  return std::make_pair<TrackingGeometryPtr, ContextDecorators>(
      std::move(aTrackingGeometry), std::move(aContextDecorators));
}