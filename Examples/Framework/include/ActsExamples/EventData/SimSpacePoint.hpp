// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Validation/ProtoTrackClassification.hpp"
namespace ActsExamples {
struct SimSpacePoint {
  // Hit id
  size_t m_index;
  // Global position
  float m_x;
  float m_y;
  float m_z;
  float m_r;
  // volume and layer ID
  Acts::GeometryIdentifier m_geoId;
  // VarianceR/Z of the SP position.
  float varianceR;
  float varianceZ;
  size_t index() const { return m_index; }
  float x() const { return m_x; }
  float y() const { return m_y; }
  float z() const { return m_z; }
  float r() const { return m_r; }
};

inline bool operator==(SimSpacePoint a, SimSpacePoint b) {
  return (a.m_index == b.m_index && a.m_geoId.volume() == b.m_geoId.volume() &&
          a.m_geoId.layer() == b.m_geoId.layer() && a.m_x == b.m_x &&
          a.m_y == b.m_y && a.m_z == b.m_z && a.varianceR == b.varianceR &&
          a.varianceZ == b.varianceZ);
}
}  // namespace ActsExamples
