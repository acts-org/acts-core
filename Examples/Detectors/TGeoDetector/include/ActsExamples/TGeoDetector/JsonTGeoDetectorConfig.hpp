// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Plugins/TGeo/TGeoCylinderDiscSplitter.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "ActsExamples/TGeoDetector/TGeoDetector.hpp"
#include "ActsExamples/TGeoDetector/TGeoITkModuleSplitter.hpp"
#include "ActsExamples/Utilities/Options.hpp"

#include <map>
#include <string>

// Namespace of the module splitters
namespace Acts {

/// Read config for cylinder/disc module splitter
void from_json(const nlohmann::json& j,
               Acts::TGeoCylinderDiscSplitter::Config& cdc) {
  /// Number of segments in phi for a disc
  cdc.cylinderPhiSegments = j.at("geo-tgeo-cyl-nphi-segs");
  /// Number of segments in r for a disk
  cdc.cylinderLongitudinalSegments = j.at("geo-tgeo-cyl-nz-segs");
  /// Number of segments in phi for a disc
  cdc.discPhiSegments = j.at("geo-tgeo-disc-nphi-segs");
  /// Number of segments in r for a disk
  cdc.discRadialSegments = j.at("geo-tgeo-disc-nr-segs");
}

/// Write config for cylinder/disc module splitter
void to_json(nlohmann::json& j,
             const Acts::TGeoCylinderDiscSplitter::Config& cdc) {
  j = nlohmann::json{{"geo-tgeo-cyl-nphi-segs", cdc.cylinderPhiSegments},
                     {"geo-tgeo-cyl-nz-segs", cdc.cylinderLongitudinalSegments},
                     {"geo-tgeo-disc-nphi-segs", cdc.discPhiSegments},
                     {"geo-tgeo-disc-nr-segs", cdc.discRadialSegments}};
}

// enum specialization by nlohman library
NLOHMANN_JSON_SERIALIZE_ENUM(Acts::BinningType,
                             {
                                 {Acts::BinningType::equidistant,
                                  "equidistant"},
                                 {Acts::BinningType::arbitrary, "arbitrary"},
                             })

}  // namespace Acts

