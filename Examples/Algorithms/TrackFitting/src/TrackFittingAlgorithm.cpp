// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TrackFitting/TrackFittingAlgorithm.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/TrackProxy.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Result.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/MeasurementCalibration.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/TrackFitting/TrackFitterFunction.hpp"

#include <cstddef>
#include <functional>
#include <ostream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

ActsExamples::TrackFittingAlgorithm::TrackFittingAlgorithm(
    Config config, Acts::Logging::Level level)
    : ActsExamples::IAlgorithm("TrackFittingAlgorithm", level),
      m_cfg(std::move(config)) {
  if (m_cfg.inputMeasurements.empty()) {
    throw std::invalid_argument("Missing input measurement collection");
  }
  if (m_cfg.inputProtoTracks.empty()) {
    throw std::invalid_argument("Missing input proto tracks collection");
  }
  if (m_cfg.inputInitialTrackParameters.empty()) {
    throw std::invalid_argument(
        "Missing input initial track parameters collection");
  }
  if (m_cfg.outputTracks.empty()) {
    throw std::invalid_argument("Missing output tracks collection");
  }
  if (!m_cfg.calibrator) {
    throw std::invalid_argument("Missing calibrator");
  }
  if (m_cfg.inputClusters.empty() && m_cfg.calibrator->needsClusters()) {
    throw std::invalid_argument("The configured calibrator needs clusters");
  }

  m_inputMeasurements.initialize(m_cfg.inputMeasurements);
  m_inputProtoTracks.initialize(m_cfg.inputProtoTracks);
  m_inputInitialTrackParameters.initialize(m_cfg.inputInitialTrackParameters);
  m_inputClusters.maybeInitialize(m_cfg.inputClusters);
  m_outputTracks.initialize(m_cfg.outputTracks);
}

ActsExamples::ProcessCode ActsExamples::TrackFittingAlgorithm::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  // Read input data
  const auto& measurements = m_inputMeasurements(ctx);
  const auto& protoTracks = m_inputProtoTracks(ctx);
  const auto& initialParameters = m_inputInitialTrackParameters(ctx);

  const ClusterContainer* clusters =
      m_inputClusters.isInitialized() ? &m_inputClusters(ctx) : nullptr;

  // Consistency cross checks
  if (protoTracks.size() != initialParameters.size()) {
    ACTS_FATAL("Inconsistent number of proto tracks and parameters "
               << protoTracks.size() << " vs " << initialParameters.size());
    return ProcessCode::ABORT;
  }

  // Construct a perigee surface as the target surface
  auto pSurface = Acts::Surface::makeShared<Acts::PerigeeSurface>(
      Acts::Vector3{0., 0., 0.});

  // Measurement calibrator must be instantiated here, because we need the
  // measurements to construct it. The other extensions are hold by the
  // fit-function-object
  ActsExamples::MeasurementCalibratorAdapter calibrator(*(m_cfg.calibrator),
                                                        measurements, clusters);

  TrackFitterFunction::GeneralFitterOptions options{
      ctx.geoContext, ctx.magFieldContext, ctx.calibContext, pSurface.get(),
      Acts::PropagatorPlainOptions(ctx.geoContext, ctx.magFieldContext)};

  auto trackContainer = std::make_shared<Acts::VectorTrackContainer>();
  auto trackStateContainer = std::make_shared<Acts::VectorMultiTrajectory>();
  TrackContainer tracks(trackContainer, trackStateContainer);

  // Perform the fit for each input track
  std::vector<Acts::SourceLink> trackSourceLinks;
  for (std::size_t itrack = 0; itrack < protoTracks.size(); ++itrack) {
    // Check if you are not in picking mode
    if (m_cfg.pickTrack > -1 &&
        static_cast<std::size_t>(m_cfg.pickTrack) != itrack) {
      continue;
    }

    // The list of hits and the initial start parameters
    const auto& protoTrack = protoTracks[itrack];
    const auto& initialParams = initialParameters[itrack];

    // We can have empty tracks which must give empty fit results so the number
    // of entries in input and output containers matches.
    if (protoTrack.empty()) {
      ACTS_WARNING("Empty track " << itrack << " found.");
      continue;
    }

    ACTS_VERBOSE("Initial 4 position: "
                 << initialParams.fourPosition(ctx.geoContext).transpose());
    ACTS_VERBOSE(
        "Initial direction: " << initialParams.direction().transpose());
    ACTS_VERBOSE("Initial momentum: " << initialParams.absoluteMomentum());

    // Clear & reserve the right size
    trackSourceLinks.clear();
    trackSourceLinks.reserve(protoTrack.size());

    // Fill the source links via their indices from the container
    for (auto measIndex : protoTrack) {
      ConstVariableBoundMeasurementProxy measurement =
          measurements.getMeasurement(measIndex);
      IndexSourceLink sourceLink(measurement.geometryId(), measIndex);
      trackSourceLinks.push_back(Acts::SourceLink(sourceLink));
    }

    ACTS_VERBOSE("Invoke fitter for track " << itrack);
    auto result = (*m_cfg.fit)(trackSourceLinks, initialParams, options,
                               calibrator, tracks);

    if (result.ok()) {
      // Get the fit output object
      const auto& track = result.value();
      if (track.hasReferenceSurface()) {
        ACTS_VERBOSE("Fitted parameters for track " << itrack);
        ACTS_VERBOSE("  " << track.parameters().transpose());
        ACTS_VERBOSE("Measurements: (prototrack->track): "
                     << protoTrack.size() << " -> " << track.nMeasurements());
      } else {
        ACTS_VERBOSE("No fitted parameters for track " << itrack);
      }
    } else {
      ACTS_WARNING("Fit failed for track "
                   << itrack << " with error: " << result.error() << ", "
                   << result.error().message());
    }
  }

  std::stringstream ss;
  trackStateContainer->statistics().toStream(ss);
  ACTS_DEBUG(ss.str());

  ConstTrackContainer constTracks{
      std::make_shared<Acts::ConstVectorTrackContainer>(
          std::move(*trackContainer)),
      std::make_shared<Acts::ConstVectorMultiTrajectory>(
          std::move(*trackStateContainer))};

  m_outputTracks(ctx, std::move(constTracks));
  return ActsExamples::ProcessCode::SUCCESS;
}
