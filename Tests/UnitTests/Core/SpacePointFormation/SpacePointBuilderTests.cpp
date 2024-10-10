// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/GenericCurvilinearTrackParameters.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/detail/TestSourceLink.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/SpacePointFormation/SpacePointBuilder.hpp"
#include "Acts/SpacePointFormation/SpacePointBuilderConfig.hpp"
#include "Acts/SpacePointFormation/SpacePointBuilderOptions.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CubicTrackingGeometry.hpp"
#include "Acts/Tests/CommonHelpers/MeasurementsCreator.hpp"
#include "Acts/Tests/CommonHelpers/TestSpacePoint.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <utility>
#include <vector>

namespace bdata = boost::unit_test::data;
using namespace Acts::UnitLiterals;

namespace Acts::Test {

using StraightPropagator = Propagator<StraightLineStepper, Navigator>;
using TestSourceLink = detail::Test::TestSourceLink;
using ConstantFieldStepper = EigenStepper<>;
using ConstantFieldPropagator = Propagator<ConstantFieldStepper, Navigator>;
// Construct initial track parameters.
CurvilinearTrackParameters makeParameters(double phi, double theta, double p,
                                          double q) {
  // create covariance matrix from reasonable standard deviations
  BoundVector stddev;
  stddev[eBoundLoc0] = 100_um;
  stddev[eBoundLoc1] = 100_um;
  stddev[eBoundTime] = 25_ns;
  stddev[eBoundPhi] = 2_degree;
  stddev[eBoundTheta] = 2_degree;
  stddev[eBoundQOverP] = 1 / 100_GeV;
  BoundSquareMatrix cov = stddev.cwiseProduct(stddev).asDiagonal();
  // Let the particle start from the origin
  Vector4 mPos4(-3_m, 0., 0., 0.);
  return CurvilinearTrackParameters(mPos4, phi, theta, q / p, cov,
                                    ParticleHypothesis::pionLike(q));
}

std::pair<Vector3, Vector3> stripEnds(
    const std::shared_ptr<const TrackingGeometry>& geo,
    const GeometryContext& gctx, const SourceLink& slink,
    const double stripFrac = 0.4) {
  auto testslink = slink.get<TestSourceLink>();
  const auto lpos = testslink.parameters;

  Vector3 globalFakeMom(1, 1, 1);
  const auto geoId = testslink.m_geometryId;
  const Surface* surface = geo->findSurface(geoId);

  const double stripLength = 40.;
  const double end1x = lpos[0] + stripLength * stripFrac;
  const double end1y = lpos[1];
  const double end2x = lpos[0] - stripLength * (1 - stripFrac);
  const double end2y = lpos[1];
  const Vector2 lpos1(end1x, end1y);
  const Vector2 lpos2(end2x, end2y);

  auto gPos1 = surface->localToGlobal(gctx, lpos1, globalFakeMom);
  auto gPos2 = surface->localToGlobal(gctx, lpos2, globalFakeMom);

  return std::make_pair(gPos1, gPos2);
}

// Create a test context
GeometryContext tgContext = GeometryContext();

const GeometryContext geoCtx;
const MagneticFieldContext magCtx;

// detector geometry
CubicTrackingGeometry geometryStore(geoCtx);
const auto geometry = geometryStore();

// detector resolutions
const MeasurementResolution resPixel = {MeasurementType::eLoc01,
                                        {25_um, 50_um}};
const MeasurementResolution resStrip = {MeasurementType::eLoc01,
                                        {100_um, 100_um}};
const MeasurementResolutionMap resolutions = {
    {GeometryIdentifier().setVolume(2), resPixel},
    {GeometryIdentifier().setVolume(3).setLayer(2), resStrip},
    {GeometryIdentifier().setVolume(3).setLayer(4), resStrip},
    {GeometryIdentifier().setVolume(3).setLayer(6), resStrip},
    {GeometryIdentifier().setVolume(3).setLayer(8), resStrip},
};

std::default_random_engine rng(42);

BOOST_DATA_TEST_CASE(SpacePointBuilder_basic, bdata::xrange(1), index) {
  (void)index;

  double phi = 5._degree;
  double theta = 95._degree;
  double p = 50._GeV;
  double q = 1;

  Navigator navigator({
      geometry,
      true,  // sensitive
      true,  // material
      false  // passive
  });
  auto field = std::make_shared<ConstantBField>(Vector3(0.0, 0.0, 2._T));
  ConstantFieldStepper stepper(std::move(field));

  ConstantFieldPropagator propagator(std::move(stepper), std::move(navigator));
  auto start = makeParameters(phi, theta, p, q);

  auto measurements =
      createMeasurements(propagator, geoCtx, magCtx, start, resolutions, rng);

  const auto sourceLinks = measurements.sourceLinks;

  std::vector<SourceLink> frontSourceLinks;
  std::vector<SourceLink> backSourceLinks;
  std::vector<SourceLink> singleHitSourceLinks;

  std::vector<const Vector3*> frontStripEnds;
  std::vector<const Vector3*> backStripEnds;

  for (auto& sl : sourceLinks) {
    const auto geoId = sl.m_geometryId;
    const auto volumeId = geoId.volume();
    if (volumeId == 2) {  // pixel type detector
      singleHitSourceLinks.emplace_back(SourceLink{sl});
    } else if (volumeId == 3) {  // strip type detector

      const auto layerId = geoId.layer();

      if (layerId == 2 || layerId == 6) {
        frontSourceLinks.emplace_back(SourceLink{sl});
      } else if (layerId == 4 || layerId == 8) {
        backSourceLinks.emplace_back(SourceLink{sl});
      }
    }  // volume 3 (strip detector)
  }

  BOOST_CHECK_EQUAL(frontSourceLinks.size(), 2);
  BOOST_CHECK_EQUAL(backSourceLinks.size(), 2);

  Vector3 vertex = Vector3(-3_m, 0., 0.);

  auto spConstructor =
      [](const Vector3& pos, const std::optional<ActsScalar>& t,
         const Vector2& cov, const std::optional<ActsScalar>& covT,
         boost::container::static_vector<SourceLink, 2> slinks)
      -> TestSpacePoint {
    return TestSpacePoint(pos, t, cov[0], cov[1], covT, std::move(slinks));
  };

  auto spBuilderConfig = SpacePointBuilderConfig();
  spBuilderConfig.trackingGeometry = geometry;

  TestSourceLink::SurfaceAccessor surfaceAccessor{*geometry};
  spBuilderConfig.slSurfaceAccessor
      .connect<&TestSourceLink::SurfaceAccessor::operator()>(&surfaceAccessor);

  auto spBuilder =
      SpacePointBuilder<TestSpacePoint>(spBuilderConfig, spConstructor);

  // for cosmic  without vertex constraint, usePerpProj = true
  auto spBuilderConfig_perp = SpacePointBuilderConfig();
  spBuilderConfig_perp.trackingGeometry = geometry;
  spBuilderConfig_perp.slSurfaceAccessor
      .connect<&TestSourceLink::SurfaceAccessor::operator()>(&surfaceAccessor);

  spBuilderConfig_perp.usePerpProj = true;

  auto spBuilder_perp =
      SpacePointBuilder<TestSpacePoint>(spBuilderConfig_perp, spConstructor);

  TestSpacePointContainer spacePoints;
  TestSpacePointContainer spacePoints_extra;

  auto accessor = [](const SourceLink& slink) {
    auto testslink = slink.get<TestSourceLink>();
    BoundVector param;
    param.setZero();
    param[eBoundLoc0] = testslink.parameters[eBoundLoc0];
    param[eBoundLoc1] = testslink.parameters[eBoundLoc1];

    BoundSquareMatrix cov = BoundSquareMatrix::Zero();
    cov.topLeftCorner<2, 2>() = testslink.covariance;

    return std::make_pair(param, cov);
  };

  for (auto& sl : singleHitSourceLinks) {
    std::vector<SourceLink> slinks;
    slinks.emplace_back(sl);
    SpacePointBuilderOptions spOpt;
    spOpt.vertex = vertex;
    spOpt.paramCovAccessor = accessor;
    spBuilder.buildSpacePoint(geoCtx, slinks, spOpt,
                              std::back_inserter(spacePoints));
  }
  BOOST_CHECK_EQUAL(spacePoints.size(), 2);
  std::vector<std::pair<SourceLink, SourceLink>> slinkPairs;

  // strip SP building

  StripPairOptions pairOpt;
  pairOpt.paramCovAccessor = accessor;

  spBuilder.makeSourceLinkPairs(tgContext, frontSourceLinks, backSourceLinks,
                                slinkPairs, pairOpt);

  BOOST_CHECK_EQUAL(slinkPairs.size(), 2);

  for (auto& slinkPair : slinkPairs) {
    const std::pair<Vector3, Vector3> end1 =
        stripEnds(geometry, geoCtx, slinkPair.first);
    const std::pair<Vector3, Vector3> end2 =
        stripEnds(geometry, geoCtx, slinkPair.second);

    std::shared_ptr<const TestSpacePoint> spacePoint = nullptr;

    auto strippair = std::make_pair(end1, end2);
    std::vector<SourceLink> slinks;
    slinks.emplace_back(slinkPair.first);
    slinks.emplace_back(slinkPair.second);

    SpacePointBuilderOptions spOpt{strippair, accessor};

    // nominal strip sp building
    spBuilder.buildSpacePoint(geoCtx, slinks, spOpt,
                              std::back_inserter(spacePoints));

    // sp building without vertex constraint
    spBuilder_perp.buildSpacePoint(geoCtx, slinks, spOpt,
                                   std::back_inserter(spacePoints));

    // put measurements slightly outside strips to test recovery
    const std::pair<Vector3, Vector3> end3 =
        stripEnds(geometry, geoCtx, slinkPair.first, 1.01);
    const std::pair<Vector3, Vector3> end4 =
        stripEnds(geometry, geoCtx, slinkPair.second, 1.02);
    // the other side of the strips
    const std::pair<Vector3, Vector3> end5 =
        stripEnds(geometry, geoCtx, slinkPair.first, -0.01);
    const std::pair<Vector3, Vector3> end6 =
        stripEnds(geometry, geoCtx, slinkPair.second, -0.02);

    auto spBuilderConfig_badStrips = SpacePointBuilderConfig();

    spBuilderConfig_badStrips.trackingGeometry = geometry;
    spBuilderConfig_badStrips.slSurfaceAccessor
        .connect<&TestSourceLink::SurfaceAccessor::operator()>(
            &surfaceAccessor);

    auto spBuilder_badStrips = SpacePointBuilder<TestSpacePoint>(
        spBuilderConfig_badStrips, spConstructor);
    // sp building with the recovery method
    SpacePointBuilderOptions spOpt_badStrips1{std::make_pair(end3, end4),
                                              accessor};
    spOpt_badStrips1.vertex = vertex;
    spOpt_badStrips1.stripLengthTolerance = 0.0001;
    spOpt_badStrips1.stripLengthGapTolerance = 50.;
    spBuilder_badStrips.buildSpacePoint(geoCtx, slinks, spOpt_badStrips1,
                                        std::back_inserter(spacePoints_extra));

    SpacePointBuilderOptions spOpt_badStrips2{std::make_pair(end5, end6),
                                              accessor};
    spOpt_badStrips2.vertex = vertex;
    spOpt_badStrips2.stripLengthTolerance = 0.0001;
    spOpt_badStrips2.stripLengthGapTolerance = 50.;
    spBuilder_badStrips.buildSpacePoint(geoCtx, slinks, spOpt_badStrips2,
                                        std::back_inserter(spacePoints_extra));
  }

  for (auto& sp : spacePoints) {
    std::cout << "space point (" << sp.x() << " " << sp.y() << " " << sp.z()
              << ") var (r,z): " << sp.varianceR() << " " << sp.varianceZ()
              << std::endl;
  }
  std::cout << "space points produced with bad strips:" << std::endl;
  for (auto& sp : spacePoints_extra) {
    std::cout << "space point (" << sp.x() << " " << sp.y() << " " << sp.z()
              << ") var (r,z): " << sp.varianceR() << " " << sp.varianceZ()
              << std::endl;
  }

  BOOST_CHECK_EQUAL(spacePoints.size(), 6);
}

}  // namespace Acts::Test
