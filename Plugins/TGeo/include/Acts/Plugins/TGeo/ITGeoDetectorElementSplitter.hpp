// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"

#include <memory>
#include <vector>

class TGeoNode;

namespace Acts {

class TGeoDetectorElement;

/// @brief ITGeoElementSplitter
///
/// Interface class that allows to define splitting of TGeoElements into
/// sub-elements
class ITGeoDetectorElementSplitter {
 public:
  virtual ~ITGeoDetectorElementSplitter() = default;

  /// Take a geometry context and TGeoElement and split it into sub elements
  ///
  /// @param gctx is a geometry context object
  /// @param tgde is a TGeoDetectorElement that is eventually split
  ///
  /// @note If no split is performed the unsplit detector element is returned
  ///
  /// @return a vector of TGeoDetectorElement objects
  virtual std::vector<std::shared_ptr<const Acts::TGeoDetectorElement>> split(
      const GeometryContext& gctx,
      std::shared_ptr<const Acts::TGeoDetectorElement> tgde) const = 0;
};

}  // namespace Acts
