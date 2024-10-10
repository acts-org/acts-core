// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"

namespace Acts {

// Dummy track linearizer
class DummyTrackLinearizer;

/// @class DummyVertexFitter
/// @brief Dummy vertex fitter class, only to be used
/// for ensuring interfaces where a vertex fitter type is
/// required but no fitter is actually needed
template <typename linearizer_t = DummyTrackLinearizer>
class DummyVertexFitter {
 public:
  using Linearizer_t = linearizer_t;
  using Propagator_t = void;

  // Do not allow an instance creation
  DummyVertexFitter() = delete;

  /// @brief Dummy fit method
  Result<Vertex> fit(const std::vector<InputTrack>&, const linearizer_t&,
                     const VertexingOptions&) const;
};

}  // namespace Acts
