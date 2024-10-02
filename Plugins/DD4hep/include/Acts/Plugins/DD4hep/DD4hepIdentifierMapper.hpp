// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/GeometryIdMapper.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <climits>

namespace Acts {

using DD4hepIdentifier = DD4hepDetectorElement::DD4hepVolumeID;

/// @brief This struct helps to capture the DD4hep identifier
/// from the DD4hepDetectorElement from a Acts::Surface
///
/// It can be used for mapping/assigning geometry ids to surfaces
struct DD4hepIdentifierCapture {
  /// @brief Return an invalid identifier for volumes as they are not directly translated
  /// @return maximum value
  DD4hepIdentifier operator()(
      const Acts::Experimental::DetectorVolume& /*volume*/) const {
    return std::numeric_limits<DD4hepIdentifier>::max();
  }

  /// @brief Return an invalid identifier for portal objects as they are not directly translated
  /// @return maximum value
  DD4hepIdentifier operator()(
      const Acts::Experimental::Portal& /*portal*/) const {
    return std::numeric_limits<DD4hepIdentifier>::max();
  }

  /// @brief Return the DD4hep identifier from the DD4hepDetectorElement associated to the surface
  /// @param surface the Acts::Surface object
  /// @return the DD4hep identifier (or maximum value if now DD4hepDetectorElement is associated)
  DD4hepIdentifier operator()(const Acts::Surface& surface) const {
    // Get the DD4hepDetectorElement
    const auto* dde = surface.associatedDetectorElement();
    const auto* dd4hepDetElement =
        dynamic_cast<const Acts::DD4hepDetectorElement*>(dde);
    // Check if it is valid
    if (dd4hepDetElement) {
      return dd4hepDetElement->sourceElement().volumeID();
    }  // Return the maximum value
    return std::numeric_limits<DD4hepIdentifier>::max();
  }
};

using DD4hepIdentifierMapper =
    Experimental::GeometryIdMapper<DD4hepIdentifier, DD4hepIdentifierCapture>;

}  // namespace Acts
