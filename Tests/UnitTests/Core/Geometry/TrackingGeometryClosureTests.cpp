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
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/BinnedArray.hpp"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include "TrackingVolumeCreation.hpp"

using namespace Acts::UnitLiterals;

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();

TrackingGeometry makeTrackingGeometry(const GeometryIdentifierHook& hook) {
  /// we test a two-level hierarchy
  /// every deeper level hierarchy is a derivate of this
  ///
  /// WorldVolume   with volumeID       == 1
  /// - InnerVolume with volumeID       == 2
  /// -- InnerInnerVolume with volumeID == 3
  /// -- InnerOuterVolume with volumeID == 4
  /// - OuterVolume with volumeID       == 5

  // sensitive surface definitions
  double surfaceHalfLengthZ = 50_mm;
  double surfaceRstagger = 5_mm;
  double surfaceZoverlap = 10_mm;
  double layerEnvelope = 0.5_mm;
  double volumeEnvelope = 10_mm;

  // inner inner volume definitions
  double iiv_surfaceR = 25_mm;
  double iiv_volumeR =
      iiv_surfaceR + 0.5 * surfaceRstagger + layerEnvelope + volumeEnvelope;

  ///  inner outer volume definitions
  double iov_surfaceR = 100_mm;
  double iov_volumeR =
      iov_surfaceR + 0.5 * surfaceRstagger + layerEnvelope + volumeEnvelope;

  ///  inner inner volume
  auto iiVolume = constructCylinderVolume(
      tgContext, surfaceHalfLengthZ, iiv_surfaceR, surfaceRstagger,
      surfaceZoverlap, layerEnvelope, volumeEnvelope, 0., iiv_volumeR,
      "InnerInnerVolume");
  ///  inner outer volume
  auto ioVolume = constructCylinderVolume(
      tgContext, surfaceHalfLengthZ, iov_surfaceR, surfaceRstagger,
      surfaceZoverlap, layerEnvelope, volumeEnvelope, iiv_volumeR, iov_volumeR,
      "InnerOuterVolume");

  // now create the Inner Container volume
  double volumeHalfZ =
      (4 * surfaceHalfLengthZ - surfaceZoverlap) + volumeEnvelope;
  /// the inner volume
  auto iVolume = constructContainerVolume(
      tgContext, iiVolume, ioVolume, iov_volumeR, volumeHalfZ, "InnerVolume");

  // outer volume definitions
  double ov_surfaceR = 150_mm;
  double ov_volumeR =
      ov_surfaceR + 0.5 * surfaceRstagger + layerEnvelope + volumeEnvelope;

  ///  inner outer volume
  auto oVolume = constructCylinderVolume(
      tgContext, surfaceHalfLengthZ, ov_surfaceR, surfaceRstagger,
      surfaceZoverlap, layerEnvelope, volumeEnvelope, iov_volumeR, ov_volumeR,
      "OuterVolume");
  /// the inner volume
  auto volume = constructContainerVolume(
      tgContext, iVolume, oVolume, ov_volumeR, volumeHalfZ, "WorldVolume");

  // creating a TrackingGeometry
  // -> close the geometry, this should set the GeometryIdentifier
  TrackingGeometry tGeometry(volume, nullptr, hook);
  return tGeometry;
}

