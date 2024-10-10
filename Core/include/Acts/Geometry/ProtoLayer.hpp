// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace Acts {

/// @struct ProtoLayer
///
/// Encapsulates min/max boundaries that will be turned into a layer.
/// The struct allows this information to be obtained in a consistent
/// way, or be caller provided.

struct ProtoLayer {
 public:
  /// The extent of the ProtoLayer
  Extent extent;

  /// The envelope parameters
  ExtentEnvelope envelope = ExtentEnvelope::Zero();

  /// The local transform
  Transform3 transform = Transform3::Identity();

  /// Constructor
  ///
  /// Loops over a provided vector of surface and calculates the various
  /// min/max values in one go. Also takes into account the thickness
  /// of an associated DetectorElement, if it exists.
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param surfaces The vector of surfaces to consider
  /// @param transformIn The local transform to evaluate the sizing in
  ProtoLayer(const GeometryContext& gctx,
             const std::vector<const Surface*>& surfaces,
             const Transform3& transformIn = Transform3::Identity());

  /// Constructor
  ///
  /// Loops over a provided vector of surface and calculates the various
  /// min/max values in one go. Also takes into account the thickness
  /// of an associated DetectorElement, if it exists.
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param surfaces The vector of surfaces to consider
  /// @param transformIn The local transform to evaluate the sizing in
  ProtoLayer(const GeometryContext& gctx,
             const std::vector<std::shared_ptr<const Surface>>& surfaces,
             const Transform3& transformIn = Transform3::Identity());

  /// Constructor
  ///
  /// Loops over a provided vector of surface and calculates the various
  /// min/max values in one go. Also takes into account the thickness
  /// of an associated DetectorElement, if it exists.
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param surfaces The vector of surfaces to consider
  /// @param transformIn The local transform to evaluate the sizing in
  ProtoLayer(const GeometryContext& gctx,
             const std::vector<std::shared_ptr<Surface>>& surfaces,
             const Transform3& transformIn = Transform3::Identity());

  ProtoLayer() = default;

  /// Get the parameters : min
  /// @param bval The accessed binning value
  /// @param addenv The steering if enevlope is added or not
  double min(BinningValue bval, bool addenv = true) const;

  // Get the  parameters : max
  /// @param bval The accessed binning value
  /// @param addenv The steering if enevlope is added or not
  double max(BinningValue bval, bool addenv = true) const;

  // Get the  parameters : max
  /// @param bval The accessed binning value
  /// @param addenv The steering if enevlope is added or not
  double medium(BinningValue bval, bool addenv = true) const;

  // Get the  parameters : max
  /// @param bval The accessed binning value
  /// @param addenv The steering if enevlope is added or not
  double range(BinningValue bval, bool addenv = true) const;

  /// Output to ostream
  /// @param sl the input ostream
  std::ostream& toStream(std::ostream& sl) const;

  /// Output stream operator
  /// @param sl the input ostream
  /// @param pl the ProtoLayer to be printed
  /// @return the output ostream
  friend std::ostream& operator<<(std::ostream& sl, const ProtoLayer& pl) {
    return pl.toStream(sl);
  }

  /// Give access to the surfaces used/assigned to the ProtoLayer
  const std::vector<const Surface*>& surfaces() const;

  /// Add a surface, this will also increase the extent
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param surface The surface which is added to the ProtoLayer
  void add(const GeometryContext& gctx, const Surface& surface);

 private:
  /// Helper method which performs the actual min/max calculation
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param surfaces The surfaces to build this protolayer out of
  void measure(const GeometryContext& gctx,
               const std::vector<const Surface*>& surfaces);

  /// Store the list of surfaces used for this proto layer
  std::vector<const Surface*> m_surfaces = {};
};

inline const std::vector<const Surface*>& ProtoLayer::surfaces() const {
  return m_surfaces;
}

}  // namespace Acts
