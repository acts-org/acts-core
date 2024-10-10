// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Navigation/PortalNavigation.hpp"

#include <map>
#include <memory>
#include <vector>

namespace Acts {

class VolumeBounds;
class Surface;

namespace Experimental {

class DetectorVolume;
class Portal;

/// @brief Container to collect root volumes for the
/// construction of a Detector
///
/// @note each root volume is expected to contain a full
/// search tree for eventually contained sub volumes
///
/// This struct collects all root volumes that will then
/// be provided to the Detector object
struct RootDetectorVolumes {
  /// The list of root volumes
  std::vector<std::shared_ptr<DetectorVolume>> volumes = {};
  /// The Root volumes finder
  ExternalNavigationDelegate volumeFinder;
};

/// @brief The currently built detector components
/// including the constructed volumes and the current
/// shell/coating, i.e. portals ordered in a map
///
/// @note the map indices of the shell coating represent
/// their respective index in the portal vector of the
/// VolumeBounds derivative that is described by the given
/// component.
struct DetectorComponent {
  /// Define a portal container
  using PortalContainer = std::map<unsigned int, std::shared_ptr<Portal>>;
  /// The vector of constructed volume(s)
  std::vector<std::shared_ptr<DetectorVolume>> volumes = {};
  /// The current map of outside portals
  PortalContainer portals = {};
  /// The root volumes finder
  RootDetectorVolumes rootVolumes = {};
};

/// @brief Holder struct for the external structure components
/// required to construct a detectorVolume
struct ExternalStructure {
  /// A 3D transform where the volume should be positioned
  Transform3 transform = Transform3::Identity();
  /// A shape definition
  std::unique_ptr<VolumeBounds> bounds = nullptr;
  /// And a portal generator
  PortalGenerator portalGenerator;
};

/// @brief Holder struct for the internal structure components of a DetectorVolume
///
/// @note the surface surfacesUpdater needs to handle also portal providing
/// of contained volumes.
struct InternalStructure {
  /// Contained surfaces of this volume, handled by the surfacesUpdater
  std::vector<std::shared_ptr<Surface>> surfaces = {};
  /// Contained volumes of this volume, handled by the volumeUpdater
  std::vector<std::shared_ptr<DetectorVolume>> volumes = {};
  /// Navigation delegate for surfaces
  InternalNavigationDelegate surfacesUpdater;
  // Navigation delegate for volumes
  ExternalNavigationDelegate volumeUpdater;
};

}  // namespace Experimental
}  // namespace Acts
