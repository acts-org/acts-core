// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/EDM4hep/EDM4hepMeasurementWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Plugins/EDM4hep/TrackerHitCompatibility.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepUtil.hpp"

#include <stdexcept>

#include <edm4hep/TrackerHitPlane.h>
#include <edm4hep/TrackerHitPlaneCollection.h>
#include <podio/Frame.h>

namespace ActsExamples {

EDM4hepMeasurementWriter::EDM4hepMeasurementWriter(
    const EDM4hepMeasurementWriter::Config& config, Acts::Logging::Level level)
    : WriterT(config.inputMeasurements, "EDM4hepMeasurementWriter", level),
      m_cfg(config),
      m_writer(config.outputPath) {
  ACTS_VERBOSE("Created output file " << config.outputPath);

  // Input container for measurements is already checked by base constructor
  m_inputClusters.maybeInitialize(m_cfg.inputClusters);
}

ActsExamples::ProcessCode EDM4hepMeasurementWriter::finalize() {
  m_writer.finish();

  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode EDM4hepMeasurementWriter::writeT(
    const AlgorithmContext& ctx, const MeasurementContainer& measurements) {
  ClusterContainer clusters;

  podio::Frame frame;

  edm4hep::TrackerHitPlaneCollection hitsPlane;
  edm4hep::TrackerHit3DCollection hits;

  if (!m_cfg.inputClusters.empty()) {
    ACTS_VERBOSE("Fetch clusters for writing: " << m_cfg.inputClusters);
    clusters = m_inputClusters(ctx);
  }

  ACTS_VERBOSE("Writing " << measurements.size()
                          << " measurements in this event.");

  for (Index hitIdx = 0u; hitIdx < measurements.size(); ++hitIdx) {
    ConstVariableBoundMeasurementProxy from =
        measurements.getMeasurement(hitIdx);
    const Cluster* fromCluster = clusters.empty() ? nullptr : &clusters[hitIdx];

    auto to = hitsPlane.create();
    EDM4hepUtil::writeMeasurement(
        from, to, fromCluster, hits,
        [](Acts::GeometryIdentifier id) { return id.value(); });
  }

  frame.put(std::move(hitsPlane), "ActsTrackerHitsPlane");
  frame.put(std::move(hits), "ActsTrackerHitsRaw");

  std::lock_guard guard(m_writeMutex);
  m_writer.writeFrame(frame, "events");

  return ActsExamples::ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
