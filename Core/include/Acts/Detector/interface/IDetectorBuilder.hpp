// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"

#include <memory>

namespace Acts::Experimental {

class Detector;

/// @brief This is the interface for building a Detector object
///
/// This is the top level interface, all lower level building
/// blocks should be done with detector component builders.
///
/// @note As this is the last builder, the returned detector object
/// is const and cannot be modified anymore.
class IDetectorBuilder {
 public:
  virtual ~IDetectorBuilder() = default;

  /// The virtual interface definition for detector builders
  ///
  /// @param gctx the geometry context at the creation of the internal structure
  ///
  /// @return a shared detector object
  virtual std::shared_ptr<const Detector> construct(
      const GeometryContext& gctx) const = 0;
};

}  // namespace Acts::Experimental
