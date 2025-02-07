// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/ProxyAccessor.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/Tests/CommonHelpers/NonCompileTestHelpers.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"

using namespace Acts;

ACTS_DOES_NOT_COMPILE_SUITE_BEGIN(BuildFromConstRef)

{
  VectorTrackContainer mutVtc;
  VectorMultiTrajectory mutMtj;

  TrackContainer mutTc{mutVtc, mutMtj};
  static_assert(!mutTc.ReadOnly, "Unexpectedly read only");

  auto t = mutTc.makeTrack();
  t.appendTrackState();
  t.appendTrackState();
  t.appendTrackState();
  t = mutTc.makeTrack();
  t.appendTrackState();

  ConstVectorTrackContainer vtc{std::move(mutVtc)};
  ConstVectorMultiTrajectory mtj{std::move(mutMtj)};

  TrackContainer ctc{vtc, mtj};
  ACTS_DOES_NOT_COMPILE_BEGIN(AddTrackToConstTrackContainer)
  ctc.addTrack();
  ACTS_DOES_NOT_COMPILE_END()
}

{  // const correctness
  VectorTrackContainer vtc;
  VectorMultiTrajectory mtj;
  {
    TrackContainer tc{vtc, mtj};

    ACTS_DOES_NOT_COMPILE_BEGIN(TrackMutateConstProxyRef)
    for (const auto track : tc) {
      track.parameters().setRandom();
    }
    ACTS_DOES_NOT_COMPILE_END()
  }

  ConstVectorTrackContainer cvtc{std::move(vtc)};
  ConstVectorMultiTrajectory cmtj{std::move(mtj)};
  {
    TrackContainer tc{cvtc, cmtj};

    ACTS_DOES_NOT_COMPILE_BEGIN(TrackMutateConstProxy)
    for (auto track : tc) {
      track.parameters().setRandom();
    }
    ACTS_DOES_NOT_COMPILE_END()
  }
}

{
  VectorTrackContainer vtc;
  VectorMultiTrajectory mtj;
  TrackContainer tc{vtc, mtj};
  auto t = tc.makeTrack();
  (void)t;

  ConstProxyAccessor<unsigned int> caccNMeasuements("nMeasurements");
  static_cast<void>(caccNMeasuements);  // suppressed unused warning
  ACTS_DOES_NOT_COMPILE_BEGIN(ConstAccessorMutate)
  caccNMeasuements(t) = 66;
  ACTS_DOES_NOT_COMPILE_END()

  ACTS_DOES_NOT_COMPILE_BEGIN(MutationThroughContainerConstRef)
  const auto& ctc = tc;
  ctc.getTrack(idx).covariance().setRandom();
  ACTS_DOES_NOT_COMPILE_END()

  ACTS_DOES_NOT_COMPILE_BEGIN(MutationThroughProxyConstRef)
  const auto& ctp = t;
  ctp.covariance().setRandom();
  ACTS_DOES_NOT_COMPILE_END()
}

ACTS_DOES_NOT_COMPILE_SUITE_END()
