// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Traccc Plugin include(s)
#include "Acts/Plugins/Traccc/CellConversion.hpp"
#include "Acts/Plugins/Traccc/TrackConversion.hpp"

// Acts Examples include(s)
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Traccc/Common/Conversion/CellMapConversion.hpp"
#include "ActsExamples/Traccc/Common/Conversion/DigitizationConversion.hpp"
#include "ActsExamples/Traccc/Common/Conversion/MeasurementConversion.hpp"
#include "ActsExamples/Traccc/Common/Measurement/Debug.hpp"
#include "ActsExamples/Traccc/Common/Measurement/MeasurementMatch.hpp"
#include "ActsExamples/Traccc/Common/Util/IndexMap.hpp"

// Acts include(s)
#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData//TrackContainer.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Measurement.hpp"

// Detray include(s).
#include "detray/core/detector.hpp"

// Traccc include(s).
#include "traccc/geometry/geometry.hpp"

// VecMem include(s).
#include <vecmem/memory/memory_resource.hpp>

// System include(s)
#include <memory>
#include <sstream>
#include <string>

namespace ActsExamples::Traccc::Common {

/// @brief Class for converting Acts data to traccc data.
class Converter {
 private:
  using detector_t =
      detray::detector<detray::default_metadata, detray::host_container_types>;

  const Acts::TrackingGeometry& trackingGeometry;
  const detector_t& detector;

  // Cache the converted digitalization configuration, the surface transforms,
  // and the barcode map.
  const traccc::digitization_config digitizationConfig;
  const traccc::geometry surfaceTransforms;
  const std::map<std::uint64_t, detray::geometry::barcode> barcodeMap;

  const Acts::Logger& actsLogger;

  /// @brief Writes the number of traccc measurements and Acts measurements to the logger.
  /// If the number of measurements do not matching a warning is shown.
  /// @param tracccMeasurements the traccc measurements.
  /// @param measurements the Acts measurements.
  template <typename allocator_t>
  void logMeasurementCountComparison(
      const std::vector<traccc::measurement, allocator_t>& tracccMeasurements,
      const std::vector<ActsExamples::BoundVariantMeasurement>& measurements)
      const {
    if (tracccMeasurements.size() != measurements.size()) {
      std::stringstream ss;
      ss << "Number of measurements do not match (traccc: "
         << tracccMeasurements.size() << ", acts: " << measurements.size()
         << ")\n"
         << "Perhaps mergeCommonCorner or doMerge is false in the digitization "
            "algorithm config?";
      ACTS_WARNING(ss.str());
    } else {
      std::stringstream ss;
      ss << "Number of Acts and Traccc measurements match (count: "
         << measurements.size() << ")";
      ACTS_INFO(ss.str());
    }
  }

  /// @brief Creates a map from traccc measurements to acts measurements.
  /// The traccc elements will map to an acts measurement that it is equivalent
  /// to. The resulting map is assumed to be bijective, thus if any element is
  /// unable to find a match an error is thrown.
  /// @param tracccMeasurements the traccc measurements.
  /// @param measurements the acts measurements.
  /// @return A map from traccc measurement to acts bound variant measurement.
  template <typename allocator_t>
  std::map<traccc::measurement, ActsExamples::BoundVariantMeasurement>
  measurementConversionMap(
      const std::vector<traccc::measurement, allocator_t>& tracccMeasurements,
      const std::vector<ActsExamples::BoundVariantMeasurement>& measurements)
      const {
    logMeasurementCountComparison(tracccMeasurements, measurements);

    auto convertedMeasurements =
        Conversion::createActsMeasurements(detector, tracccMeasurements);
    auto indexMap = Measurement::matchMap(convertedMeasurements, measurements);

    ACTS_DEBUG(std::string("Traccc (1) and Acts (2) measurement index pairing "
                           "information:\n") +
               Measurement::pairingStatistics(convertedMeasurements,
                                              measurements, indexMap));

    return Util::referenceMap(tracccMeasurements, measurements, indexMap);
  }

