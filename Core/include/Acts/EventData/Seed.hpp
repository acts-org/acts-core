// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <array>
#include <limits>

namespace Acts {

template <typename external_spacepoint_t, std::size_t N = 3ul>
class Seed {
  static_assert(N >= 3ul);

 public:
  using value_type = external_spacepoint_t;
  static constexpr std::size_t DIM = N;

  template <typename... args_t>
    requires(sizeof...(args_t) == N) &&
            (std::same_as<external_spacepoint_t, args_t> && ...)
  explicit Seed(const args_t&... points);

  void setVertexZ(float vertex) noexcept;
  void setQuality(float seedQuality) noexcept;

  const std::array<const external_spacepoint_t*, N>& sp() const noexcept;
  float z() const noexcept;
  float seedQuality() const noexcept;

 private:
  std::array<const external_spacepoint_t*, N> m_spacepoints{};
  float m_vertexZ{0.f};
  float m_seedQuality{-std::numeric_limits<float>::infinity()};
};

}  // namespace Acts

#include "Acts/EventData/Seed.ipp"
