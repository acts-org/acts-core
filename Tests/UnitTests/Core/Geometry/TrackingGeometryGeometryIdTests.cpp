// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/ApproachDescriptor.hpp"
#include "Acts/Geometry/BoundarySurfaceT.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/BinnedArray.hpp"

#include <memory>
#include <vector>

#include "TrackingVolumeCreation.hpp"

using namespace Acts::UnitLiterals;

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();

///  create three cylinder surfaces
///  the surface radius (will also be the layer radius)
double iVsurfaceHalfLengthZ = 50_mm;
double iVsurfaceR = 25_mm;
double iVsurfaceRstagger = 5_mm;
double iVsurfaceZoverlap = 10_mm;
double iVlayerEnvelope = 0.5_mm;
double iVvolumeEnvelope = 10_mm;
double iVvolumeR =
    iVsurfaceR + 0.5 * iVsurfaceRstagger + iVlayerEnvelope + iVvolumeEnvelope;

///  the surface radius (will also be the layer radius)
double oVsurfaceHalfLengthZ = 50_mm;
double oVsurfaceR = 100_mm;
double oVsurfaceRstagger = 5_mm;
double oVsurfaceZoverlap = 10_mm;
double oVlayerEnvelope = 0.5_mm;
double oVvolumeEnvelope = 10_mm;
double oVvolumeR =
    oVsurfaceR + 0.5 * oVsurfaceRstagger + oVlayerEnvelope + oVvolumeEnvelope;

///  inner volume
auto iVolume = constructCylinderVolume(
    tgContext, iVsurfaceHalfLengthZ, iVsurfaceR, iVsurfaceRstagger,
    iVsurfaceZoverlap, iVlayerEnvelope, iVvolumeEnvelope, 0., iVvolumeR,
    "InnerVolume");

BOOST_AUTO_TEST_CASE(GeometryIdentifier_innervolume_test) {
  BOOST_CHECK_EQUAL(0ul, iVolume->geometryId().value());
  // check the boundary surfaces
  for (const auto& bSf : iVolume->boundarySurfaces()) {
    BOOST_CHECK_EQUAL(0ul, bSf->surfaceRepresentation().geometryId().value());
    for (const auto& lay : iVolume->confinedLayers()->arrayObjects()) {
      BOOST_CHECK_EQUAL(0ul, lay->geometryId().value());
      // check the approach surfaces
      for (const auto& asf : lay->approachDescriptor()->containedSurfaces()) {
        BOOST_CHECK_EQUAL(0ul, asf->geometryId().value());
      }
      // check the layer surface array
      for (const auto& ssf : lay->surfaceArray()->surfaces()) {
        BOOST_CHECK_EQUAL(0ul, ssf->geometryId().value());
      }
    }
  }
}

///  outer volume
auto oVolume = constructCylinderVolume(
    tgContext, oVsurfaceHalfLengthZ, oVsurfaceR, oVsurfaceRstagger,
    oVsurfaceZoverlap, oVlayerEnvelope, oVvolumeEnvelope, iVvolumeR, oVvolumeR,
    "OuterVolume");

BOOST_AUTO_TEST_CASE(GeometryIdentifier_outervolume_test) {
  BOOST_CHECK_EQUAL(0ul, oVolume->geometryId().value());
  // check the boundary surfaces
  for (const auto& bSf : iVolume->boundarySurfaces()) {
    BOOST_CHECK_EQUAL(0ul, bSf->surfaceRepresentation().geometryId().value());
    for (const auto& lay : oVolume->confinedLayers()->arrayObjects()) {
      BOOST_CHECK_EQUAL(0ul, lay->geometryId().value());
      // check the approach surfaces
      for (const auto& asf : lay->approachDescriptor()->containedSurfaces()) {
        BOOST_CHECK_EQUAL(0ul, asf->geometryId().value());
      }
      // check the layer surface array
      for (const auto& ssf : lay->surfaceArray()->surfaces()) {
        BOOST_CHECK_EQUAL(0ul, ssf->geometryId().value());
      }
    }
  }
}
//
double oVvolumeHalfZ =
    (4 * oVsurfaceHalfLengthZ - oVsurfaceZoverlap) + oVvolumeEnvelope;
// now create the container volume
auto hVolume = constructContainerVolume(tgContext, iVolume, oVolume, oVvolumeR,
                                        oVvolumeHalfZ, "Container");

///  pre-check on GeometryIdentifier
BOOST_AUTO_TEST_CASE(GeometryIdentifier_containervolume_test) {
  ///  let's check that the geometry ID values are all 0
  BOOST_CHECK_EQUAL(0ul, hVolume->geometryId().value());
  /// check the boundaries of the hVolume, should also be 0
  for (const auto& hbsf : hVolume->boundarySurfaces()) {
    BOOST_CHECK_EQUAL(0ul, hbsf->surfaceRepresentation().geometryId().value());
  }
  for (const auto& cVol : hVolume->confinedVolumes()->arrayObjects()) {
    /// let's check everything is set to 0
    BOOST_CHECK_EQUAL(0ul, cVol->geometryId().value());
    // check the boundary surfaces
    for (const auto& bSf : cVol->boundarySurfaces()) {
      BOOST_CHECK_EQUAL(0ul, bSf->surfaceRepresentation().geometryId().value());
    }
    for (const auto& lay : cVol->confinedLayers()->arrayObjects()) {
      BOOST_CHECK_EQUAL(0ul, lay->geometryId().value());
      // check the approach surfaces
      for (const auto& asf : lay->approachDescriptor()->containedSurfaces()) {
        BOOST_CHECK_EQUAL(0ul, asf->geometryId().value());
      }
      // check the layer surface array
      for (auto ssf : lay->surfaceArray()->surfaces()) {
        BOOST_CHECK_EQUAL(0ul, ssf->geometryId().value());
      }
    }
  }
}

}  // namespace Acts::Test
