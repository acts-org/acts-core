// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cmath>
#include <optional>

#pragma once

struct SpacePoint {
  float m_x{};
  float m_y{};
  float m_z{};
  float m_r{};
  int layer{};
  float varianceR{};
  float varianceZ{};
  std::optional<float> m_t;
  std::optional<float> varianceT;
  float x() const { return m_x; }
  float y() const { return m_y; }
  float z() const { return m_z; }
  float r() const { return m_r; }
  std::optional<float> t() const { return m_t; }
};

bool operator==(SpacePoint a, SpacePoint b) {
  if (std::abs(a.m_x - b.m_x) < 1e-6 && std::abs(a.m_y - b.m_y) < 1e-6 &&
      std::abs(a.m_z - b.m_z) < 1e-6) {
    return true;
  } else {
    return false;
  }
}
