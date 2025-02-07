// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "Acts/Vertexing/Vertex.hpp"

#include <map>

namespace Acts {

/// @brief Helper struct for storing vertex related information
struct VertexInfo {
  VertexInfo() = default;

  VertexInfo(const Acts::Vertex& constr, const Acts::Vector4& pos)
      : constraint(constr),
        linPoint(pos),
        oldPosition(pos),
        seedPosition(pos) {}

  // Vertex constraint
  Acts::Vertex constraint;

  // Point where all associated tracks are linearized
  Acts::Vector4 linPoint{Acts::Vector4::Zero()};

  // Vertex position from the last iteration of the fit
  Acts::Vector4 oldPosition{Acts::Vector4::Zero()};

  // The seed position (i.e., the first estimate for the vertex position as
  // obtained by the vertex seed finder)
  Acts::Vector4 seedPosition{Acts::Vector4::Zero()};

  // If set to true, the associated tracks need to be relinearized at a more
  // recent vertex position
  bool relinearize = true;

  // Vector of all tracks that are currently assigned to vertex
  std::vector<InputTrack> trackLinks;

  std::map<InputTrack, const BoundTrackParameters> impactParams3D;
};

}  // namespace Acts