BOOST_AUTO_TEST_CASE(GeometryIdentifier_closeGeometry_test) {
  GeometryIdentifierHook hook{};
  TrackingGeometry tGeometry = makeTrackingGeometry(hook);
  auto world = tGeometry.highestTrackingVolume();

  // the lambda for checking
  auto check_vol = [](const TrackingVolume& vol,
                      GeometryIdentifier::Value geoid) {
    // check the geometry id of the volume
    BOOST_CHECK_EQUAL(geoid, vol.geometryId().volume());
    // check the geometry id of all boundary surfaces of the volume
    // - this is strictly only possible when glueing is OFF
    GeometryIdentifier::Value bsurface_id = 0;
    for (const auto& bSf : vol.boundarySurfaces()) {
      // check the bsurface volume id
      auto bs_vol_id = bSf->surfaceRepresentation().geometryId().volume();
      BOOST_CHECK_EQUAL(geoid, bs_vol_id);
      // check the bsurface boundary id
      auto bs_bsf_id = bSf->surfaceRepresentation().geometryId().boundary();
      auto bs_ext_id = bSf->surfaceRepresentation().geometryId().extra();
      BOOST_CHECK_EQUAL(++bsurface_id, bs_bsf_id);
      BOOST_CHECK_EQUAL(bs_ext_id, 0);
    }
    // testing the layer and its approach surfaces
    if (vol.confinedLayers() != nullptr) {
      // layers start are counted from 1 - n
      GeometryIdentifier::Value layer_id = 0;
      for (const auto& lay : vol.confinedLayers()->arrayObjects()) {
        // check the layer volume id and layer id
        auto lay_vol_id = lay->geometryId().volume();
        auto lay_lay_id = lay->geometryId().layer();
        BOOST_CHECK_EQUAL(++layer_id, lay_lay_id);
        BOOST_CHECK_EQUAL(geoid, lay_vol_id);
        // test the layer approach surfaces
        if (lay->approachDescriptor() != nullptr) {
          // approach surfaces are counted from 1 - n
          GeometryIdentifier::Value asurface_id = 0;
          for (const auto& asf :
               lay->approachDescriptor()->containedSurfaces()) {
            // check the approach volume id, approach layer id
            auto asf_vol_id = asf->geometryId().volume();
            auto asf_lay_id = asf->geometryId().layer();
            auto asf_asf_id = asf->geometryId().approach();
            auto ssf_ext_id = asf->geometryId().extra();
            BOOST_CHECK_EQUAL(layer_id, asf_lay_id);
            BOOST_CHECK_EQUAL(geoid, asf_vol_id);
            BOOST_CHECK_EQUAL(++asurface_id, asf_asf_id);
            BOOST_CHECK_EQUAL(0, ssf_ext_id);
          }
        }
        // test the sensitive surfaces
        if (lay->surfaceArray() != nullptr) {
          // sensitive surfaces are counted from 1 - n
          GeometryIdentifier::Value ssurface_id = 0;
          for (const auto& ssf : lay->surfaceArray()->surfaces()) {
            // check the approach volume id, approach layer id
            auto ssf_vol_id = ssf->geometryId().volume();
            auto ssf_lay_id = ssf->geometryId().layer();
            auto ssf_ssf_id = ssf->geometryId().sensitive();
            auto ssf_ext_id = ssf->geometryId().extra();
            BOOST_CHECK_EQUAL(layer_id, ssf_lay_id);
            BOOST_CHECK_EQUAL(geoid, ssf_vol_id);
            BOOST_CHECK_EQUAL(++ssurface_id, ssf_ssf_id);
            BOOST_CHECK_EQUAL(0, ssf_ext_id);
          }
        }
      }
    }
  };

  // get the two volumes the world is built of
  auto ioVolumes = world->confinedVolumes()->arrayObjects();
  // check the size - has to be two volumes
  BOOST_CHECK_EQUAL(2ul, ioVolumes.size());
  // get the innermost volumes
  auto iioVolumes = ioVolumes[0]->confinedVolumes()->arrayObjects();
  // check the size - has to be two volumes
  BOOST_CHECK_EQUAL(2ul, iioVolumes.size());

  // check the world
  check_vol(*world, 1);
  // - check InnerVolume
  check_vol(*ioVolumes[0], 2);
  // -- check the InnerInnerVolume
  check_vol(*iioVolumes[0], 3);
  // -- check the InenerOuterVolume
  check_vol(*iioVolumes[1], 4);
  // - check the OuterVolume
  check_vol(*ioVolumes[1], 5);
}

template <typename Callable>
struct CallableHook : public Acts::GeometryIdentifierHook {
  Callable callable;

  CallableHook(const Callable& c) : callable(c) {}

  Acts::GeometryIdentifier decorateIdentifier(
      Acts::GeometryIdentifier identifier,
      const Acts::Surface& surface) const override {
    return callable(identifier, surface);
  }
};

