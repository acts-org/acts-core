// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/TGeoDetector/TGeoDetector.hpp"

#include "Acts/Geometry/CylinderVolumeBuilder.hpp"
#include "Acts/Geometry/CylinderVolumeHelper.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/PassiveLayerBuilder.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingGeometryBuilder.hpp"
#include "Acts/Geometry/TrackingVolumeArrayCreator.hpp"
#include "Acts/Plugins/TGeo/TGeoCylinderDiscSplitter.hpp"
#include "Acts/Plugins/TGeo/TGeoDetectorElement.hpp"
#include "Acts/Plugins/TGeo/TGeoLayerBuilder.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"
#include "ActsExamples/TGeoDetector/JsonTGeoDetectorConfig.hpp"
#include "ActsExamples/Utilities/Options.hpp"

#include <cstdlib>
#include <fstream>
#include <list>

#include <boost/program_options.hpp>

namespace ActsExamples {
using namespace Options;

namespace {

/// @brief Function that constructs a set of layer builder
///        configs from a central @c TGeoDetector config.
///
/// @param config The input config
/// @return Vector of layer builder configs
std::vector<Acts::TGeoLayerBuilder::Config> makeLayerBuilderConfigs(
    const TGeoDetector::Config& config) {
  std::vector<Acts::TGeoLayerBuilder::Config> detLayerConfigs;

  // iterate over all configured detector volumes
  for (const auto& volume : config.volumes) {
    Acts::TGeoLayerBuilder::Config layerBuilderConfig;
    layerBuilderConfig.configurationName = volume.name;
    layerBuilderConfig.unit = config.unitScalor;

    // configure surface autobinning
    std::vector<std::pair<double, double>> binTolerances(
        static_cast<size_t>(Acts::binValues), {0., 0.});
    binTolerances[Acts::binR] = {volume.binToleranceR.lower.value_or(0.),
                                 volume.binToleranceR.upper.value_or(0.)};
    binTolerances[Acts::binZ] = {volume.binToleranceZ.lower.value_or(0.),
                                 volume.binToleranceZ.upper.value_or(0.)};
    binTolerances[Acts::binPhi] = {volume.binTolerancePhi.lower.value_or(0.),
                                   volume.binTolerancePhi.upper.value_or(0.)};

    layerBuilderConfig.autoSurfaceBinning = true;
    layerBuilderConfig.surfaceBinMatcher =
        Acts::SurfaceBinningMatcher(binTolerances);

    // loop over the negative/central/positive layer configurations
    for (auto ncp : {
             TGeoDetector::Config::Negative,
             TGeoDetector::Config::Central,
             TGeoDetector::Config::Positive,
         }) {
      if (!volume.layers.at(ncp)) {
        continue;
      }

      Acts::TGeoLayerBuilder::LayerConfig lConfig;
      lConfig.volumeName = volume.subVolumeName.at(ncp);
      lConfig.sensorNames = volume.sensitiveNames.at(ncp);
      lConfig.localAxes = volume.sensitiveAxes.at(ncp);

      auto rR = volume.rRange.at(ncp);
      auto rMin = rR.lower.value_or(0.);
      auto rMax = rR.upper.value_or(std::numeric_limits<double>::max());
      auto zR = volume.zRange.at(ncp);
      auto zMin = zR.lower.value_or(-std::numeric_limits<double>::max());
      auto zMax = zR.upper.value_or(std::numeric_limits<double>::max());
      lConfig.parseRanges = {
          {Acts::binR, {rMin, rMax}},
          {Acts::binZ, {zMin, zMax}},
      };

      // Fill the layer splitting parameters in r/z
      auto str = volume.splitTolR.at(ncp);
      auto stz = volume.splitTolZ.at(ncp);
      if (0 < str) {
        lConfig.splitConfigs.emplace_back(Acts::binR, str);
      }
      if (0 < stz) {
        lConfig.splitConfigs.emplace_back(Acts::binZ, stz);
      }

      layerBuilderConfig.layerConfigurations[ncp].push_back(lConfig);
    }

    // Perform splitting of cylinders and discs
    if (volume.cylinderDiscSplit) {
      Acts::TGeoCylinderDiscSplitter::Config cdsConfig;
      cdsConfig.cylinderPhiSegments = volume.cylinderNPhiSegments;
      cdsConfig.cylinderLongitudinalSegments = volume.cylinderNZSegments;
      cdsConfig.discPhiSegments = volume.discNPhiSegments;
      cdsConfig.discRadialSegments = volume.discNRSegments;
      layerBuilderConfig.detectorElementSplitter =
          std::make_shared<const Acts::TGeoCylinderDiscSplitter>(cdsConfig);
    }

    detLayerConfigs.push_back(layerBuilderConfig);
  }

  return detLayerConfigs;
}

/// @brief Function to build the generic tracking geometry
// from a TGeo object.
///
/// It does *currently* not translate the material, this has
/// to be done with a material mapping stage
///
/// @tparam variable_map_t is the variable map
///
/// @param vm is the variable map from the options
std::shared_ptr<const Acts::TrackingGeometry> buildTGeoDetector(
    const TGeoDetector::Config& config, const Acts::GeometryContext& context,
    std::vector<std::shared_ptr<const Acts::TGeoDetectorElement>>&
        detElementStore,
    std::shared_ptr<const Acts::IMaterialDecorator> mdecorator) {
  // configure surface array creator
  Acts::SurfaceArrayCreator::Config sacConfig;
  auto surfaceArrayCreator = std::make_shared<const Acts::SurfaceArrayCreator>(
      sacConfig,
      Acts::getDefaultLogger("SurfaceArrayCreator", config.surfaceLogLevel));
  // configure the proto layer helper
  Acts::ProtoLayerHelper::Config plhConfig;
  auto protoLayerHelper = std::make_shared<const Acts::ProtoLayerHelper>(
      plhConfig,
      Acts::getDefaultLogger("ProtoLayerHelper", config.layerLogLevel));
  // configure the layer creator that uses the surface array creator
  Acts::LayerCreator::Config lcConfig;
  lcConfig.surfaceArrayCreator = surfaceArrayCreator;
  auto layerCreator = std::make_shared<const Acts::LayerCreator>(
      lcConfig, Acts::getDefaultLogger("LayerCreator", config.layerLogLevel));
  // configure the layer array creator
  Acts::LayerArrayCreator::Config lacConfig;
  auto layerArrayCreator = std::make_shared<const Acts::LayerArrayCreator>(
      lacConfig,
      Acts::getDefaultLogger("LayerArrayCreator", config.layerLogLevel));
  // tracking volume array creator
  Acts::TrackingVolumeArrayCreator::Config tvacConfig;
  auto tVolumeArrayCreator =
      std::make_shared<const Acts::TrackingVolumeArrayCreator>(
          tvacConfig, Acts::getDefaultLogger("TrackingVolumeArrayCreator",
                                             config.volumeLogLevel));
  // configure the cylinder volume helper
  Acts::CylinderVolumeHelper::Config cvhConfig;
  cvhConfig.layerArrayCreator = layerArrayCreator;
  cvhConfig.trackingVolumeArrayCreator = tVolumeArrayCreator;
  auto cylinderVolumeHelper =
      std::make_shared<const Acts::CylinderVolumeHelper>(
          cvhConfig, Acts::getDefaultLogger("CylinderVolumeHelper",
                                            config.volumeLogLevel));

  //-------------------------------------------------------------------------------------
  // list the volume builders
  std::list<std::shared_ptr<const Acts::ITrackingVolumeBuilder>> volumeBuilders;

  // Create a beam pipe if configured to do so
  if (config.buildBeamPipe) {
    /// configure the beam pipe layer builder
    Acts::PassiveLayerBuilder::Config bplConfig;
    bplConfig.layerIdentification = "BeamPipe";
    bplConfig.centralLayerRadii = {config.beamPipeRadius};
    bplConfig.centralLayerHalflengthZ = {config.beamPipeHalflengthZ};
    bplConfig.centralLayerThickness = {config.beamPipeLayerThickness};
    auto beamPipeBuilder = std::make_shared<const Acts::PassiveLayerBuilder>(
        bplConfig,
        Acts::getDefaultLogger("BeamPipeLayerBuilder", config.layerLogLevel));
    // create the volume for the beam pipe
    Acts::CylinderVolumeBuilder::Config bpvConfig;
    bpvConfig.trackingVolumeHelper = cylinderVolumeHelper;
    bpvConfig.volumeName = "BeamPipe";
    bpvConfig.layerBuilder = beamPipeBuilder;
    bpvConfig.layerEnvelopeR = {1. * Acts::UnitConstants::mm,
                                1. * Acts::UnitConstants::mm};
    bpvConfig.buildToRadiusZero = true;
    auto beamPipeVolumeBuilder =
        std::make_shared<const Acts::CylinderVolumeBuilder>(
            bpvConfig, Acts::getDefaultLogger("BeamPipeVolumeBuilder",
                                              config.volumeLogLevel));
    // add to the list of builders
    volumeBuilders.push_back(beamPipeVolumeBuilder);
  }

  // Import the file from
  TGeoManager::Import(config.fileName.c_str());

  auto layerBuilderConfigs = makeLayerBuilderConfigs(config);

  // Remember the layer builders to collect the detector elements
  std::vector<std::shared_ptr<const Acts::TGeoLayerBuilder>> tgLayerBuilders;

  for (auto& lbc : layerBuilderConfigs) {
    std::shared_ptr<const Acts::LayerCreator> layerCreatorLB = nullptr;

    if (lbc.autoSurfaceBinning) {
      // Configure surface array creator (optionally) per layer builder
      // (in order to configure them to work appropriately)
      Acts::SurfaceArrayCreator::Config sacConfigLB;
      sacConfigLB.surfaceMatcher = lbc.surfaceBinMatcher;
      auto surfaceArrayCreatorLB =
          std::make_shared<const Acts::SurfaceArrayCreator>(
              sacConfigLB, Acts::getDefaultLogger(
                               lbc.configurationName + "SurfaceArrayCreator",
                               config.surfaceLogLevel));
      // configure the layer creator that uses the surface array creator
      Acts::LayerCreator::Config lcConfigLB;
      lcConfigLB.surfaceArrayCreator = surfaceArrayCreatorLB;
      layerCreatorLB = std::make_shared<const Acts::LayerCreator>(
          lcConfigLB,
          Acts::getDefaultLogger(lbc.configurationName + "LayerCreator",
                                 config.layerLogLevel));
    }

    // Configure the proto layer helper
    Acts::ProtoLayerHelper::Config plhConfigLB;
    auto protoLayerHelperLB = std::make_shared<const Acts::ProtoLayerHelper>(
        plhConfigLB,
        Acts::getDefaultLogger(lbc.configurationName + "ProtoLayerHelper",
                               config.layerLogLevel));

    //-------------------------------------------------------------------------------------
    lbc.layerCreator =
        (layerCreatorLB != nullptr) ? layerCreatorLB : layerCreator;
    lbc.protoLayerHelper =
        (protoLayerHelperLB != nullptr) ? protoLayerHelperLB : protoLayerHelper;

    auto layerBuilder = std::make_shared<const Acts::TGeoLayerBuilder>(
        lbc, Acts::getDefaultLogger(lbc.configurationName + "LayerBuilder",
                                    config.layerLogLevel));
    // remember the layer builder
    tgLayerBuilders.push_back(layerBuilder);

    // build the pixel volume
    Acts::CylinderVolumeBuilder::Config volumeConfig;
    volumeConfig.trackingVolumeHelper = cylinderVolumeHelper;
    volumeConfig.volumeName = lbc.configurationName;
    volumeConfig.buildToRadiusZero = (volumeBuilders.size() == 0);
    volumeConfig.layerEnvelopeR = {1. * Acts::UnitConstants::mm,
                                   5. * Acts::UnitConstants::mm};
    auto ringLayoutConfiguration =
        [&](const std::vector<Acts::TGeoLayerBuilder::LayerConfig>& lConfigs)
        -> void {
      for (const auto& lcfg : lConfigs) {
        for (const auto& scfg : lcfg.splitConfigs) {
          if (scfg.first == Acts::binR and scfg.second > 0.) {
            volumeConfig.ringTolerance =
                std::max(volumeConfig.ringTolerance, scfg.second);
            volumeConfig.checkRingLayout = true;
          }
        }
      }
    };
    ringLayoutConfiguration(lbc.layerConfigurations[0]);
    ringLayoutConfiguration(lbc.layerConfigurations[2]);
    volumeConfig.layerBuilder = layerBuilder;
    volumeConfig.volumeSignature = 0;
    auto volumeBuilder = std::make_shared<const Acts::CylinderVolumeBuilder>(
        volumeConfig,
        Acts::getDefaultLogger(lbc.configurationName + "VolumeBuilder",
                               config.volumeLogLevel));
    // add to the list of builders
    volumeBuilders.push_back(volumeBuilder);
  }

  //-------------------------------------------------------------------------------------
  // create the tracking geometry
  Acts::TrackingGeometryBuilder::Config tgConfig;
  // Add the builders
  tgConfig.materialDecorator = mdecorator;

  for (auto& vb : volumeBuilders) {
    tgConfig.trackingVolumeBuilders.push_back(
        [=](const auto& gcontext, const auto& inner, const auto&) {
          return vb->trackingVolume(gcontext, inner);
        });
  }
  // Add the helper
  tgConfig.trackingVolumeHelper = cylinderVolumeHelper;
  auto cylinderGeometryBuilder =
      std::make_shared<const Acts::TrackingGeometryBuilder>(
          tgConfig, Acts::getDefaultLogger("TrackerGeometryBuilder",
                                           config.volumeLogLevel));
  // get the geometry
  auto trackingGeometry = cylinderGeometryBuilder->trackingGeometry(context);
  // collect the detector element store
  for (auto& lBuilder : tgLayerBuilders) {
    auto detElements = lBuilder->detectorElements();
    detElementStore.insert(detElementStore.begin(), detElements.begin(),
                           detElements.end());
  }

  /// return the tracking geometry
  return trackingGeometry;
}

/// Read the TGeo layer builder configurations from the user configuration.
void readTGeoLayerBuilderConfigs(const Variables& vm,
                                 TGeoDetector::Config& config) {
  const auto path = vm["geo-tgeo-jsonconfig"].template as<std::string>();
  nlohmann::json djson;
  if (path.empty()) {
    return;
  }
  std::ifstream infile(path, std::ifstream::in | std::ifstream::binary);
  infile >> djson;

  config.unitScalor = djson["geo-tgeo-unit-scalor"];

  config.buildBeamPipe = djson["geo-tgeo-build-beampipe"];
  if (config.buildBeamPipe) {
    const auto beamPipeParameters =
        djson["geo-tgeo-beampipe-parameters"].get<std::array<double, 3>>();
    config.beamPipeRadius = beamPipeParameters[0];
    config.beamPipeHalflengthZ = beamPipeParameters[1];
    config.beamPipeLayerThickness = beamPipeParameters[2];
  }

  // Fill nested volume configs
  for (const auto& volume : djson["Volumes"]) {
    auto& vol = config.volumes.emplace_back();
    vol = volume;
  }
}

/// Dump TGeo Detector config to file.
void writeTGeoDetectorConfig(const Variables& vm,
                             TGeoDetector::Config& config) {
  const auto path = vm["geo-tgeo-dump-jsonconfig"].template as<std::string>();
  nlohmann::ordered_json djson;
  if (path.empty()) {
    return;
  }
  std::ofstream outfile(path, std::ofstream::out | std::ofstream::binary);

  djson["geo-tgeo-unit-scalor"] = config.unitScalor;
  djson["geo-tgeo-build-beampipe"] = config.buildBeamPipe;
  djson["geo-tgeo-beampipe-parameters"] =
      std::array<double, 3>{config.beamPipeRadius, config.beamPipeHalflengthZ,
                            config.beamPipeLayerThickness};

  // Enable empty volume dump
  if (config.volumes.size() == 0)
    config.volumes.emplace_back();
  djson["Volumes"] = config.volumes;

  outfile << djson.dump(2) << std::endl;
}

}  // namespace

void TGeoDetector::addOptions(
    boost::program_options::options_description& desc) const {
  using boost::program_options::value;

  // The options are encoded in a json file, where an empty version containing
  // the required structure can be dumped.
  //
  //   # Unit scalor from ROOT to Acts.
  //   "geo-tgeo-unit-scalor": 1,
  //   # Beam pipe parameters {r, z, t} in [mm]. Beam pipe is automatically
  //     created if the parameters are present.
  //   "geo-tgeo-beampipe-parameters": [29.0,3000.0,0.8]
  //
  // Each detector volume configuration is one logical block which can
  // be repeated as many times as there are usable detector volumes.
  //
  // required per-volume options:
  // In case intervals are required, a lower and an upper value must be passed.
  //
  //  # Detector volume name
  //  "geo-tgeo-volume": "InnerPixels",
  //  # vTolerance interval in r [mm] for automated surface binninng.
  //  "geo-tgeo-sfbin-r-tolerance": {"lower": 5, "upper": 5},
  //  # Tolerance interval in phi [rad] for automated surface binning.
  //  "geo-tgeo-sfbin-phi-tolerance": {"lower": 0.025, "upper": 0.025},
  //  # Tolerance interval in z [mm] for automated surface binning.
  //  "geo-tgeo-sfbin-z-tolerance": {"lower": 5, "upper": 5},
  //
  // optional per-volume layer options that can be present once.
  // `geo-tgeo-{n,c,p}-layers` must be present for each volume and if it is
  // non-zero, all other layer options with the same tag ("negative",
  // "central", "positive") must be set as well.
  //
  //  # boolean switch whether there are negative/central/positive layers
  //  "geo-tgeo-volume-layers":
  //    {
  //      "negative": true,
  //      "central": true,
  //      "positive": true
  //    },
  //  #
  //  "geo-tgeo-subvolume-names": { "negative": , "central": , "positive": },
  //  # Name identifier of the volume for searching n,c,p layers
  //  "geo-tgeo-sensitive-names": { ... }
  //  # Axes definition for n,c,p sensitive objects
  //  "geo-tgeo-sensitive-axes": { ... }
  //  # Radial range(s) for n,c,p layers to restrict the module parsing
  //  "geo-tgeo-layer-r-ranges": { ... }
  //  # Longitudinal range(s) for n,c,p layers to restrict the module parsing
  //  "geo-tgeo-layer-z-ranges": { ... }
  //  # R-tolerances (if > 0.) that triggers splitting of collected surfaces
  //  # into different negative layers
  //  "geo-tgeo-layer-r-split": { ... }
  //  # Z-tolerances (if > 0.) that triggers splitting of collected surfaces
  //  # into different negative layers
  //  "geo-tgeo-layer-z-split": { ... }
  //
  //  In case cylinder / disc splitting is turned on:
  //
  //   "geo-tgeo-cyl-nz-segs"    # number of z segments for cylinder splitting
  //   "geo-tgeo-cyl-nphi-segs"  # number of phi segments for cylinder splitting
  //   "geo-tgeo-disc-nr-segs"   # number of r segments for disc splitting
  //   "geo-tgeo-disc-nphi-segs" # number of phi segments for disc splitting

  auto opt = desc.add_options();
  // required global options
  opt("geo-tgeo-filename", value<std::string>()->default_value(""),
      "Root file name.");
  opt("geo-tgeo-jsonconfig", value<std::string>()->default_value(""),
      "Json config file name.");
  opt("geo-tgeo-dump-jsonconfig",
      value<std::string>()->default_value("empty_config.json"),
      "Json file to dump empty config into.");
}

auto TGeoDetector::finalize(
    const boost::program_options::variables_map& vm,
    std::shared_ptr<const Acts::IMaterialDecorator> mdecorator)
    -> std::pair<TrackingGeometryPtr, ContextDecorators> {
  Config config;

  config.fileName = vm["geo-tgeo-filename"].as<std::string>();

  config.surfaceLogLevel =
      Acts::Logging::Level(vm["geo-surface-loglevel"].template as<size_t>());
  config.layerLogLevel =
      Acts::Logging::Level(vm["geo-layer-loglevel"].template as<size_t>());
  config.volumeLogLevel =
      Acts::Logging::Level(vm["geo-volume-loglevel"].template as<size_t>());

  // No valid geometry configuration. Stop
  if (vm["geo-tgeo-jsonconfig"].as<std::string>().empty()) {
    writeTGeoDetectorConfig(vm, config);
    std::exit(EXIT_SUCCESS);
  }

  readTGeoLayerBuilderConfigs(vm, config);

  return finalize(config, std::move(mdecorator));
}

auto TGeoDetector::finalize(
    const Config& cfg,
    std::shared_ptr<const Acts::IMaterialDecorator> mdecorator)
    -> std::pair<TrackingGeometryPtr, ContextDecorators> {
  Acts::GeometryContext tGeoContext;
  TrackingGeometryPtr tgeoTrackingGeometry =
      buildTGeoDetector(cfg, tGeoContext, detectorStore, mdecorator);

  ContextDecorators tgeoContextDeocrators = {};
  // Return the pair of geometry and empty decorators
  return std::make_pair<TrackingGeometryPtr, ContextDecorators>(
      std::move(tgeoTrackingGeometry), std::move(tgeoContextDeocrators));
}

}  // namespace ActsExamples