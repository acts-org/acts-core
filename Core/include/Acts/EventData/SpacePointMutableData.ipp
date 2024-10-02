// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <iostream>

namespace Acts {

inline float SpacePointMutableData::quality(const std::size_t idx) const {
  assert(idx < m_quality.size());
  return m_quality[idx];
}

inline float SpacePointMutableData::deltaR(const std::size_t idx) const {
  assert(idx < m_deltaR.size());
  return m_deltaR[idx];
}

inline void SpacePointMutableData::setQuality(const std::size_t idx,
                                              const float value) {
  assert(idx < m_quality.size());
  if (value > m_quality[idx]) {
    m_quality[idx] = value;
  }
}

inline void SpacePointMutableData::setDeltaR(const std::size_t idx,
                                             const float value) {
  assert(idx < m_deltaR.size());
  m_deltaR[idx] = value;
}

inline void SpacePointMutableData::resize(const std::size_t n) {
  clear();
  m_quality.resize(n, -std::numeric_limits<float>::infinity());
  m_deltaR.resize(n, 0.f);
}

inline void SpacePointMutableData::clear() {
  m_quality.clear();
  m_deltaR.clear();
}

}  // namespace Acts
