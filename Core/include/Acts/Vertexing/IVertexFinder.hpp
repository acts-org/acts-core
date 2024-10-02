// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Utilities/Any.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"

#include <vector>

namespace Acts {

class Vertex;
struct InputTrack;
struct VertexingOptions;

/// Common interface for both vertex finders and vertex seed finders
class IVertexFinder {
 public:
  /// Type-erased wrapper for concrete state objects
  using State = Acts::AnyBase<128>;

  /// The main finder method that will return a set of found vertex candidates
  /// @param trackVector The input track collection
  /// @param vertexingOptions The vertexing options
  /// @param state The state object (needs to be created via @c makeState)
  /// @return The found vertex candidates
  virtual Result<std::vector<Vertex>> find(
      const std::vector<InputTrack>& trackVector,
      const VertexingOptions& vertexingOptions, State& state) const = 0;

  /// Function to create a state object for this concrete vertex finder
  /// @param mctx The magnetic field context
  /// @return The state object
  virtual State makeState(const MagneticFieldContext& mctx) const = 0;

  /// For vertex finders that have an internal state of active tracks, this
  /// method instructs them to mark used tracks for removal
  /// @param anyState The state object
  /// @param removedTracks The tracks to be removed
  virtual void setTracksToRemove(
      State& anyState, const std::vector<InputTrack>& removedTracks) const = 0;

  /// Virtual destructor
  virtual ~IVertexFinder() = default;
};
}  // namespace Acts
