// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Utilities/Concepts.hpp"

namespace Acts {

/// @brief Concept ensuring compatibility TrackingGeometry
/// and Detector navigation interfaces with the client code
/// @tparam propagator_state_t Type of the object for navigation state
/// @tparam navigator_t Type of the navigator object
template <typename propagator_state_t, typename navigator_t>
concept NavigationCompatibleConcept = requires(propagator_state_t &p,
                                               navigator_t &n) {
  requires requires {
    {
      n.currentVolume(p.navigation)
    } -> Concepts::same_as_any_of<const TrackingVolume *,
                                  const Acts::Experimental::DetectorVolume *>;
  };
};

}  // namespace Acts
