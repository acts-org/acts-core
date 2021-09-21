// This file is part of the Acts project.
//
// Copyright (C) 2018-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Plugins/TGeo/TGeoCylinderDiscSplitter.hpp"
#include "ActsExamples/TGeoDetector/TGeoDetector.hpp"
#include "ActsExamples/Utilities/Options.hpp"

// Namespace of the module splitters
namespace Acts {

/// Read config for cylinder/disc module splitter
void from_json(const nlohmann::json& j,
               Acts::TGeoCylinderDiscSplitter::Config& msc) {
  /// Number of segments in phi for a disc
  msc.cylinderPhiSegments = j.at("geo-tgeo-cyl-nphi-segs");
  /// Number of segments in r for a disk
  msc.cylinderLongitudinalSegments = j.at("geo-tgeo-cyl-nz-segs");
  /// Number of segments in phi for a disc
  msc.discPhiSegments = j.at("geo-tgeo-disc-nphi-segs");
  /// Number of segments in r for a disk
  msc.discRadialSegments = j.at("geo-tgeo-disc-nr-segs");
}

/// Write config for cylinder/disc module splitter
void to_json(nlohmann::ordered_json& j,
             const Acts::TGeoCylinderDiscSplitter::Config& cdc) {
  j = nlohmann::ordered_json{
      {"geo-tgeo-cyl-nphi-segs", cdc.cylinderPhiSegments},
      {"geo-tgeo-cyl-nz-segs", cdc.cylinderLongitudinalSegments},
      {"geo-tgeo-disc-nphi-segs", cdc.discPhiSegments},
      {"geo-tgeo-disc-nr-segs", cdc.discRadialSegments}};
}

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
void to_json(nlohmann::ordered_json& j,
             const ActsExamples::Options::Interval& interval) {
  // no direct conversion from std::optional to json
  j = nlohmann::ordered_json{{"lower", interval.lower.value_or(0)},
                             {"upper", interval.upper.value_or(0)}};
}

}  // namespace Options

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
void to_json(nlohmann::ordered_json& j,
             const ActsExamples::TGeoDetector::Config::LayerTriplet<T>& ltr) {
  j = nlohmann::ordered_json{{"negative", ltr.negative},
                             {"central", ltr.central},
                             {"positive", ltr.positive}};
}

/// Read volume struct
void from_json(const nlohmann::json& j, TGeoDetector::Config::Volume& vol) {
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

  vol.cylinderDiscSplit = j.at("geo-tgeo-cyl-disc-split");
  if (vol.cylinderDiscSplit) {
    Acts::TGeoCylinderDiscSplitter::Config cdConfig =
        j.at("Splitters").at("CylinderDisk");
    vol.cylinderNZSegments = cdConfig.cylinderLongitudinalSegments;
    vol.cylinderNPhiSegments = cdConfig.cylinderPhiSegments;
    vol.discNRSegments = cdConfig.discRadialSegments;
    vol.discNPhiSegments = cdConfig.discPhiSegments;
  }
}

/// Write volume struct
void to_json(nlohmann::ordered_json& j,
             const TGeoDetector::Config::Volume& vol) {
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
  j["geo-tgeo-cyl-disc-split"] = vol.cylinderDiscSplit;

  Acts::TGeoCylinderDiscSplitter::Config cdConfig;
  cdConfig.cylinderLongitudinalSegments = vol.cylinderNZSegments;
  cdConfig.cylinderPhiSegments = vol.cylinderNPhiSegments;
  cdConfig.discRadialSegments = vol.discNRSegments;
  cdConfig.discPhiSegments = vol.discNPhiSegments;

  j["Splitters"]["CylinderDisk"] = cdConfig;
}

}  // namespace ActsExamples