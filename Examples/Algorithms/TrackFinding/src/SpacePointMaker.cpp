// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TrackFinding/SpacePointMaker.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/SpacePointFormation/SpacePointBuilderConfig.hpp"
#include "Acts/SpacePointFormation/SpacePointBuilderOptions.hpp"
#include "ActsExamples/EventData/GeometryContainers.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Utilities/GroupBy.hpp"
#include "ActsExamples/Utilities/Range.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <variant>

ActsExamples::SpacePointMaker::SpacePointMaker(Config cfg,
                                               Acts::Logging::Level lvl)
    : IAlgorithm("SpacePointMaker", lvl), m_cfg(std::move(cfg)) {
  if (m_cfg.inputSourceLinks.empty()) {
    throw std::invalid_argument("Missing source link input collection");
  }
  if (m_cfg.inputMeasurements.empty()) {
    throw std::invalid_argument("Missing measurement input collection");
  }
  if (m_cfg.outputSpacePoints.empty()) {
    throw std::invalid_argument("Missing space point output collection");
  }
  if (!m_cfg.trackingGeometry) {
    throw std::invalid_argument("Missing tracking geometry");
  }
  if (m_cfg.geometrySelection.empty()) {
    throw std::invalid_argument("Missing space point maker geometry selection");
  }

  m_inputSourceLinks.initialize(m_cfg.inputSourceLinks);
  m_inputMeasurements.initialize(m_cfg.inputMeasurements);
  m_outputSpacePoints.initialize(m_cfg.outputSpacePoints);

  // ensure geometry selection contains only valid inputs
  for (const auto& geoId : m_cfg.geometrySelection) {
    if ((geoId.approach() != 0u) || (geoId.boundary() != 0u) ||
        (geoId.sensitive() != 0u)) {
      throw std::invalid_argument(
          "Invalid geometry selection: only volume and layer are allowed to be "
          "set");
    }
  }
  // remove geometry selection duplicates
  //
  // the geometry selections must be mutually exclusive, i.e. if we have a
  // selection that contains both a volume and a layer within that same volume,
  // we would create the space points for the layer twice.
  auto isDuplicate = [](Acts::GeometryIdentifier ref,
                        Acts::GeometryIdentifier cmp) {
    // code assumes ref < cmp and that only volume and layer can be non-zero
    // root node always contains everything
    if (ref.volume() == 0) {
      return true;
    }
    // unequal volumes always means separate hierarchies
    if (ref.volume() != cmp.volume()) {
      return false;
    }
    // within the same volume hierarchy only consider layers
    return (ref.layer() == cmp.layer());
  };
  // sort geometry selection so the unique filtering works
  std::ranges::sort(m_cfg.geometrySelection,
                    std::less<Acts::GeometryIdentifier>{});
  auto geoSelBeg = m_cfg.geometrySelection.begin();
  auto geoSelEnd = m_cfg.geometrySelection.end();
  auto geoSelLastUnique = std::unique(geoSelBeg, geoSelEnd, isDuplicate);
  if (geoSelLastUnique != geoSelEnd) {
    ACTS_WARNING("Removed " << std::distance(geoSelLastUnique, geoSelEnd)
                            << " geometry selection duplicates");
    m_cfg.geometrySelection.erase(geoSelLastUnique, geoSelEnd);
  }
  ACTS_INFO("Space point geometry selection:");
  for (const auto& geoId : m_cfg.geometrySelection) {
    ACTS_INFO("  " << geoId);
  }
  auto spBuilderConfig = Acts::SpacePointBuilderConfig();
  spBuilderConfig.trackingGeometry = m_cfg.trackingGeometry;

  m_slSurfaceAccessor.emplace(
      IndexSourceLink::SurfaceAccessor{*m_cfg.trackingGeometry});
  spBuilderConfig.slSurfaceAccessor
      .connect<&IndexSourceLink::SurfaceAccessor::operator()>(
          &m_slSurfaceAccessor.value());

  auto spConstructor =
      [](const Acts::Vector3& pos, std::optional<double> t,
         const Acts::Vector2& cov, std::optional<double> varT,
         boost::container::static_vector<Acts::SourceLink, 2> slinks)
      -> SimSpacePoint {
    return SimSpacePoint(pos, t, cov[0], cov[1], varT, std::move(slinks));
  };

  m_spacePointBuilder = Acts::SpacePointBuilder<SimSpacePoint>(
      spBuilderConfig, spConstructor,
      Acts::getDefaultLogger("SpacePointBuilder", lvl));
}

ActsExamples::ProcessCode ActsExamples::SpacePointMaker::execute(
    const AlgorithmContext& ctx) const {
  const auto& sourceLinks = m_inputSourceLinks(ctx);
  const auto& measurements = m_inputMeasurements(ctx);

  // TODO Support strip measurements
  Acts::SpacePointBuilderOptions spOpt;

  spOpt.paramCovAccessor = [&measurements](Acts::SourceLink slink) {
    const auto islink = slink.get<IndexSourceLink>();
    const ConstVariableBoundMeasurementProxy meas =
        measurements.getMeasurement(islink.index());

    return std::make_pair(meas.fullParameters(), meas.fullCovariance());
  };

  SimSpacePointContainer spacePoints;
  for (Acts::GeometryIdentifier geoId : m_cfg.geometrySelection) {
    // select volume/layer depending on what is set in the geometry id
    auto range = selectLowestNonZeroGeometryObject(sourceLinks, geoId);
    // groupByModule only works with geometry containers, not with an
    // arbitrary range. do the equivalent grouping manually
    auto groupedByModule = makeGroupBy(range, detail::GeometryIdGetter());

    for (const auto& [moduleGeoId, moduleSourceLinks] : groupedByModule) {
      for (const auto& sourceLink : moduleSourceLinks) {
        m_spacePointBuilder.buildSpacePoint(
            ctx.geoContext, {Acts::SourceLink{sourceLink}}, spOpt,
            std::back_inserter(spacePoints));
      }
    }
  }

  spacePoints.shrink_to_fit();

  ACTS_DEBUG("Created " << spacePoints.size() << " space points");
  m_outputSpacePoints(ctx, std::move(spacePoints));

  return ActsExamples::ProcessCode::SUCCESS;
}
