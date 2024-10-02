// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TrackFindingExaTrkX/PrototracksToParameters.hpp"

#include "Acts/Seeding/BinnedGroup.hpp"
#include "Acts/Seeding/EstimateTrackParamsFromSeed.hpp"
#include "Acts/Seeding/SeedFilter.hpp"
#include "Acts/Seeding/SeedFinder.hpp"
#include "Acts/Seeding/SeedFinderConfig.hpp"
#include "Acts/Utilities/Zip.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Utilities/EventDataTransforms.hpp"

#include <algorithm>
#include <tuple>

using namespace ActsExamples;
using namespace Acts::UnitLiterals;

namespace ActsExamples {

PrototracksToParameters::PrototracksToParameters(Config cfg,
                                                 Acts::Logging::Level lvl)
    : IAlgorithm("PrototracksToParsAndSeeds", lvl), m_cfg(std::move(cfg)) {
  m_outputSeeds.initialize(m_cfg.outputSeeds);
  m_outputProtoTracks.initialize(m_cfg.outputProtoTracks);
  m_inputProtoTracks.initialize(m_cfg.inputProtoTracks);
  m_inputSpacePoints.initialize(m_cfg.inputSpacePoints);
  m_outputParameters.initialize(m_cfg.outputParameters);

  if (m_cfg.geometry == nullptr) {
    throw std::invalid_argument("No geometry given");
  }
  if (m_cfg.magneticField == nullptr) {
    throw std::invalid_argument("No magnetic field given");
  }

  // Set up the track parameters covariance (the same for all tracks)
  for (std::size_t i = Acts::eBoundLoc0; i < Acts::eBoundSize; ++i) {
    m_covariance(i, i) = m_cfg.initialVarInflation[i] * m_cfg.initialSigmas[i] *
                         m_cfg.initialSigmas[i];
  }
}

PrototracksToParameters::~PrototracksToParameters() {}

ProcessCode PrototracksToParameters::execute(
    const AlgorithmContext &ctx) const {
  auto bCache = m_cfg.magneticField->makeCache(ctx.magFieldContext);
  const auto &sps = m_inputSpacePoints(ctx);
  auto prototracks = m_inputProtoTracks(ctx);

  // Make some lookup tables. Allocate space for the maximum number of indices
  // (max 2 source links per spacepoint)
  std::vector<const SimSpacePoint *> indexToSpacepoint(2 * sps.size(), nullptr);
  std::vector<Acts::GeometryIdentifier> indexToGeoId(
      2 * sps.size(), Acts::GeometryIdentifier{0});

  for (const auto &sp : sps) {
    for (const auto &sl : sp.sourceLinks()) {
      const auto &isl = sl.template get<IndexSourceLink>();
      indexToSpacepoint[isl.index()] = &sp;
      indexToGeoId[isl.index()] = isl.geometryId();
    }
  }

  ProtoTrackContainer seededTracks;
  seededTracks.reserve(prototracks.size());

  SimSeedContainer seeds;
  seeds.reserve(prototracks.size());

  TrackParametersContainer parameters;
  parameters.reserve(prototracks.size());

  // Loop over the prototracks to make seeds
  ProtoTrack tmpTrack;
  std::vector<const SimSpacePoint *> tmpSps;
  std::size_t skippedTracks = 0;
  for (auto &track : prototracks) {
    ACTS_VERBOSE("Try to get seed from prototrack with " << track.size()
                                                         << " hits");
    // Make prototrack unique with respect to volume and layer
    // so we don't get a seed where we have two spacepoints on the same layer

    // Here, we want to create a seed only if the prototrack with removed unique
    // layer-volume spacepoints has 3 or more hits. However, if this is the
    // case, we want to keep the whole prototrack. Therefore, we operate on a
    // tmpTrack.
    std::ranges::sort(track, {}, [&](const auto &t) {
      return std::make_tuple(indexToGeoId[t].volume(), indexToGeoId[t].layer());
    });

    tmpTrack.clear();
    std::unique_copy(
        track.begin(), track.end(), std::back_inserter(tmpTrack),
        [&](auto a, auto b) {
          return indexToGeoId[a].volume() == indexToGeoId[b].volume() &&
                 indexToGeoId[a].layer() == indexToGeoId[b].layer();
        });

    // in this case we cannot seed properly
    if (tmpTrack.size() < 3) {
      ACTS_DEBUG(
          "Cannot seed because less then three hits with unique (layer, "
          "volume)");
      skippedTracks++;
      continue;
    }

    // Make the seed
    tmpSps.clear();
    std::transform(track.begin(), track.end(), std::back_inserter(tmpSps),
                   [&](auto i) { return indexToSpacepoint[i]; });
    tmpSps.erase(std::remove_if(tmpSps.begin(), tmpSps.end(),
                                [](auto sp) { return sp == nullptr; }),
                 tmpSps.end());

    if (tmpSps.size() < 3) {
      ACTS_WARNING("Could not find all spacepoints, skip");
      skippedTracks++;
      continue;
    }

    std::ranges::sort(tmpSps, {}, [](const auto &t) { return t->r(); });

    // Simply use r = m*z + t and solve for r=0 to find z vertex position...
    // Probably not the textbook way to do
    const auto m = (tmpSps.back()->r() - tmpSps.front()->r()) /
                   (tmpSps.back()->z() - tmpSps.front()->z());
    const auto t = tmpSps.front()->r() - m * tmpSps.front()->z();
    const auto z_vertex = -t / m;
    const auto s = tmpSps.size();

    SimSeed seed = m_cfg.buildTightSeeds
                       ? SimSeed(*tmpSps[0], *tmpSps[1], *tmpSps[2])
                       : SimSeed(*tmpSps[0], *tmpSps[s / 2], *tmpSps[s - 1]);
    seed.setVertexZ(z_vertex);

    // Compute parameters
    const auto &bottomSP = seed.sp().front();
    const auto geoId = bottomSP->sourceLinks()
                           .front()
                           .template get<IndexSourceLink>()
                           .geometryId();
    const auto &surface = *m_cfg.geometry->findSurface(geoId);

    auto field = m_cfg.magneticField->getField(
        {bottomSP->x(), bottomSP->y(), bottomSP->z()}, bCache);
    if (!field.ok()) {
      ACTS_ERROR("Field lookup error: " << field.error());
      return ProcessCode::ABORT;
    }

    auto pars = Acts::estimateTrackParamsFromSeed(
        ctx.geoContext, seed.sp().begin(), seed.sp().end(), surface, *field,
        m_cfg.bFieldMin);

    if (not pars) {
      ACTS_WARNING("Skip track because of bad params");
    }

    seededTracks.push_back(track);
    seeds.emplace_back(std::move(seed));
    parameters.push_back(Acts::BoundTrackParameters(
        surface.getSharedPtr(), *pars, m_covariance, m_cfg.particleHypothesis));
  }

  if (skippedTracks > 0) {
    ACTS_WARNING("Skipped seeding of " << skippedTracks);
  }

  ACTS_DEBUG("Seeded " << seeds.size() << " out of " << prototracks.size()
                       << " prototracks");

  m_outputSeeds(ctx, std::move(seeds));
  m_outputProtoTracks(ctx, std::move(seededTracks));
  m_outputParameters(ctx, std::move(parameters));

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
