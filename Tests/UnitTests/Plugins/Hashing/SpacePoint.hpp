// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cmath>
#include <optional>

struct SpacePoint {
  // Member variables
  float m_x{};
  float m_y{};
  float m_z{};
  float m_r{};
  int layer{};
  float varianceR{};
  float varianceZ{};
  std::optional<float> m_t;
  std::optional<float> varianceT;

  // Default constructor
  SpacePoint() = default;

  // Constructor with parameters
  SpacePoint(float x, float y, float z, float r, int l, float varR, float varZ,
             std::optional<float> t, std::optional<float> varT)
      : m_x(x),
        m_y(y),
        m_z(z),
        m_r(r),
        layer(l),
        varianceR(varR),
        varianceZ(varZ),
        m_t(t),
        varianceT(varT) {}

  // Member functions
  float x() const { return m_x; }
  float y() const { return m_y; }
  float z() const { return m_z; }
  float r() const { return m_r; }
  std::optional<float> t() const { return m_t; }

  // Equality operator
  friend bool operator==(const SpacePoint &a, const SpacePoint &b) {
    return std::abs(a.m_x - b.m_x) < 1e-6 && std::abs(a.m_y - b.m_y) < 1e-6 &&
           std::abs(a.m_z - b.m_z) < 1e-6;
  }
};
