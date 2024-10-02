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
#include <string>
#include <vector>

namespace Acts {

class Layer;
using LayerPtr = std::shared_ptr<const Layer>;
using LayerVector = std::vector<LayerPtr>;

/// @class ILayerBuilder
///
/// Interface class for ILayerBuilders in a typical
/// | EC- | Central | EC+ |
/// detector setup.
///
class ILayerBuilder {
 public:
  /// Virtual destructor
  virtual ~ILayerBuilder() = default;

  /// LayerBuilder interface method
  ///
  /// @param gctx is the geometry context under
  /// which the geometry is built
  ///
  /// @return  the layers at negative side
  virtual const LayerVector negativeLayers(
      const GeometryContext& gctx) const = 0;

  /// LayerBuilder interface method
  ///
  /// @param gctx is the geometry context under
  /// which the geometry is built
  ///
  /// @return the layers at the central sector
  virtual const LayerVector centralLayers(
      const GeometryContext& gctx) const = 0;

  /// LayerBuilder interface method
  ///
  /// @param gctx is the geometry context under
  /// which the geometry is built
  ///
  /// @return  the layers at positive side
  virtual const LayerVector positiveLayers(
      const GeometryContext& gctx) const = 0;

  /// Name identification
  /// @return the string based identification
  virtual const std::string& identification() const = 0;
};

}  // namespace Acts
