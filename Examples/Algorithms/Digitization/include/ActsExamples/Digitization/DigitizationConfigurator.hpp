// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"

#include <map>

namespace Acts {
class Surface;
}  // namespace Acts

namespace ActsExamples {

/// Helper configurator that takes a simplified (per volume, per layer)
/// Input digitization file and creates a full fletched per module
/// digitization configuration file.
///
/// It acts as a visitor and then builds a fully developed digitization file
/// for the geometric digitization, filling in the correct dimensions and
/// number of bins.
///
/// The simplified file is assumed to have just one bin for the geometric
/// digitization, which is then used to calculate the number of bins with
/// respect to the bounds range.
struct DigitizationConfigurator {
  /// Simplified input components for digitization
  Acts::GeometryHierarchyMap<DigiComponentsConfig> inputDigiComponents;

  /// Final collection of output components
  std::vector<std::pair<Acts::GeometryIdentifier, DigiComponentsConfig>>
      outputDigiComponents;

  /// This tries to compactify the output map
  bool compactify = false;

  /// High level reference configurations for compactification
  std::map<Acts::GeometryIdentifier, DigiComponentsConfig>
      volumeLayerComponents;

  /// The visitor call for the geometry
  ///
  /// @param surface is the surfaces that is visited
  ///
  /// it adds an appropriate entry into the digitisation configuration
  void operator()(const Acts::Surface* surface);
};

}  // namespace ActsExamples