  template <typename track_container_t, typename trajectory_t,
            template <typename> class holder_t>
  void mapMeasurements(
      Acts::TrackContainer<track_container_t, trajectory_t, holder_t>&
          trackContainer,
      const std::map<traccc::measurement, ActsExamples::BoundVariantMeasurement>
          map) const {
    for (auto track : trackContainer) {
      for (auto trackState : track.trackStates()) {
        const auto tracccMeasurement = trackState.getUncalibratedSourceLink()
                                           .template get<traccc::measurement>();
        const ActsExamples::BoundVariantMeasurement& measurement =
            map.at(tracccMeasurement);
        std::visit(
            [&trackState](auto& m) {
              trackState.setUncalibratedSourceLink(m.sourceLink());

              using MeasurementType = std::decay_t<decltype(m)>;
              constexpr std::size_t size = MeasurementType::size();
              trackState.allocateCalibrated(m.size());
              assert(trackState.hasCalibrated());

              trackState.template calibrated<size>() = m.parameters();
              trackState.template calibratedCovariance<size>() = m.covariance();
              trackState.setProjector(m.projector());
            },
            measurement);
      }
    }
  }

 protected:
  const Acts::Logger& logger() const { return actsLogger; }

 public:
  Converter(const Acts::TrackingGeometry& tg, const detector_t& det,
            const traccc::digitization_config& dc, const traccc::geometry& sf,
            const std::map<std::uint64_t, detray::geometry::barcode>& bm,
            const Acts::Logger& l)
      : trackingGeometry(tg),
        detector(det),
        digitizationConfig(dc),
        surfaceTransforms(sf),
        barcodeMap(bm),
        actsLogger(l) {}

  /// @brief Converts a map of cells to traccc cells and modules.
  /// @param map The geometry id -> cell collection map which corresponds to each geometry's cells.
  /// @param mr The memory resource to use.
  /// @returns a tuple containing traccc input data, i.e. (cells, modules).
  auto convertCells(
      const std::map<Acts::GeometryIdentifier, std::vector<Cluster::Cell>>& map,
      vecmem::memory_resource* mr) const {
    auto tcm = Conversion::tracccCellsMap(map);
    auto res = Acts::TracccPlugin::createCellsAndModules(
        mr, tcm, &surfaceTransforms, &digitizationConfig, &barcodeMap);

    std::stringstream ss;
    ss << "Successfully converted Acts cells (obtained "
       << std::get<0>(res).size() << " traccc cells and "
       << std::get<1>(res).size() << " traccc modules)";
    ACTS_INFO(ss.str());

    return res;
  }

  /// @brief Converts a container of traccc tracks to a container of Acts tracks.
  /// The given traccc measurements are compared with the given acts
  /// measurements to determine the mapping between measurements and ensure that
  /// the two collections of measurements are equivalent. The newly created Acts
  /// tracks will use measurements from the acts measurement container. The Acts
  /// measurements provided will be used for setting both the calibrated and
  /// uncalibrated measurements/sourcelinks.
  /// @param tracccTrackContainer The traccc tracks.
  /// @param tracccMeasurements the traccc measurements.
  /// @param measurements the Acts measurements.
  /// @return An Acts const track container.
  template <typename traccc_track_container_t, typename allocator_t>
  auto convertTracks(
      traccc_track_container_t& tracccTrackContainer,
      const std::vector<traccc::measurement, allocator_t>& tracccMeasurements,
      const std::vector<ActsExamples::BoundVariantMeasurement>& measurements)
      const {
    auto trackContainer = std::make_shared<Acts::VectorTrackContainer>();
    auto trackStateContainer = std::make_shared<Acts::VectorMultiTrajectory>();
    TrackContainer tracks(trackContainer, trackStateContainer);

    Acts::TracccPlugin::makeTracks(tracccTrackContainer, tracks, detector,
                                   trackingGeometry);

    std::stringstream ss;
    ss << "Converted " << tracccTrackContainer.size() << " traccc tracks";
    ACTS_INFO(ss.str());

    auto mcm = measurementConversionMap(tracccMeasurements, measurements);

    ACTS_INFO(
        "Found a 1:1 mapping of indexes between traccc and Acts measurements");

    mapMeasurements(tracks, mcm);

    ACTS_INFO("Updated track state measurements");

    ConstTrackContainer constTracks{
        std::make_shared<Acts::ConstVectorTrackContainer>(
            std::move(*trackContainer)),
        std::make_shared<Acts::ConstVectorMultiTrajectory>(
            std::move(*trackStateContainer))};

    return constTracks;
  }
};

}  // namespace ActsExamples::Traccc::Common
