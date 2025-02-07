// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

namespace Acts {

template <typename D, std::size_t M, bool ReadOnly>
inline TrackStateProxy<D, M, ReadOnly>::TrackStateProxy(
    detail_lt::ConstIf<MultiTrajectory<D>, ReadOnly>& trajectory,
    IndexType istate)
    : m_traj(&trajectory), m_istate(istate) {}

template <typename D, std::size_t M, bool ReadOnly>
TrackStatePropMask TrackStateProxy<D, M, ReadOnly>::getMask() const {
  using PM = TrackStatePropMask;

  PM mask = PM::None;
  if (hasPredicted()) {
    mask |= PM::Predicted;
  }
  if (hasFiltered()) {
    mask |= PM::Filtered;
  }
  if (hasSmoothed()) {
    mask |= PM::Smoothed;
  }
  if (hasJacobian()) {
    mask |= PM::Jacobian;
  }
  if (hasCalibrated()) {
    mask |= PM::Calibrated;
  }
  return mask;
}

template <typename D, std::size_t M, bool ReadOnly>
inline auto TrackStateProxy<D, M, ReadOnly>::parameters() const
    -> ConstParameters {
  if (hasSmoothed()) {
    return smoothed();
  } else if (hasFiltered()) {
    return filtered();
  } else {
    return predicted();
  }
}

template <typename D, std::size_t M, bool ReadOnly>
inline auto TrackStateProxy<D, M, ReadOnly>::covariance() const
    -> ConstCovariance {
  if (hasSmoothed()) {
    return smoothedCovariance();
  } else if (hasFiltered()) {
    return filteredCovariance();
  } else {
    return predictedCovariance();
  }
}

template <typename D, std::size_t M, bool ReadOnly>
inline auto TrackStateProxy<D, M, ReadOnly>::getUncalibratedSourceLink() const
    -> SourceLink {
  assert(has<hashString("uncalibratedSourceLink")>());
  return m_traj->getUncalibratedSourceLink(m_istate);
}

}  // namespace Acts
