// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/EDM4hep/EDM4hepMeasurementReader.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Plugins/EDM4hep/TrackerHitCompatibility.hpp"
#include "Acts/Plugins/Podio/PodioUtil.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepUtil.hpp"

#include <list>
#include <stdexcept>

#include <edm4hep/TrackerHitPlane.h>
#include <edm4hep/TrackerHitPlaneCollection.h>

namespace ActsExamples {

EDM4hepMeasurementReader::EDM4hepMeasurementReader(
    const EDM4hepMeasurementReader::Config& config, Acts::Logging::Level level)
    : m_cfg(config),
      m_logger(Acts::getDefaultLogger("EDM4hepMeasurementReader", level)) {
  if (m_cfg.outputMeasurements.empty()) {
    throw std::invalid_argument("Missing measurement output collection");
  }

  m_eventsRange = std::make_pair(0, reader().getEntries("events"));

  m_outputMeasurements.initialize(m_cfg.outputMeasurements);
  m_outputMeasurementSimHitsMap.initialize(m_cfg.outputMeasurementSimHitsMap);
  m_outputClusters.maybeInitialize(m_cfg.outputClusters);
}

std::string EDM4hepMeasurementReader::EDM4hepMeasurementReader::name() const {
  return "EDM4hepMeasurementReader";
}

std::pair<std::size_t, std::size_t> EDM4hepMeasurementReader::availableEvents()
    const {
  return m_eventsRange;
}

ProcessCode EDM4hepMeasurementReader::read(const AlgorithmContext& ctx) {
  MeasurementContainer measurements;
  ClusterContainer clusters;
  // TODO what about those?
  IndexMultimap<Index> measurementSimHitsMap;

  podio::Frame frame = reader().readEntry("events", ctx.eventNumber);

  const auto& trackerHitPlaneCollection =
      frame.get<edm4hep::TrackerHitPlaneCollection>("ActsTrackerHitsPlane");
  const auto& trackerHitRawCollection =
      frame.get<edm4hep::TrackerHit3DCollection>("ActsTrackerHitsRaw");

  for (const auto& trackerHitPlane : trackerHitPlaneCollection) {
    Cluster cluster;
    EDM4hepUtil::readMeasurement(
        measurements, trackerHitPlane, &trackerHitRawCollection, &cluster,
        [](std::uint64_t cellId) { return Acts::GeometryIdentifier(cellId); });

    clusters.push_back(std::move(cluster));
  }

  // Write the data to the EventStore
  m_outputMeasurements(ctx, std::move(measurements));
  m_outputMeasurementSimHitsMap(ctx, std::move(measurementSimHitsMap));
  if (!m_cfg.outputClusters.empty()) {
    m_outputClusters(ctx, std::move(clusters));
  }

  return ProcessCode::SUCCESS;
}

Acts::PodioUtil::ROOTReader& EDM4hepMeasurementReader::reader() {
  bool exists = false;
  auto& reader = m_reader.local(exists);
  if (!exists) {
    reader.openFile(m_cfg.inputPath);
  }

  return reader;
}

}  // namespace ActsExamples