namespace ActsExamples {

namespace Options {

/// Read config for options interval
void from_json(const nlohmann::json& j,
               ActsExamples::Options::Interval& interval) {
  interval.lower = j.at("lower");
  interval.upper = j.at("upper");
}

/// Write config for options interval
void to_json(nlohmann::json& j,
             const ActsExamples::Options::Interval& interval) {
  // no direct conversion from std::optional to json
  j = nlohmann::json{{"lower", interval.lower.value_or(0)},
                     {"upper", interval.upper.value_or(0)}};
}

}  // namespace Options

void from_json(const nlohmann::json& j,
               ActsExamples::TGeoITkModuleSplitter::Config& msc) {
  msc.barrelMap =
      j["geo-tgeo-barrel-map"].get<std::map<std::string, unsigned int>>();
  msc.discMap =
      j["geo-tgeo-disc-map"]
          .get<std::map<std::string, std::vector<std::pair<double, double>>>>();
}

void to_json(nlohmann::json& j,
             const ActsExamples::TGeoITkModuleSplitter::Config& msc) {
  j["geo-tgeo-barrel-map"] = msc.barrelMap;
  j["geo-tgeo-disc-map"] = msc.discMap;
}

/// Read layer configuration triplets
template <typename T>
void from_json(const nlohmann::json& j,
               ActsExamples::TGeoDetector::Config::LayerTriplet<T>& ltr) {
  ltr.negative = j.at("negative").get<T>();
  ltr.central = j.at("central").get<T>();
  ltr.positive = j.at("positive").get<T>();
}

/// Write layer configuration triplets
template <typename T>
void to_json(nlohmann::json& j,
             const ActsExamples::TGeoDetector::Config::LayerTriplet<T>& ltr) {
  j = nlohmann::json{{"negative", ltr.negative},
                     {"central", ltr.central},
                     {"positive", ltr.positive}};
}

/// Read volume struct
void from_json(const nlohmann::json& j,
               ActsExamples::TGeoDetector::Config::Volume& vol) {
  // subdetector selection
  vol.name = j.at("geo-tgeo-volume-name");

  // configure surface autobinning
  vol.binToleranceR = j.at("geo-tgeo-sfbin-r-tolerance");
  vol.binToleranceZ = j.at("geo-tgeo-sfbin-z-tolerance");
  vol.binTolerancePhi = j.at("geo-tgeo-sfbin-phi-tolerance");

  // Fill layer triplets
  vol.layers = j.at("geo-tgeo-volume-layers");
  vol.subVolumeName = j.at("geo-tgeo-subvolume-names");
  vol.sensitiveNames = j.at("geo-tgeo-sensitive-names");
  vol.sensitiveAxes = j.at("geo-tgeo-sensitive-axes");
  vol.rRange = j.at("geo-tgeo-layer-r-ranges");
  vol.zRange = j.at("geo-tgeo-layer-z-ranges");
  vol.splitTolR = j.at("geo-tgeo-layer-r-split");
  vol.splitTolZ = j.at("geo-tgeo-layer-z-split");
  // Set binning manually
  vol.binning0 = j.at("geo-tgeo-binning0");
  vol.binning1 = j.at("geo-tgeo-binning1");

  vol.cylinderDiscSplit = j.at("geo-tgeo-cyl-disc-split");
  if (vol.cylinderDiscSplit) {
    Acts::TGeoCylinderDiscSplitter::Config cdConfig =
        j.at("Splitters").at("CylinderDisk");
    vol.cylinderNZSegments = cdConfig.cylinderLongitudinalSegments;
    vol.cylinderNPhiSegments = cdConfig.cylinderPhiSegments;
    vol.discNRSegments = cdConfig.discRadialSegments;
    vol.discNPhiSegments = cdConfig.discPhiSegments;
  }

  // Don't require ITk module splitting to be present
  if (j.count("geo-tgeo-itk-module-split") != 0) {
    vol.itkModuleSplit = j.at("geo-tgeo-itk-module-split");
    if (vol.itkModuleSplit) {
      ActsExamples::TGeoITkModuleSplitter::Config itkConfig =
          j.at("Splitters").at("ITk");
      vol.barrelMap = itkConfig.barrelMap;
      vol.discMap = itkConfig.discMap;
    }
  } else {
    vol.itkModuleSplit = false;
  }
}

/// Write volume struct
void to_json(nlohmann::json& j, const TGeoDetector::Config::Volume& vol) {
  j["geo-tgeo-volume-name"] = vol.name;

  j["geo-tgeo-sfbin-r-tolerance"] = vol.binToleranceR;
  j["geo-tgeo-sfbin-z-tolerance"] = vol.binToleranceZ;
  j["geo-tgeo-sfbin-phi-tolerance"] = vol.binTolerancePhi;

  j["geo-tgeo-volume-layers"] = vol.layers;
  j["geo-tgeo-subvolume-names"] = vol.subVolumeName;
  j["geo-tgeo-sensitive-names"] = vol.sensitiveNames;
  j["geo-tgeo-sensitive-axes"] = vol.sensitiveAxes;
  j["geo-tgeo-layer-r-ranges"] = vol.rRange;
  j["geo-tgeo-layer-z-ranges"] = vol.zRange;
  j["geo-tgeo-layer-r-split"] = vol.splitTolR;
  j["geo-tgeo-layer-z-split"] = vol.splitTolZ;
  j["geo-tgeo-binning0"] = vol.binning0;
  j["geo-tgeo-binning1"] = vol.binning1;

  j["geo-tgeo-cyl-disc-split"] = vol.cylinderDiscSplit;
  j["geo-tgeo-itk-module-split"] = vol.itkModuleSplit;

  Acts::TGeoCylinderDiscSplitter::Config cdConfig;
  cdConfig.cylinderLongitudinalSegments = vol.cylinderNZSegments;
  cdConfig.cylinderPhiSegments = vol.cylinderNPhiSegments;
  cdConfig.discRadialSegments = vol.discNRSegments;
  cdConfig.discPhiSegments = vol.discNPhiSegments;
  j["Splitters"]["CylinderDisk"] = cdConfig;

  if (vol.itkModuleSplit) {
    ActsExamples::TGeoITkModuleSplitter::Config itkConfig;
    itkConfig.barrelMap = vol.barrelMap;
    itkConfig.discMap = vol.discMap;
    j["Splitters"]["ITk"] = itkConfig;
  }
}

}  // namespace ActsExamples
