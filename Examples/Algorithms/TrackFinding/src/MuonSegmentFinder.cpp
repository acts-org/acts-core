

#include "Acts/Seeding/StrawChamberLineSeeder.hpp"

#include "ActsExamples/TrackFinding/MuonSegmentFinder.hpp"
#include "ActsExamples/EventData/MuonSpacePoint.hpp"


namespace ActsExamples{



    MuonSegmentFinder::MuonSegmentFinder(Config cfg, Acts::Logging::Level lvl)
    : IAlgorithm("MuonSegmentFinder", lvl),
      m_cfg(std::move(cfg)),
      m_logger(Acts::getDefaultLogger("MuonSegmentFinder", lvl)) {
//   if (m_cfg.inDriftCircles.empty()) {
//     throw std::invalid_argument(
//         "MuonSegmentFinder: Missing drift circle collection");
//   }
//   if (m_cfg.inSimHits.empty()) {
//     throw std::invalid_argument("MuonSegmentFinder: Missing sim hit collection");
//   }

//   m_inputDriftCircles.initialize(m_cfg.inDriftCircles);
//   m_inputSimHits.initialize(m_cfg.inSimHits);
}

ProcessCode MuonSegmentFinder::execute( const AlgorithmContext& ctx) const {
  
  using StrawSeeder_t = Acts::StrawChamberLineSeeder<MuonSpacePoint, MuonSpacePointSorter>;
  StrawSeeder_t::Config seedCfg{};
  std::vector<const MuonSpacePoint*> hits{};
  
  StrawSeeder_t seeder{hits, std::move(seedCfg), m_logger->clone()};
  return ProcessCode::SUCCESS;
}

ProcessCode MuonSegmentFinder::initialize() {
  return ProcessCode::SUCCESS;
}
ProcessCode MuonSegmentFinder::finalize() {
  return ProcessCode::SUCCESS;
}
}