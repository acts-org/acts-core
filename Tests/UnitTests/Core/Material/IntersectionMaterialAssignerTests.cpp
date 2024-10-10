// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/IntersectionMaterialAssigner.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <limits>

namespace Acts::Test {

auto tContext = GeometryContext();
auto mContext = MagneticFieldContext();

BOOST_AUTO_TEST_SUITE(IntersectionMaterialAssignerTestSuite)

BOOST_AUTO_TEST_CASE(FindSurfaceIntersections) {
  // Create a vector of surfaces
  std::vector<std::shared_ptr<Surface>> surfaces = {
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 20.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 50.0,
                                           100.0)};

  IntersectionMaterialAssigner::Config imCfg;
  for (const auto &surface : surfaces) {
    imCfg.surfaces.push_back(surface.get());
  }

  IntersectionMaterialAssigner imAssigner(imCfg);
  auto [surfaceCandides, volumeCandidates] = imAssigner.assignmentCandidates(
      tContext, mContext, Vector3(0, 0, 0), Vector3(1, 1, 0).normalized());

  BOOST_CHECK_EQUAL(surfaceCandides.size(), 3u);
  BOOST_CHECK_EQUAL(volumeCandidates.size(), 0u);
}

BOOST_AUTO_TEST_CASE(FindTrackingVolumeIntersections) {
  auto cylinerVolumeBounds =
      std::make_shared<CylinderVolumeBounds>(20.0, 100.0, 400.0);
  auto volume = std::make_shared<TrackingVolume>(
      Transform3::Identity(), cylinerVolumeBounds, nullptr, nullptr, nullptr,
      MutableTrackingVolumeVector{}, "CylindricalTrackignVolume");

  IntersectionMaterialAssigner::Config imCfg;
  imCfg.trackingVolumes.push_back(volume.get());

  IntersectionMaterialAssigner imAssigner(imCfg);
  auto [surfaceCandides, volumeCandidates] = imAssigner.assignmentCandidates(
      tContext, mContext, Vector3(0, 0, 0), Vector3(1, 1, 0).normalized());

  BOOST_CHECK_EQUAL(surfaceCandides.size(), 0u);
  BOOST_CHECK_EQUAL(volumeCandidates.size(), 1u);
}

BOOST_AUTO_TEST_CASE(FindDetectorVolumeIntersections) {
  auto cylinerVolumeBounds =
      std::make_shared<CylinderVolumeBounds>(20.0, 100.0, 400.0);

  auto portalGenerator = Experimental::defaultPortalGenerator();

  auto volume = Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "CylindricalDetectorVolume",
      Transform3::Identity(), std::move(cylinerVolumeBounds),
      Experimental::tryAllPortals());

  IntersectionMaterialAssigner::Config imCfg;
  imCfg.detectorVolumes.push_back(volume.get());

  IntersectionMaterialAssigner imAssigner(imCfg);
  auto [surfaceCandides, volumeCandidates] = imAssigner.assignmentCandidates(
      tContext, mContext, Vector3(0, 0, 0), Vector3(1, 1, 0).normalized());

  BOOST_CHECK_EQUAL(surfaceCandides.size(), 0u);
  BOOST_CHECK_EQUAL(volumeCandidates.size(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
