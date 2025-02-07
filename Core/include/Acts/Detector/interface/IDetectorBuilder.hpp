// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
