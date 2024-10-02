// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/MaterialSlab.hpp"

namespace Acts {

class Surface;

/// @brief The Material interaction volume struct
/// It acts as a switch between detctor and tracking volume
/// as long as those co-exist alongside
struct InteractionVolume {
  /// The tracking volume
  const TrackingVolume* trackingVolume = nullptr;
  /// The detector volume
  const Experimental::DetectorVolume* detectorVolume = nullptr;

  /// Empty constructor
  InteractionVolume() = default;

  /// Constructor from tracking volume
  /// @param tv The tracking volume
  InteractionVolume(const TrackingVolume* tv) : trackingVolume(tv) {}

  /// Constructor from detector volume
  /// @param dv The detector volume
  InteractionVolume(const Experimental::DetectorVolume* dv)
      : detectorVolume(dv) {}

  /// Forward the geometry identifier
  GeometryIdentifier geometryId() const {
    if (trackingVolume != nullptr) {
      return trackingVolume->geometryId();
    } else if (detectorVolume != nullptr) {
      return detectorVolume->geometryId();
    } else {
      return GeometryIdentifier();
    }
  }

  /// Check if the volume is valid
  bool empty() const {
    return trackingVolume == nullptr && detectorVolume == nullptr;
  }
};

/// @brief The Material interaction struct
/// It records the surface  and the passed material
/// This is only necessary recorded when configured
struct MaterialInteraction {
  /// The particle position at the interaction.
  Vector3 position = Vector3(0., 0., 0);
  /// The particle time at the interaction.
  double time = 0.0;
  /// The particle direction at the interaction.
  Vector3 direction = Vector3(0., 0., 0);
  /// The momentum change due to the interaction.
  double deltaP = 0.0;
  /// Expected phi variance due to the interactions.
  double sigmaPhi2 = 0.0;
  /// Expected theta variance due to the interactions.
  double sigmaTheta2 = 0.0;
  /// Expected q/p variance due to the interactions.
  double sigmaQoP2 = 0.0;
  /// The position where the interaction occurred.
  Vector3 intersection = Vector3(0., 0., 0);
  /// The ID where the interaction occurred.
  GeometryIdentifier intersectionID;
  /// The surface where the interaction occurred.
  const Surface* surface = nullptr;
  /// The volume where the interaction occurred.
  InteractionVolume volume{};
  /// Update the volume step to implement the proper step size
  bool updatedVolumeStep = false;
  /// The path correction factor due to non-zero incidence on the surface.
  double pathCorrection = 1.;
  /// The effective, passed material properties including the path correction.
  MaterialSlab materialSlab;
};

/// Simple result struct to be returned
/// It mainly acts as an internal state which is
/// created for every propagation/extrapolation step
struct RecordedMaterial {
  // The accumulated materialInX0
  double materialInX0 = 0.;
  /// The accumulated materialInL0
  double materialInL0 = 0.;
  /// This one is only filled when recordInteractions is switched on
  std::vector<MaterialInteraction> materialInteractions;
};

/// And recorded material track
/// - this is start:  position, start momentum
///   and the Recorded material
using RecordedMaterialTrack =
    std::pair<std::pair<Acts::Vector3, Acts::Vector3>, RecordedMaterial>;

}  // namespace Acts
