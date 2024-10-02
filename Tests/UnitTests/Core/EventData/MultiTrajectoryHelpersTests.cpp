// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/EventData/MultiTrajectoryHelpers.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(MultiTrajectoryHelpers)

BOOST_AUTO_TEST_CASE(trajectoryState) {
  auto surface = Surface::makeShared<PerigeeSurface>(Vector3(0, 0, 0));

  VectorMultiTrajectory traj;

  auto ts = traj.makeTrackState(TrackStatePropMask::None);
  ts.typeFlags().set(MeasurementFlag);
  ts.setReferenceSurface(surface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(OutlierFlag);
  ts.setReferenceSurface(surface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(MeasurementFlag);
  ts.typeFlags().set(SharedHitFlag);
  ts.setReferenceSurface(surface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(HoleFlag);
  ts.setReferenceSurface(surface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(OutlierFlag);
  ts.setReferenceSurface(surface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(HoleFlag);
  ts.setReferenceSurface(surface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(MeasurementFlag);
  ts.typeFlags().set(SharedHitFlag);
  ts.setReferenceSurface(surface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(OutlierFlag);
  ts.setReferenceSurface(surface);

  auto state = Acts::MultiTrajectoryHelpers::trajectoryState(traj, ts.index());
  BOOST_CHECK_EQUAL(state.nHoles, 2);
  BOOST_CHECK_EQUAL(state.nMeasurements, 3);
  BOOST_CHECK_EQUAL(state.nOutliers, 3);
  BOOST_CHECK_EQUAL(state.nSharedHits, 2);
}

BOOST_AUTO_TEST_CASE(trajectoryStateVolume) {
  GeometryIdentifier::Value searchVolume = 1;
  GeometryIdentifier::Value otherVolume = 2;
  std::vector<GeometryIdentifier::Value> volumes = {searchVolume};

  auto searchSurface = Surface::makeShared<PerigeeSurface>(Vector3(0, 0, 0));
  searchSurface->assignGeometryId(GeometryIdentifier().setVolume(searchVolume));
  auto otherSurface = Surface::makeShared<PerigeeSurface>(Vector3(0, 0, 0));
  otherSurface->assignGeometryId(GeometryIdentifier().setVolume(otherVolume));

  VectorMultiTrajectory traj;

  auto ts = traj.makeTrackState(TrackStatePropMask::None);
  ts.typeFlags().set(MeasurementFlag);
  ts.setReferenceSurface(searchSurface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(OutlierFlag);
  ts.setReferenceSurface(searchSurface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(MeasurementFlag);
  ts.typeFlags().set(SharedHitFlag);
  ts.setReferenceSurface(searchSurface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(HoleFlag);
  ts.setReferenceSurface(searchSurface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(OutlierFlag);
  ts.setReferenceSurface(searchSurface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(HoleFlag);
  ts.setReferenceSurface(searchSurface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(MeasurementFlag);
  ts.typeFlags().set(SharedHitFlag);
  ts.setReferenceSurface(otherSurface);

  ts = traj.makeTrackState(TrackStatePropMask::None, ts.index());
  ts.typeFlags().set(OutlierFlag);
  ts.setReferenceSurface(otherSurface);

  auto state =
      Acts::MultiTrajectoryHelpers::trajectoryState(traj, ts.index(), volumes);
  BOOST_CHECK_EQUAL(state.at(searchVolume).nHoles, 2);
  BOOST_CHECK_EQUAL(state.at(searchVolume).nMeasurements, 2);
  BOOST_CHECK_EQUAL(state.at(searchVolume).nOutliers, 2);
  BOOST_CHECK_EQUAL(state.at(searchVolume).nSharedHits, 1);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
