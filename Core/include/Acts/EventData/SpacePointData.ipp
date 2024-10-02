// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <iostream>

namespace Acts {

inline float SpacePointData::x(const std::size_t idx) const {
  assert(idx < m_x.size());
  return m_x[idx];
}

inline float SpacePointData::y(const std::size_t idx) const {
  assert(idx < m_y.size());
  return m_y[idx];
}

inline float SpacePointData::z(const std::size_t idx) const {
  assert(idx < m_z.size());
  return m_z[idx];
}

inline float SpacePointData::radius(const std::size_t idx) const {
  assert(idx < m_radius.size());
  return m_radius[idx];
}

inline float SpacePointData::phi(const std::size_t idx) const {
  assert(idx < m_phi.size());
  return m_phi[idx];
}

inline float SpacePointData::varianceZ(const std::size_t idx) const {
  assert(idx < m_varianceZ.size());
  return m_varianceZ[idx];
}

inline float SpacePointData::varianceR(const std::size_t idx) const {
  assert(idx < m_varianceR.size());
  return m_varianceR[idx];
}

inline void SpacePointData::setX(const std::size_t idx, const float value) {
  assert(idx < m_x.size());
  m_x[idx] = value;
}

inline void SpacePointData::setY(const std::size_t idx, const float value) {
  assert(idx < m_y.size());
  m_y[idx] = value;
}

inline void SpacePointData::setZ(const std::size_t idx, const float value) {
  assert(idx < m_z.size());
  m_z[idx] = value;
}

inline void SpacePointData::setRadius(const std::size_t idx,
                                      const float value) {
  assert(idx < m_radius.size());
  m_radius[idx] = value;
}

inline void SpacePointData::setPhi(const std::size_t idx, const float value) {
  assert(idx < m_phi.size());
  m_phi[idx] = value;
}

inline void SpacePointData::setVarianceZ(const std::size_t idx,
                                         const float value) {
  assert(idx < m_varianceZ.size());
  m_varianceZ[idx] = value;
}

inline void SpacePointData::setVarianceR(const std::size_t idx,
                                         const float value) {
  assert(idx < m_varianceR.size());
  m_varianceR[idx] = value;
}

inline bool SpacePointData::hasDynamicVariable() const {
  return !m_topStripVector.empty();
}

inline const Acts::Vector3& SpacePointData::topStripVector(
    const std::size_t idx) const {
  assert(idx < m_topStripVector.size());
  return m_topStripVector[idx];
}

inline const Acts::Vector3& SpacePointData::bottomStripVector(
    const std::size_t idx) const {
  assert(idx < m_bottomStripVector.size());
  return m_bottomStripVector[idx];
}

inline const Acts::Vector3& SpacePointData::stripCenterDistance(
    const std::size_t idx) const {
  assert(idx < m_stripCenterDistance.size());
  return m_stripCenterDistance[idx];
}

inline const Acts::Vector3& SpacePointData::topStripCenterPosition(
    const std::size_t idx) const {
  assert(idx < m_topStripCenterPosition.size());
  return m_topStripCenterPosition[idx];
}

inline void SpacePointData::setTopStripVector(const std::size_t idx,
                                              const Acts::Vector3& value) {
  assert(idx < m_topStripVector.size());
  m_topStripVector[idx] = value;
}

inline void SpacePointData::setBottomStripVector(const std::size_t idx,
                                                 const Acts::Vector3& value) {
  assert(idx < m_bottomStripVector.size());
  m_bottomStripVector[idx] = value;
}

inline void SpacePointData::setStripCenterDistance(const std::size_t idx,
                                                   const Acts::Vector3& value) {
  assert(idx < m_stripCenterDistance.size());
  m_stripCenterDistance[idx] = value;
}

inline void SpacePointData::setTopStripCenterPosition(
    const std::size_t idx, const Acts::Vector3& value) {
  assert(idx < m_topStripCenterPosition.size());
  m_topStripCenterPosition[idx] = value;
}

inline void SpacePointData::resize(const std::size_t n, bool resizeDynamic) {
  clear();

  m_x.resize(n, 0.f);
  m_y.resize(n, 0.f);
  m_z.resize(n, 0.f);
  m_radius.resize(n, 0.f);
  m_phi.resize(n, 0.f);
  m_varianceZ.resize(n, 0.f);
  m_varianceR.resize(n, 0.f);

  if (resizeDynamic) {
    m_topStripVector.resize(n, {0, 0, 0});
    m_bottomStripVector.resize(n, {0, 0, 0});
    m_stripCenterDistance.resize(n, {0, 0, 0});
    m_topStripCenterPosition.resize(n, {0, 0, 0});
  }
}

inline void SpacePointData::clear() {
  m_x.clear();
  m_y.clear();
  m_z.clear();
  m_radius.clear();
  m_phi.clear();
  m_varianceZ.clear();
  m_varianceR.clear();
  // dynamic variables
  m_topStripVector.clear();
  m_bottomStripVector.clear();
  m_stripCenterDistance.clear();
  m_topStripCenterPosition.clear();
}

}  // namespace Acts