BOOST_AUTO_TEST_CASE(GeometryIdentifier_closeGeometry_test_extra) {
  std::size_t extra = 0;
  std::unordered_map<const Surface*, std::size_t> extraMap;
  auto hookImpl = [&](GeometryIdentifier orig, const Surface& srf) {
    ++extra;
    extraMap[&srf] = extra;
    orig.setExtra(extra);
    return orig;
  };
  CallableHook<decltype(hookImpl)> hook{hookImpl};

  TrackingGeometry tGeometry = makeTrackingGeometry(hook);
  auto world = tGeometry.highestTrackingVolume();

  // the lambda for checking
  auto check_vol = [&extraMap](const TrackingVolume& vol,
                               GeometryIdentifier::Value geoid) {
    // check the geometry id of the volume
    BOOST_CHECK_EQUAL(geoid, vol.geometryId().volume());
    // check the geometry id of all boundary surfaces of the volume
    // - this is strictly only possible when glueing is OFF
    GeometryIdentifier::Value bsurface_id = 0;
    for (const auto& bSf : vol.boundarySurfaces()) {
      // check the bsurface volume id
      auto bs_vol_id = bSf->surfaceRepresentation().geometryId().volume();
      BOOST_CHECK_EQUAL(geoid, bs_vol_id);
      // check the bsurface boundary id
      auto bs_bsf_id = bSf->surfaceRepresentation().geometryId().boundary();
      auto bs_ext_id = bSf->surfaceRepresentation().geometryId().extra();
      BOOST_CHECK_EQUAL(++bsurface_id, bs_bsf_id);
      BOOST_CHECK_EQUAL(bs_ext_id, 0);
    }
    // testing the layer and its approach surfaces
    if (vol.confinedLayers() != nullptr) {
      // layers start are counted from 1 - n
      GeometryIdentifier::Value layer_id = 0;
      for (const auto& lay : vol.confinedLayers()->arrayObjects()) {
        // check the layer volume id and layer id
        auto lay_vol_id = lay->geometryId().volume();
        auto lay_lay_id = lay->geometryId().layer();
        BOOST_CHECK_EQUAL(++layer_id, lay_lay_id);
        BOOST_CHECK_EQUAL(geoid, lay_vol_id);
        // test the layer approach surfaces
        if (lay->approachDescriptor() != nullptr) {
          // approach surfaces are counted from 1 - n
          GeometryIdentifier::Value asurface_id = 0;
          for (const auto& asf :
               lay->approachDescriptor()->containedSurfaces()) {
            // check the approach volume id, approach layer id
            auto asf_vol_id = asf->geometryId().volume();
            auto asf_lay_id = asf->geometryId().layer();
            auto asf_asf_id = asf->geometryId().approach();
            auto ssf_ext_id = asf->geometryId().extra();
            BOOST_CHECK_EQUAL(layer_id, asf_lay_id);
            BOOST_CHECK_EQUAL(geoid, asf_vol_id);
            BOOST_CHECK_EQUAL(++asurface_id, asf_asf_id);
            BOOST_CHECK_EQUAL(0, ssf_ext_id);
          }
        }
        // test the sensitive surfaces
        if (lay->surfaceArray() != nullptr) {
          // sensitive surfaces are counted from 1 - n
          GeometryIdentifier::Value ssurface_id = 0;
          for (const auto& ssf : lay->surfaceArray()->surfaces()) {
            // check the approach volume id, approach layer id
            auto ssf_vol_id = ssf->geometryId().volume();
            auto ssf_lay_id = ssf->geometryId().layer();
            auto ssf_ssf_id = ssf->geometryId().sensitive();
            auto ssf_ext_id = ssf->geometryId().extra();
            BOOST_CHECK_EQUAL(layer_id, ssf_lay_id);
            BOOST_CHECK_EQUAL(geoid, ssf_vol_id);
            BOOST_CHECK_EQUAL(++ssurface_id, ssf_ssf_id);
            BOOST_CHECK_EQUAL(extraMap[ssf], ssf_ext_id);
          }
        }
      }
    }
  };

  // get the two volumes the world is built of
  auto ioVolumes = world->confinedVolumes()->arrayObjects();
  // check the size - has to be two volumes
  BOOST_CHECK_EQUAL(2ul, ioVolumes.size());
  // get the innermost volumes
  auto iioVolumes = ioVolumes[0]->confinedVolumes()->arrayObjects();
  // check the size - has to be two volumes
  BOOST_CHECK_EQUAL(2ul, iioVolumes.size());

  // check the world
  check_vol(*world, 1);
  // - check InnerVolume
  check_vol(*ioVolumes[0], 2);
  // -- check the InnerInnerVolume
  check_vol(*iioVolumes[0], 3);
  // -- check the InenerOuterVolume
  check_vol(*iioVolumes[1], 4);
  // - check the OuterVolume
  check_vol(*ioVolumes[1], 5);
}

BOOST_AUTO_TEST_CASE(TrackingGeometry_testVisitSurfaces) {
  GeometryIdentifierHook hook{};
  TrackingGeometry tGeometry = makeTrackingGeometry(hook);

  // this will also cover TrackingVolume::visitSurfaces
  // it's a pretty bare-bones test, and only asserts that the
  // method is called on the expected number of surfaces
  std::size_t nSurfaces = 0;
  tGeometry.visitSurfaces([&nSurfaces](const auto*) { nSurfaces++; });
  BOOST_CHECK_EQUAL(nSurfaces, 9u);

  // this will also cover TrackingVolume::visitVolumes
  std::size_t nVolumes = 0;
  tGeometry.visitVolumes([&nVolumes](const auto*) { nVolumes++; });
  BOOST_CHECK_EQUAL(nVolumes, 5u);
}

}  // namespace Acts::Test
