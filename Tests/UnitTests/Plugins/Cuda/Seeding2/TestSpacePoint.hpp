// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// System include(s).
#include <iosfwd>

/// Simple spacepoint implementation for the test
struct TestSpacePoint {
  float m_x;
  float m_y;
  float m_z;
  float m_r;
  int m_surface;
  float m_varianceR;
  float m_varianceZ;
  float x() const { return m_x; }
  float y() const { return m_y; }
  float z() const { return m_z; }
  float r() const { return m_r; }
};

/// Helper operator for comparing the test spacepoints
bool operator==(const TestSpacePoint& a, const TestSpacePoint& b);

/// Output / print operator for @c TestSpacePoint
std::ostream& operator<<(std::ostream& out, const TestSpacePoint& sp);
