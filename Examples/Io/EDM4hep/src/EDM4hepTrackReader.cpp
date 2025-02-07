// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/EDM4hep/EDM4hepTrackReader.hpp"

#include "Acts/Plugins/EDM4hep/EDM4hepUtil.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepUtil.hpp"

#include <stdexcept>

#include <edm4hep/TrackCollection.h>
#include <podio/Frame.h>

namespace ActsExamples {

EDM4hepTrackReader::EDM4hepTrackReader(const Config& config,
                                       Acts::Logging::Level level)
    : m_cfg(config),
      m_logger(Acts::getDefaultLogger("EDM4hepSimHitReader", level)) {
  if (m_cfg.outputTracks.empty()) {
    throw std::invalid_argument("Missing output trajectories collection");
  }

  m_outputTracks.initialize(m_cfg.outputTracks);

  m_eventsRange = {0, reader().getEntries("events")};
}

std::pair<std::size_t, std::size_t> EDM4hepTrackReader::availableEvents()
    const {
  return m_eventsRange;
}

std::string EDM4hepTrackReader::EDM4hepTrackReader::name() const {
  return "EDM4hepTrackReader";
}

ProcessCode EDM4hepTrackReader::read(const AlgorithmContext& ctx) {
  podio::Frame frame = reader().readEntry("events", ctx.eventNumber);

  const auto& trackCollection =
      frame.get<edm4hep::TrackCollection>(m_cfg.inputTracks);

  auto trackContainer = std::make_shared<Acts::VectorTrackContainer>();
  auto trackStateContainer = std::make_shared<Acts::VectorMultiTrajectory>();
  TrackContainer tracks(trackContainer, trackStateContainer);

  for (const auto& inputTrack : trackCollection) {
    auto track = tracks.makeTrack();
    Acts::EDM4hepUtil::readTrack(inputTrack, track, m_cfg.Bz);
  }

  ConstTrackContainer constTracks{
      std::make_shared<Acts::ConstVectorTrackContainer>(
          std::move(*trackContainer)),
      std::make_shared<Acts::ConstVectorMultiTrajectory>(
          std::move(*trackStateContainer))};

  m_outputTracks(ctx, std::move(constTracks));

  return ProcessCode::SUCCESS;
}

Acts::PodioUtil::ROOTReader& EDM4hepTrackReader::reader() {
  bool exists = false;
  auto& reader = m_reader.local(exists);
  if (!exists) {
    reader.openFile(m_cfg.inputPath);
  }

  return reader;
}

}  // namespace ActsExamples
