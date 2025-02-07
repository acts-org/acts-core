// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
