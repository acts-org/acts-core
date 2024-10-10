// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/TGeo/TGeoDetectorElement.hpp"

class TGeoNode;

namespace Acts {

/// @brief ITGeoIdentierProvider
///
/// Interface class that provides an Identifier from a TGeoNode
class ITGeoIdentifierProvider {
 public:
  /// Take a geometry context and a TGeoNode and provide an identifier
  ///
  /// @param gctx is a geometry context object
  /// @param tgnode is a TGeoNode that is translated
  virtual TGeoDetectorElement::Identifier identify(
      const GeometryContext& gctx, const TGeoNode& tgnode) const = 0;
};

}  // namespace Acts
