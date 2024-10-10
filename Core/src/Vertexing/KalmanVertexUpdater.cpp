// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/KalmanVertexUpdater.hpp"

#include "Acts/Vertexing/Vertex.hpp"

#include <stdexcept>

namespace Acts::KalmanVertexUpdater {

namespace detail {
template <unsigned int nDimVertex>
void updateVertexWithTrackImpl(Vertex& vtx, TrackAtVertex& trk, int sign);

template <unsigned int nDimVertex>
void updateTrackWithVertexImpl(TrackAtVertex& track, const Vertex& vtx);
}  // namespace detail

// The two functions don't contain any of the actual update code, they
// only dispatch into templated functions, effectively doing a
// runtime-to-compile time conversion.

void updateVertexWithTrack(Vertex& vtx, TrackAtVertex& trk,
                           unsigned int nDimVertex) {
  if (nDimVertex == 3) {
    detail::updateVertexWithTrackImpl<3>(vtx, trk, 1);
  } else if (nDimVertex == 4) {
    detail::updateVertexWithTrackImpl<4>(vtx, trk, 1);
  } else {
    throw std::invalid_argument(
        "The vertex dimension must either be 3 (when fitting the spatial "
        "coordinates) or 4 (when fitting the spatial coordinates + time).");
  }
}

void updateTrackWithVertex(TrackAtVertex& track, const Vertex& vtx,
                           unsigned int nDimVertex) {
  if (nDimVertex == 3) {
    detail::updateTrackWithVertexImpl<3>(track, vtx);
  } else if (nDimVertex == 4) {
    detail::updateTrackWithVertexImpl<4>(track, vtx);
  } else {
    throw std::invalid_argument(
        "The vertex dimension must either be 3 (when fitting the spatial "
        "coordinates) or 4 (when fitting the spatial coordinates + time).");
  }
}

}  // namespace Acts::KalmanVertexUpdater
