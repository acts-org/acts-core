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
SpacePointProxyIterator<container_t>::SpacePointProxyIterator(
    const container_t& container, std::size_t index)
    : m_container(&container), m_index(index) {}

template <typename container_t>
SpacePointProxyIterator<container_t>&
SpacePointProxyIterator<container_t>::operator++() {
  ++m_index;
  return *this;
}

template <typename container_t>
SpacePointProxyIterator<container_t>&
SpacePointProxyIterator<container_t>::operator--() {
  --m_index;
  return *this;
}

template <typename container_t>
SpacePointProxyIterator<container_t>
SpacePointProxyIterator<container_t>::operator++(int) {
  SpacePointProxyIterator other(*this);
  ++m_index;
  return other;
}

template <typename container_t>
SpacePointProxyIterator<container_t>
SpacePointProxyIterator<container_t>::operator--(int) {
  SpacePointProxyIterator other(*this);
  --m_index;
  return other;
}

template <typename container_t>
bool SpacePointProxyIterator<container_t>::operator==(
    const SpacePointProxyIterator<container_t>& other) const {
  return m_container == other.m_container && m_index == other.m_index;
}

template <typename container_t>
auto SpacePointProxyIterator<container_t>::operator<=>(
    const SpacePointProxyIterator<container_t>& other) const {
  return m_index <=> other.m_index;
}

template <typename container_t>
SpacePointProxyIterator<container_t>&
SpacePointProxyIterator<container_t>::operator+=(const std::size_t offset) {
  m_index += offset;
  return *this;
}

template <typename container_t>
SpacePointProxyIterator<container_t>&
SpacePointProxyIterator<container_t>::operator-=(const std::size_t offset) {
  m_index -= offset;
  return *this;
}

template <typename container_t>
SpacePointProxyIterator<container_t>
SpacePointProxyIterator<container_t>::operator+(
    const std::size_t offset) const {
  return SpacePointProxyIterator(*m_container, m_index + offset);
}

template <typename container_t>
SpacePointProxyIterator<container_t>
SpacePointProxyIterator<container_t>::operator-(
    const std::size_t offset) const {
  return SpacePointProxyIterator(*m_container, m_index - offset);
}

template <typename container_t>
typename SpacePointProxyIterator<container_t>::difference_type
SpacePointProxyIterator<container_t>::operator-(
    const SpacePointProxyIterator<container_t>& other) const {
  return m_index - other.m_index;
}

template <typename container_t>
const typename SpacePointProxyIterator<container_t>::value_type&
SpacePointProxyIterator<container_t>::operator*() const {
  return m_container->proxy(m_index);
}

}  // namespace Acts
