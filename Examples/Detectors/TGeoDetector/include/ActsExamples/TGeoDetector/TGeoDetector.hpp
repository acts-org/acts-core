// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Material/IMaterialDecorator.hpp"
#include "Acts/Plugins/TGeo/TGeoLayerBuilder.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/DetectorCommons/Detector.hpp"
#include "ActsExamples/Utilities/Options.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ActsExamples {

class TGeoDetector : public Detector {
 public:
  struct Config {
    Acts::Logging::Level logLevel = Acts::Logging::WARNING;
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

    std::shared_ptr<Acts::IMaterialDecorator> materialDecorator;

    enum SubVolume : std::size_t { Negative = 0, Central, Positive };

    template <typename T>
    struct LayerTriplet {
      LayerTriplet() = default;

      explicit LayerTriplet(T value)
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

  explicit TGeoDetector(const Config& cfg);

 private:
  Config m_cfg;
};

}  // namespace ActsExamples
