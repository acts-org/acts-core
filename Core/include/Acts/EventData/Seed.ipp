// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

namespace Acts {

template <typename external_spacepoint_t, std::size_t N>
  requires(N >= 3ul)
template <typename... args_t>
  requires(sizeof...(args_t) == N) &&
          (std::same_as<external_spacepoint_t, args_t> && ...)
Seed<external_spacepoint_t, N>::Seed(const args_t&... points)
    : m_spacepoints({&points...}) {}

template <typename external_spacepoint_t, std::size_t N>
  requires(N >= 3ul)
void Seed<external_spacepoint_t, N>::setVertexZ(float vertex) {
  m_vertexZ = vertex;
}

template <typename external_spacepoint_t, std::size_t N>
  requires(N >= 3ul)
void Seed<external_spacepoint_t, N>::setQuality(float seedQuality) {
  m_seedQuality = seedQuality;
}

template <typename external_spacepoint_t, std::size_t N>
  requires(N >= 3ul)
const std::array<const external_spacepoint_t*, N>&
Seed<external_spacepoint_t, N>::sp() const {
  return m_spacepoints;
}

template <typename external_spacepoint_t, std::size_t N>
  requires(N >= 3ul)
float Seed<external_spacepoint_t, N>::z() const {
  return m_vertexZ;
}

template <typename external_spacepoint_t, std::size_t N>
  requires(N >= 3ul)
float Seed<external_spacepoint_t, N>::seedQuality() const {
  return m_seedQuality;
}

}  // namespace Acts
