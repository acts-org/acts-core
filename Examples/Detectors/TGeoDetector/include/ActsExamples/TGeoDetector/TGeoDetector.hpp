// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Plugins/TGeo/TGeoLayerBuilder.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Utilities/Options.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Acts {
class TGeoDetectorElement;
class TrackingGeometry;
class IMaterialDecorator;
}  // namespace Acts

namespace ActsExamples {
class IContextDecorator;
}  // namespace ActsExamples

namespace ActsExamples {

struct TGeoDetector {
  using DetectorElementPtr = std::shared_ptr<const Acts::TGeoDetectorElement>;
  using DetectorStore = std::vector<DetectorElementPtr>;

  using ContextDecorators =
      std::vector<std::shared_ptr<ActsExamples::IContextDecorator>>;
  using TrackingGeometryPtr = std::shared_ptr<const Acts::TrackingGeometry>;

  /// The Store of the detector elements (lifetime: job)
  DetectorStore detectorStore;

  struct Config {
    Acts::Logging::Level surfaceLogLevel = Acts::Logging::WARNING;
    Acts::Logging::Level layerLogLevel = Acts::Logging::WARNING;
    Acts::Logging::Level volumeLogLevel = Acts::Logging::WARNING;

    void readJson(const std::string& jsonFile);

    std::string fileName;
    bool buildBeamPipe = false;
    double beamPipeRadius{0};
    double beamPipeHalflengthZ{0};
    double beamPipeLayerThickness{0};
    double beamPipeEnvelopeR{1.0};
    double layerEnvelopeR{1.0};

    double unitScalor = 1.0;

    Acts::TGeoLayerBuilder::ElementFactory elementFactory =
        Acts::TGeoLayerBuilder::defaultElementFactory;

    /// Optional geometry identifier hook to be used during closure
    std::shared_ptr<const Acts::GeometryIdentifierHook> geometryIdentifierHook =
        std::make_shared<Acts::GeometryIdentifierHook>();

    enum SubVolume : std::size_t { Negative = 0, Central, Positive };

    template <typename T>
    struct LayerTriplet {
      LayerTriplet() = default;

      LayerTriplet(T value)
          : negative{value}, central{value}, positive{value} {}

      LayerTriplet(T _negative, T _central, T _positive)
          : negative{_negative}, central{_central}, positive{_positive} {}

      T negative;
      T central;
      T positive;

      T& at(SubVolume i) {
        switch (i) {
          case Negative:
            return negative;
          case Central:
            return central;
          case Positive:
            return positive;
          default:
            throw std::invalid_argument{"Unknown index"};
        }
      }

      const T& at(SubVolume i) const {
        switch (i) {
          case Negative:
            return negative;
          case Central:
            return central;
          case Positive:
            return positive;
          default:
            throw std::invalid_argument{"Unknown index"};
        }
      }
    };

    struct Volume {
      std::string name;
      LayerTriplet<bool> layers{false};
      LayerTriplet<std::string> subVolumeName;
      LayerTriplet<std::vector<std::string>> sensitiveNames;
      LayerTriplet<std::string> sensitiveAxes;
      LayerTriplet<Options::Interval> rRange;
      LayerTriplet<Options::Interval> zRange;
      LayerTriplet<double> splitTolR{0};
      LayerTriplet<double> splitTolZ{0};
      LayerTriplet<std::vector<std::pair<int, Acts::BinningType>>> binning0;
      LayerTriplet<std::vector<std::pair<int, Acts::BinningType>>> binning1;

      Options::Interval binToleranceR;
      Options::Interval binTolerancePhi;
      Options::Interval binToleranceZ;

      bool cylinderDiscSplit = false;
      unsigned int cylinderNZSegments = 0;
      unsigned int cylinderNPhiSegments = 0;
      unsigned int discNRSegments = 0;
      unsigned int discNPhiSegments = 0;

      bool itkModuleSplit = false;
      std::map<std::string, unsigned int> barrelMap;
      std::map<std::string, std::vector<std::pair<double, double>>> discMap;
      /// pairs of regular expressions to match sensor names and category keys
      /// for either the barrelMap or the discMap
      std::map<std::string, std::string>
          splitPatterns;  // @TODO in principle vector<pair< > > would be good enough
    };

    std::vector<Volume> volumes;
  };

  static void readTGeoLayerBuilderConfigsFile(const std::string& path,
                                              Config& config);

  std::pair<TrackingGeometryPtr, ContextDecorators> finalize(
      const Config& cfg,
      std::shared_ptr<const Acts::IMaterialDecorator> mdecorator);
};

}  // namespace ActsExamples
