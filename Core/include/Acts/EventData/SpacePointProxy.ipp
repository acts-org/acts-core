// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

namespace Acts {

// Implementation
template <typename container_t>
SpacePointProxy<container_t>::SpacePointProxy(const container_t& container,
                                              std::size_t index)
    : m_container(&container), m_index(index) {}

template <typename container_t>
const typename SpacePointProxy<container_t>::ValueType&
SpacePointProxy<container_t>::externalSpacePoint() const {
  return container().sp(m_index);
}

template <typename container_t>
std::size_t SpacePointProxy<container_t>::index() const {
  return m_index;
}

template <typename container_t>
float SpacePointProxy<container_t>::x() const {
  return container().x(m_index);
}

template <typename container_t>
float SpacePointProxy<container_t>::y() const {
  return container().y(m_index);
}

template <typename container_t>
float SpacePointProxy<container_t>::z() const {
  return container().z(m_index);
}

template <typename container_t>
float SpacePointProxy<container_t>::phi() const {
  return container().phi(m_index);
}

template <typename container_t>
float SpacePointProxy<container_t>::radius() const {
  return container().radius(m_index);
}

template <typename container_t>
float SpacePointProxy<container_t>::varianceR() const {
  return container().varianceR(m_index);
}

template <typename container_t>
float SpacePointProxy<container_t>::varianceZ() const {
  return container().varianceZ(m_index);
}

template <typename container_t>
const Acts::Vector3& SpacePointProxy<container_t>::topStripVector() const {
  return container().topStripVector(m_index);
}

template <typename container_t>
const Acts::Vector3& SpacePointProxy<container_t>::bottomStripVector() const {
  return container().bottomStripVector(m_index);
}

template <typename container_t>
const Acts::Vector3& SpacePointProxy<container_t>::stripCenterDistance() const {
  return container().stripCenterDistance(m_index);
}

template <typename container_t>
const Acts::Vector3& SpacePointProxy<container_t>::topStripCenterPosition()
    const {
  return container().topStripCenterPosition(m_index);
}

template <typename container_t>
const container_t& SpacePointProxy<container_t>::container() const {
  return *m_container;
}

}  // namespace Acts
