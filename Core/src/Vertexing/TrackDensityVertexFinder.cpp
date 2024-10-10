// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/TrackDensityVertexFinder.hpp"

Acts::Result<std::vector<Acts::Vertex>> Acts::TrackDensityVertexFinder::find(
    const std::vector<InputTrack>& trackVector,
    const VertexingOptions& vertexingOptions,
    IVertexFinder::State& /*state*/) const {
  GaussianTrackDensity::State densityState(trackVector.size());

  // Calculate z seed position
  auto zAndWidthRes = m_cfg.trackDensityEstimator.globalMaximumWithWidth(
      densityState, trackVector);
  if (!zAndWidthRes.ok()) {
    return zAndWidthRes.error();
  }
  const auto& zAndWidthOpt = *zAndWidthRes;
  if (!zAndWidthOpt) {
    return std::vector<Vertex>();
  }
  const auto& zAndWidth = *zAndWidthOpt;

  double z = zAndWidth.first;

  // Calculate seed position
  // Note: constraint position is (0,0,0) if no constraint provided
  Vector4 seedPos =
      vertexingOptions.constraint.fullPosition() + Vector4(0., 0., z, 0.);

  Vertex returnVertex = Vertex(seedPos);

  SquareMatrix4 seedCov = vertexingOptions.constraint.fullCovariance();

  // Check if a constraint is provided and set the new z position constraint
  if (seedCov != SquareMatrix4::Zero() && std::isnormal(zAndWidth.second)) {
    seedCov(eZ, eZ) = zAndWidth.second * zAndWidth.second;
  }

  returnVertex.setFullCovariance(seedCov);

  return std::vector<Vertex>{returnVertex};
}
