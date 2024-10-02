// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

namespace Acts {

class Vertex;
struct TrackAtVertex;

namespace KalmanVertexUpdater {

/// KalmanVertexUpdater
///
/// @brief adds or removes track from or updates current vertex
/// Based on
/// Ref. (1):
/// R. Frühwirth et al.
/// Vertex reconstruction and track bundling at the lep collider using robust
/// algorithms
/// Computer Physics Comm.: 96 (1996) 189
/// Chapter 2.1
///
/// For remarks on the weighted formalism, which we use here, see
/// Ref. (2):
/// Giacinto Piacquadio
/// Identification of b-jets and investigation of the discovery potential of a
/// Higgs boson in the WH−−>lvbb¯ channel with the ATLAS experiment
/// CERN-THESIS-2010-027
/// Section 5.3.5

/// @brief Updates vertex with knowledge of new track
/// @note KalmanVertexUpdater updates the vertex when trk is added to the fit.
/// However, it does not add the track to the TrackAtVertex list. This to be
/// done manually after calling the method.
///
///
/// @param vtx Vertex to be updated
/// @param trk Track to be used for updating the vertex
/// @param nDimVertex number of dimensions of the vertex. Can be 3 (if we only
/// fit its spatial coordinates) or 4 (if we also fit time).
void updateVertexWithTrack(Vertex& vtx, TrackAtVertex& trk,
                           unsigned int nDimVertex);

/// Based on
/// Ref. (1):
/// R. Frühwirth et al.
/// Vertex reconstruction and track bundling at the lep collider using robust
/// algorithms
/// Computer Physics Comm.: 96 (1996) 189
/// Chapter 2.1

/// @brief Refits a single track with the knowledge of
/// the vertex it has originated from
///
/// @param track Track to update
/// @param vtx Vertex to use for updating the track
/// @param nDimVertex number of dimensions of the vertex. Can be 3 (if we only
/// fit its spatial coordinates) or 4 (if we also fit time).
void updateTrackWithVertex(TrackAtVertex& track, const Vertex& vtx,
                           unsigned int nDimVertex);

}  // namespace KalmanVertexUpdater
}  // namespace Acts
