// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Geometry/GeometryContext.hpp"

namespace Acts::Experimental {

/// @brief  This is the interface for detector component builders;
/// such a builder could be a simple detector volume builder, with
/// or without internal structure, or more complicated objects.
///
class IDetectorComponentBuilder {
 public:
  virtual ~IDetectorComponentBuilder() = default;

  /// The interface method to be implemented by all detector
  /// component builder
  ///
  /// @param gctx The geometry context for this call
  ///
  /// @return an outgoing detector component
  virtual DetectorComponent construct(const GeometryContext& gctx) const = 0;
};

}  // namespace Acts::Experimental
