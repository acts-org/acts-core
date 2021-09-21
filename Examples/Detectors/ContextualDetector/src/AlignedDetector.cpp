// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/ContextualDetector/AlignedDetector.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/ContextualDetector/ExternalAlignmentDecorator.hpp"
#include "ActsExamples/ContextualDetector/ExternallyAlignedDetectorElement.hpp"
#include "ActsExamples/ContextualDetector/InternalAlignmentDecorator.hpp"
#include "ActsExamples/ContextualDetector/InternallyAlignedDetectorElement.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"
#include "ActsExamples/GenericDetector/BuildGenericDetector.hpp"
#include "ActsExamples/GenericDetector/GenericDetectorOptions.hpp"

#include <boost/program_options.hpp>

using namespace Acts::UnitLiterals;
namespace ActsExamples::Contextual {

void AlignedDetector::addOptions(
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
      "align-no-gc", boost::program_options::bool_switch())(
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
      "Keep the first iov batch nominal.")(
      "align-mode",
      boost::program_options::value<std::string>()->default_value("internal"));
}

auto AlignedDetector::finalize(
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
  cfg.doGarbageCollection = !vm["align-no-gc"].as<bool>();

  // The misalingments
  cfg.sigmaInPlane = vm["align-sigma-iplane"].template as<double>() * 1_um;
  cfg.sigmaOutPlane = vm["align-sigma-oplane"].template as<double>() * 1_um;
  cfg.sigmaInRot = vm["align-sigma-irot"].template as<double>() * 0.001;
  cfg.sigmaOutRot = vm["align-sigma-orot"].template as<double>() * 0.001;
  cfg.firstIovNominal = vm["align-firstnominal"].template as<bool>();

  auto mode = vm["align-mode"].as<std::string>();
  if (mode == "external") {
    cfg.mode = Config::Mode::External;
  } else if (mode == "internal") {
    cfg.mode = Config::Mode::Internal;
  } else {
    throw std::invalid_argument{
        "--align-mode must be 'external' or 'internal'"};
  }

  return finalize(cfg, mdecorator);
}

auto AlignedDetector::finalize(
    const Config& cfg,
    std::shared_ptr<const Acts::IMaterialDecorator> mdecorator)
    -> std::pair<ActsExamples::IBaseDetector::TrackingGeometryPtr,
                 ContextDecorators> {
  ContextDecorators aContextDecorators;

  // Let's create a reandom number service
  ActsExamples::RandomNumbers::Config randomNumberConfig;
  randomNumberConfig.seed = cfg.seed;
  auto randomNumberSvc =
      std::make_shared<ActsExamples::RandomNumbers>(randomNumberConfig);

  auto fillDecoratorConfig = [&](AlignmentDecorator::Config& config) {
    config.iovSize = cfg.iovSize;
    config.flushSize = cfg.flushSize;
    config.doGarbageCollection = cfg.doGarbageCollection;

    // The misalingments
    config.gSigmaX = cfg.sigmaInPlane;
    config.gSigmaY = cfg.sigmaInPlane;
    config.gSigmaZ = cfg.sigmaOutPlane;
    config.aSigmaX = cfg.sigmaOutRot;
    config.aSigmaY = cfg.sigmaOutRot;
    config.aSigmaZ = cfg.sigmaInRot;
    config.randomNumberSvc = randomNumberSvc;
    config.firstIovNominal = cfg.firstIovNominal;
  };

  TrackingGeometryPtr aTrackingGeometry;
  if (cfg.mode == Config::Mode::External) {
    ExternallyAlignedDetectorElement::ContextType nominalContext;

    ExternalAlignmentDecorator::Config agcsConfig;
    fillDecoratorConfig(agcsConfig);

    std::vector<std::vector<std::shared_ptr<ExternallyAlignedDetectorElement>>>
        detectorStore;

    aTrackingGeometry =
        ActsExamples::Generic::buildDetector<ExternallyAlignedDetectorElement>(
            nominalContext, detectorStore, cfg.buildLevel,
            std::move(mdecorator), cfg.buildProto, cfg.surfaceLogLevel,
            cfg.layerLogLevel, cfg.volumeLogLevel);

    agcsConfig.trackingGeometry = aTrackingGeometry;

    // need to upcast to store in this object as well
    for (auto& lstore : detectorStore) {
      auto& target = m_detectorStore.emplace_back();
      for (auto& ldet : lstore) {
        target.push_back(ldet);
      }
    }

    aContextDecorators.push_back(std::make_shared<ExternalAlignmentDecorator>(
        std::move(agcsConfig),
        Acts::getDefaultLogger("AlignmentDecorator", cfg.decoratorLogLevel)));
  } else {
    InternallyAlignedDetectorElement::ContextType nominalContext;
    nominalContext.nominal = true;

    InternalAlignmentDecorator::Config agcsConfig;
    fillDecoratorConfig(agcsConfig);

    aTrackingGeometry =
        ActsExamples::Generic::buildDetector<InternallyAlignedDetectorElement>(
            nominalContext, agcsConfig.detectorStore, cfg.buildLevel,
            std::move(mdecorator), cfg.buildProto, cfg.surfaceLogLevel,
            cfg.layerLogLevel, cfg.volumeLogLevel);

    // need to upcast to store in this object as well
    for (auto& lstore : agcsConfig.detectorStore) {
      auto& target = m_detectorStore.emplace_back();
      for (auto& ldet : lstore) {
        target.push_back(ldet);
      }
    }

    aContextDecorators.push_back(std::make_shared<InternalAlignmentDecorator>(
        std::move(agcsConfig),
        Acts::getDefaultLogger("AlignmentDecorator", cfg.decoratorLogLevel)));
  }

  // return the pair of geometry and the alignment decorator(s)
  return std::make_pair<TrackingGeometryPtr, ContextDecorators>(
      std::move(aTrackingGeometry), std::move(aContextDecorators));
}

}  // namespace ActsExamples::Contextual