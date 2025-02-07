// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
