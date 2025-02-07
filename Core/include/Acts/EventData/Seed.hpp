// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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

  void setVertexZ(float vertex);
  void setQuality(float seedQuality);

  const std::array<const external_spacepoint_t*, N>& sp() const;
  float z() const;
  float seedQuality() const;

 private:
  std::array<const external_spacepoint_t*, N> m_spacepoints{};
  float m_vertexZ{0.f};
  float m_seedQuality{-std::numeric_limits<float>::infinity()};
};

}  // namespace Acts

#include "Acts/EventData/Seed.ipp"
