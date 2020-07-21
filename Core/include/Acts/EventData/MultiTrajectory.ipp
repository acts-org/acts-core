// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <bitset>
#include <cstdint>
#include <type_traits>
#include <vector>

#include <Eigen/Core>

#include "Acts/Utilities/TypeTraits.hpp"

namespace Acts {
namespace detail_lt {
template <typename SL, size_t M, bool ReadOnly>
inline TrackStateProxy<SL, M, ReadOnly>::TrackStateProxy(
    ConstIf<MultiTrajectory<SL, M>, ReadOnly>& trajectory, size_t istate)
    : m_traj(&trajectory), m_istate(istate) {}

template <typename SL, size_t M, bool ReadOnly>
TrackStatePropMask TrackStateProxy<SL, M, ReadOnly>::getMask() const {
  using PM = TrackStatePropMask;
  PM mask = PM::None;
  if (hasBoundPredicted()) {
    mask |= PM::BoundPredicted;
  }
  if (hasBoundFiltered()) {
    mask |= PM::BoundFiltered;
  }
  if (hasBoundSmoothed()) {
    mask |= PM::BoundSmoothed;
  }
  if (hasFreePredicted()) {
    mask |= PM::FreePredicted;
  }
  if (hasFreeFiltered()) {
    mask |= PM::FreeFiltered;
  }
  if (hasFreeSmoothed()) {
    mask |= PM::FreeSmoothed;
  }
  if (hasJacobianBoundToBound()) {
    mask |= PM::JacobianBoundToBound;
  }
  if (hasJacobianBoundToFree()) {
    mask |= PM::JacobianBoundToFree;
  }
  if (hasJacobianFreeToBound()) {
    mask |= PM::JacobianFreeToBound;
  }
  if (hasJacobianFreeToFree()) {
    mask |= PM::JacobianFreeToFree;
  }
  if (hasUncalibrated()) {
    mask |= PM::Uncalibrated;
  }
  if (hasCalibrated()) {
    mask |= PM::Calibrated;
  }

  return mask;
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::parameters() const -> Parameters {
  IndexData::IndexType idx;
  if (hasBoundSmoothed()) {
    idx = data().iboundsmoothed;
  } else if (hasBoundFiltered()) {
    idx = data().iboundfiltered;
  } else {
    idx = data().ipredicted;
  }

  return Parameters(m_traj->m_boundParams.data.col(idx).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::covariance() const -> Covariance {
  IndexData::IndexType idx;
  if (hasBoundSmoothed()) {
    idx = data().iboundsmoothed;
  } else if (hasBoundFiltered()) {
    idx = data().iboundfiltered;
  } else {
    idx = data().ipredicted;
  }
  return Covariance(m_traj->m_boundCov.data.col(idx).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::boundPredicted() const
    -> Parameters {
  assert(data().iboundpredicted != IndexData::kInvalid);
  return Parameters(m_traj->m_boundParams.col(data().iboundpredicted).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::boundPredictedCovariance() const
    -> Covariance {
  assert(data().iboundpredicted != IndexData::kInvalid);
  return Covariance(m_traj->m_boundCov.col(data().iboundpredicted).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::boundFiltered() const
    -> Parameters {
  assert(data().iboundfiltered != IndexData::kInvalid);
  return Parameters(m_traj->m_boundParams.col(data().iboundfiltered).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::boundFilteredCovariance() const
    -> Covariance {
  assert(data().iboundfiltered != IndexData::kInvalid);
  return Covariance(m_traj->m_boundCov.col(data().iboundfiltered).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::boundSmoothed() const
    -> Parameters {
  assert(data().iboundsmoothed != IndexData::kInvalid);
  return Parameters(m_traj->m_boundParams.col(data().iboundsmoothed).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::boundSmoothedCovariance() const
    -> Covariance {
  assert(data().iboundsmoothed != IndexData::kInvalid);
  return Covariance(m_traj->m_boundCov.col(data().iboundsmoothed).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::freePredicted() const
    -> FreeParameters {
  assert(data().ifreepredicted != IndexData::kInvalid);
  return FreeParameters(m_traj->m_freeParams.col(data().ifreepredicted).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::freePredictedCovariance() const
    -> FreeCovariance {
  assert(data().ifreepredicted != IndexData::kInvalid);
  return FreeCovariance(m_traj->m_freeCov.col(data().ifreepredicted).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::freeFiltered() const
    -> FreeParameters {
  assert(data().ifreefiltered != IndexData::kInvalid);
  return FreeParameters(m_traj->m_freeParams.col(data().ifreefiltered).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::freeFilteredCovariance() const
    -> FreeCovariance {
  assert(data().ifreefiltered != IndexData::kInvalid);
  return FreeCovariance(m_traj->m_freeCov.col(data().ifreefiltered).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::freeSmoothed() const
    -> FreeParameters {
  assert(data().ifreesmoothed != IndexData::kInvalid);
  return FreeParameters(m_traj->m_freeParams.col(data().ifreesmoothed).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::freeSmoothedCovariance() const
    -> FreeCovariance {
  assert(data().ifreesmoothed != IndexData::kInvalid);
  return FreeCovariance(m_traj->m_freeCov.col(data().ifreesmoothed).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::jacobianBoundToBound() const
    -> JacobianBoundToBound {
  assert(data().ijacobianboundtobound != IndexData::kInvalid);
  return JacobianBoundToBound(m_traj->m_jacBoundToBound.col(data().ijacobianboundtobound).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::jacobianBoundToFree() const
    -> JacobianBoundToFree {
  assert(data().ijacobianboundtofree != IndexData::kInvalid);
  return JacobianBoundToFree(m_traj->m_jacBoundToFree.col(data().ijacobianboundtofree).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::jacobianFreeToBound() const
    -> JacobianFreeToBound {
  assert(data().ijacobianfreetobound != IndexData::kInvalid);
  return JacobianFreeToBound(m_traj->m_jacFreeToBound.col(data().ijacobianfreetobound).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::jacobianFreeToFree() const
    -> JacobianFreeToFree {
  assert(data().ijacobianfreetofree != IndexData::kInvalid);
  return JacobianFreeToFree(m_traj->m_jacFreeToFree.col(data().ijacobianfreetofree).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::projector() const -> Projector {
  assert(data().iprojector != IndexData::kInvalid);
  return bitsetToMatrix<Projector>(m_traj->m_projectors[data().iprojector]);
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::uncalibrated() const
    -> const SourceLink& {
  assert(data().iuncalibrated != IndexData::kInvalid);
  return m_traj->m_sourceLinks[data().iuncalibrated];
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::calibrated() const
    -> Measurement {
  assert(data().icalibrated != IndexData::kInvalid);
  return Measurement(m_traj->m_meas.col(data().icalibrated).data());
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::calibratedSourceLink() const
    -> const SourceLink& {
  assert(data().icalibratedsourcelink != IndexData::kInvalid);
  return m_traj->m_sourceLinks[data().icalibratedsourcelink];
}

template <typename SL, size_t M, bool ReadOnly>
inline auto TrackStateProxy<SL, M, ReadOnly>::calibratedCovariance() const
    -> MeasurementCovariance {
  assert(data().icalibrated != IndexData::kInvalid);
  return MeasurementCovariance(
      m_traj->m_measCov.col(data().icalibrated).data());
}

}  // namespace detail_lt

template <typename SL, size_t MSM>
inline size_t MultiTrajectory<SL, MSM>::addTrackState(TrackStatePropMask mask,
                                                 size_t iprevious) {
  using PropMask = TrackStatePropMask;

  m_index.emplace_back();
  detail_lt::IndexData& p = m_index.back();
  size_t index = m_index.size() - 1;

  if (iprevious != SIZE_MAX) {
    p.iprevious = static_cast<uint16_t>(iprevious);
  }

  // always set, but can be null
  m_referenceSurfaces.emplace_back(nullptr);
  p.irefsurface = m_referenceSurfaces.size() - 1;

  if (ACTS_CHECK_BIT(mask, PropMask::BoundPredicted)) {
    m_boundParams.addCol();
    m_boundCov.addCol();
    p.iboundpredicted = m_boundParams.size() - 1;
  }

  if (ACTS_CHECK_BIT(mask, PropMask::BoundFiltered)) {
    m_boundParams.addCol();
    m_boundCov.addCol();
    p.iboundfiltered = m_boundParams.size() - 1;
  }

  if (ACTS_CHECK_BIT(mask, PropMask::BoundSmoothed)) {
    m_boundParams.addCol();
    m_boundCov.addCol();
    p.iboundsmoothed = m_boundParams.size() - 1;
  }

  if (ACTS_CHECK_BIT(mask, PropMask::FreePredicted)) {
    m_freeParams.addCol();
    m_freeCov.addCol();
    p.ifreepredicted = m_freeParams.size() - 1;
  }

  if (ACTS_CHECK_BIT(mask, PropMask::FreeFiltered)) {
    m_freeParams.addCol();
    m_freeCov.addCol();
    p.ifreefiltered = m_freeParams.size() - 1;
  }

  if (ACTS_CHECK_BIT(mask, PropMask::FreeSmoothed)) {
    m_freeParams.addCol();
    m_freeCov.addCol();
    p.ifreesmoothed = m_freeParams.size() - 1;
  }
  
  if (ACTS_CHECK_BIT(mask, PropMask::JacobianBoundToBound)) {
    m_jacBoundToBound.addCol();
    p.ijacobianboundtobound = m_jacBoundToBound.size() - 1;
  }
  
  if (ACTS_CHECK_BIT(mask, PropMask::JacobianBoundToFree)) {
    m_jacBoundToFree.addCol();
    p.ijacobianboundtofree = m_jacBoundToFree.size() - 1;
  }
  
  if (ACTS_CHECK_BIT(mask, PropMask::JacobianFreeToBound)) {
    m_jacFreeToBound.addCol();
    p.ijacobianfreetobound = m_jacFreeToBound.size() - 1;
  }
  
  if (ACTS_CHECK_BIT(mask, PropMask::JacobianFreeToFree)) {
    m_jacFreeToFree.addCol();
    p.ijacobianfreetofree = m_jacFreeToFree.size() - 1;
  }
  
  if (ACTS_CHECK_BIT(mask, PropMask::Uncalibrated)) {
    m_sourceLinks.emplace_back();
    p.iuncalibrated = m_sourceLinks.size() - 1;
  }

  if (ACTS_CHECK_BIT(mask, PropMask::Calibrated)) {
    m_meas.addCol();
    m_measCov.addCol();
    p.icalibrated = m_meas.size() - 1;

    m_sourceLinks.emplace_back();
    p.icalibratedsourcelink = m_sourceLinks.size() - 1;

    m_projectors.emplace_back();
    p.iprojector = m_projectors.size() - 1;
  }

  return index;
}

template <typename SL, size_t MSM>
template <typename F>
void MultiTrajectory<SL, MSM>::visitBackwards(size_t iendpoint, F&& callable) const {
  static_assert(detail_lt::VisitorConcept<F, ConstTrackStateProxy>,
                "Callable needs to satisfy VisitorConcept");

  while (true) {
    if constexpr (std::is_same_v<std::invoke_result_t<F, ConstTrackStateProxy>,
                                 bool>) {
      bool proceed = callable(getTrackState(iendpoint));
      // this point has no parent and ends the trajectory, or a break was
      // requested
      if (m_index[iendpoint].iprevious == detail_lt::IndexData::kInvalid ||
          !proceed) {
        break;
      }
    } else {
      callable(getTrackState(iendpoint));
      // this point has no parent and ends the trajectory
      if (m_index[iendpoint].iprevious == detail_lt::IndexData::kInvalid) {
        break;
      }
    }
    iendpoint = m_index[iendpoint].iprevious;
  }
}

template <typename SL, size_t MSM>
template <typename F>
void MultiTrajectory<SL, MSM>::applyBackwards(size_t iendpoint, F&& callable) {
  static_assert(detail_lt::VisitorConcept<F, TrackStateProxy>,
                "Callable needs to satisfy VisitorConcept");

  while (true) {
    if constexpr (std::is_same_v<std::invoke_result_t<F, TrackStateProxy>,
                                 bool>) {
      bool proceed = callable(getTrackState(iendpoint));
      // this point has no parent and ends the trajectory, or a break was
      // requested
      if (m_index[iendpoint].iprevious == detail_lt::IndexData::kInvalid ||
          !proceed) {
        break;
      }
    } else {
      callable(getTrackState(iendpoint));
      // this point has no parent and ends the trajectory
      if (m_index[iendpoint].iprevious == detail_lt::IndexData::kInvalid) {
        break;
      }
    }
    iendpoint = m_index[iendpoint].iprevious;
  }
}
}  // namespace Acts
