// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"

#include <fstream>
#include <sstream>
#include <string>

namespace Acts::TrackingGeometryView3DTest {

GeometryContext tgContext = GeometryContext();

Test::CylindricalTrackingGeometry cGeometry(tgContext);
auto tGeometry = cGeometry();

/// Helper method to visualize all types of surfaces
///
/// @param helper The visualization helper
/// @param triangulate The directive whether to create triangular meshes
/// @param tag The test tag (mode) identification
///
/// @return a string containing all written characters

static inline std::string run(IVisualization3D& helper, bool triangulate,
                              const std::string& tag) {
  std::stringstream cStream;

  ViewConfig viewSensitive = {.color = {0, 180, 240},
                              .quarterSegments = 72,
                              .triangulate = triangulate};
  ViewConfig viewPassive = {.color = {240, 280, 0},
                            .quarterSegments = 72,
                            .triangulate = triangulate};
  ViewConfig viewVolume = {.color = {220, 220, 0},
                           .quarterSegments = 72,
                           .triangulate = triangulate};
  ViewConfig viewContainer = {.color = {220, 220, 0},
                              .quarterSegments = 72,
                              .triangulate = triangulate};
  ViewConfig viewGrid = {.color = {220, 0, 0},
                         .offset = 3.,
                         .quarterSegments = 8,
                         .triangulate = triangulate};

  const Acts::TrackingVolume& tgVolume = *(tGeometry->highestTrackingVolume());

  GeometryView3D::drawTrackingVolume(helper, tgVolume, tgContext, viewContainer,
                                     viewVolume, viewPassive, viewSensitive,
                                     viewGrid, true, tag);

  GeometryView3D::drawTrackingVolume(helper, tgVolume, tgContext, viewContainer,
                                     viewVolume, viewPassive, viewSensitive,
                                     viewGrid, false);
  helper.write(cStream);

  return cStream.str();
}

}  // namespace Acts::TrackingGeometryView3DTest
