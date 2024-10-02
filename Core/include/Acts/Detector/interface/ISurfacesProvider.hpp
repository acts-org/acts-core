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
#include <vector>

namespace Acts {

class Surface;

namespace Experimental {

/// @brief This is the interface for providing surfaces
/// to the detector building process. These surfaces manly
/// describe the sensitive detector surfaces, but also passive
/// (i.e. material carrying) surfaces are considered.
///
/// These could be prefilled, or created on demand when
/// the detector is built (to increase memory locality)
class ISurfacesProvider {
 public:
  virtual ~ISurfacesProvider() = default;

  /// The virtual interface definition for detector surface providers
  ///
  /// @param gctx the geometry context at the creation of the internal structure
  ///
  /// @return a shared detector object
  virtual std::vector<std::shared_ptr<Surface>> surfaces(
      const GeometryContext& gctx) const = 0;
};

}  // namespace Experimental
}  // namespace Acts
