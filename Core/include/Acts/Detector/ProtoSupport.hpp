// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Detector/ProtoBinning.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/BinningData.hpp"

#include <optional>
#include <stdexcept>
#include <vector>

namespace Acts::Experimental {
/// @brief Support surface building instructions
///
/// There are two ways to build a support surface:
///
/// 1. Provide a surface type and the values for the binning
///
/// 2. Provide a surface directly
///
/// In both cases a binning description for proto material can be attached,
/// if the surface is split into planar approximinations, the proto binning
/// is assumed to be
struct ProtoSupport {
  // Building instructions 1 (surface type, parameters, transform is provided):

  /// The surface type to be built
  Surface::SurfaceType type = Surface::SurfaceType::Other;

  /// The offset of the support to an estimated position (e.g. from an extent)
  ActsScalar offset = 0.;

  /// A given extent from the volume, this allows to set support surfaces
  /// to fit into given volume extensions (flagged by the binning value
  /// being constrained by this extent)
  Extent volumeExtent;

  /// The volume envelope/clearance parameters: these are chosen such that the
  /// support surface does not touch the volume extent
  ExtentEnvelope volumeClearance = ExtentEnvelope::Zero();

  /// The constrain(s) from the internal surfaces, done by parsing
  /// the polyhedron vertices of the internal objects before support building
  ///
  /// The internal constraint would overwrite the volume one in order to allow
  /// support surfaces to be fitted from global volume extensions to the
  /// actually contained internal objects.
  std::vector<BinningValue> internalConstraints = {};

  // Building instructions 2 (surface is provided):

  /// The support surface can already be provided
  std::shared_ptr<Surface> surface = nullptr;

  /// The (optional) binning description for proto material
  std::optional<BinningDescription> protoMaterialBinning = std::nullopt;

  /// Potential splits into planar approximations (valid for cylinder/disc)
  unsigned int splits = 1u;

  /// Planar placement (only valid for planar support surfaces)
  BinningValue pPlacement = BinningValue::binZ;

  /// Indicate if the support surface(s) should always be addressed in
  /// navigation
  bool assignToAll = true;
};

}  // namespace Acts::Experimental
