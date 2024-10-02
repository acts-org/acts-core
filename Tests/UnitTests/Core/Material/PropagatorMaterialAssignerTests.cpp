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
#include "Acts/Detector/detail/CylindricalDetectorHelper.hpp"
#include "Acts/Geometry/CylinderLayer.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/TrackingVolumeArrayCreator.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/HomogeneousVolumeMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/PropagatorMaterialAssigner.hpp"
#include "Acts/Navigation/DetectorNavigator.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <limits>

namespace Acts::Test {

auto tContext = GeometryContext();
auto mContext = MagneticFieldContext();

ActsScalar rMin = 0.;
ActsScalar rMid = 25.;
ActsScalar rMax = 110.;

auto vCylinderOuter = std::make_shared<CylinderVolumeBounds>(rMid, rMax, 110.);

auto pCylinder = std::make_shared<const CylinderBounds>(20., 100.);

auto surfaceMaterial = std::make_shared<HomogeneousSurfaceMaterial>();

Material mat = Material::fromMolarDensity(1., 2., 3., 4., 5.);
auto volumeMaterial = std::make_shared<HomogeneousVolumeMaterial>(mat);

BOOST_AUTO_TEST_SUITE(PropagatorMaterialAssignerTestSuite)

/// Test with TrackingGeometry
BOOST_AUTO_TEST_CASE(FindSurfaceIntersectionsTrackingGeometry) {
  auto vCylinderInner =
      std::make_shared<CylinderVolumeBounds>(rMin, rMid, 110.);

  // Create a tracking geometry with  one material layer and one material volume
  auto pCylinderLayer =
      CylinderLayer::create(Transform3::Identity(), pCylinder, nullptr, 1.);
  pCylinderLayer->surfaceRepresentation().assignSurfaceMaterial(
      surfaceMaterial);

  LayerArrayCreator::Config lacConfig;
  LayerArrayCreator lac = LayerArrayCreator(lacConfig);
  auto layers = lac.layerArray(tContext, {pCylinderLayer}, rMin, rMid,
                               arbitrary, BinningValue::binR);

  auto innerVolume = std::make_shared<TrackingVolume>(
      Transform3::Identity(), vCylinderInner, nullptr, std::move(layers),
      nullptr, MutableTrackingVolumeVector{}, "InnerVolumeWithLayers");

  auto outerVolume = std::make_shared<TrackingVolume>(
      Transform3::Identity(), vCylinderOuter, volumeMaterial, nullptr, nullptr,
      MutableTrackingVolumeVector{}, "OuterVolume");
  innerVolume->glueTrackingVolume(tContext, tubeOuterCover, outerVolume.get(),
                                  tubeInnerCover);

  TrackingVolumeArrayCreator::Config tvacConfig;
  TrackingVolumeArrayCreator tvac = TrackingVolumeArrayCreator(tvacConfig);

  auto volumes = tvac.trackingVolumeArray(tContext, {innerVolume, outerVolume},
                                          BinningValue::binR);

  auto vCylinderTop = std::make_shared<CylinderVolumeBounds>(rMin, rMax, 110.);

  auto topVolume = std::make_shared<TrackingVolume>(
      Transform3::Identity(), vCylinderTop, nullptr, nullptr, volumes,
      MutableTrackingVolumeVector{}, "TopVolume");

  auto tGeometry = std::make_shared<TrackingGeometry>(topVolume);

  // Create a navigator and a propagator
  Navigator::Config navConfig{tGeometry};
  Navigator navigator(navConfig);

  using StraightLinePropagator = Propagator<StraightLineStepper, Navigator>;

  StraightLineStepper stepper;
  StraightLinePropagator propagator(stepper, navigator);

  using PropagationMaterialAssigner =
      PropagatorMaterialAssigner<StraightLinePropagator>;

  PropagationMaterialAssigner pmAssigner(propagator);
  auto [surfaceCandides, volumeCandidates] = pmAssigner.assignmentCandidates(
      tContext, mContext, Vector3(0, 0, 0), Vector3(1, 1, 0).normalized());

  BOOST_CHECK_EQUAL(surfaceCandides.size(), 1u);
  BOOST_CHECK_EQUAL(volumeCandidates.size(), 1u);
}

// Test with Detector
BOOST_AUTO_TEST_CASE(FindSurfaceIntersectionsTrackingVolume) {
  unsigned int volID = 1;
  auto assignGeoIds = [&volID](Experimental::DetectorVolume& dVol) -> void {
    dVol.assignGeometryId(GeometryIdentifier().setVolume(volID));
    unsigned int pID = 1;
    for (auto& p : dVol.portalPtrs()) {
      p->surface().assignGeometryId(
          GeometryIdentifier().setVolume(volID).setBoundary(pID));
    }
    volID++;
  };

  auto portalGenerator = Experimental::defaultPortalGenerator();

  ActsScalar rInnerL0 = 19;
  ActsScalar rOuterL0 = 21;

  Transform3 nominal = Transform3::Identity();

  auto vCylinderGap0 =
      std::make_shared<CylinderVolumeBounds>(rMin, rInnerL0, 110.);

  auto vCylinderL0 =
      std::make_shared<CylinderVolumeBounds>(rInnerL0, rOuterL0, 110.);

  auto vCylinderGap1 =
      std::make_shared<CylinderVolumeBounds>(rOuterL0, rMid, 110.);

  auto gap0 = Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Gap0", nominal, std::move(vCylinderGap0),
      Experimental::tryAllPortals());
  assignGeoIds(*gap0);

  auto pCylinderSurface =
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), pCylinder);
  pCylinderSurface->assignSurfaceMaterial(surfaceMaterial);
  pCylinderSurface->assignGeometryId(GeometryIdentifier().setSensitive(1));

  auto layer0 = Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Layer0", nominal, std::move(vCylinderL0),
      {pCylinderSurface}, {}, Experimental::tryNoVolumes(),
      Experimental::tryAllPortalsAndSurfaces());
  assignGeoIds(*layer0);

  auto gap1 = Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Gap1", nominal, std::move(vCylinderGap1),
      Experimental::tryAllPortals());
  assignGeoIds(*gap1);

  auto outerVolume = Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "OuterVolume", nominal, vCylinderOuter,
      Experimental::tryAllPortals());
  outerVolume->assignVolumeMaterial(volumeMaterial);
  assignGeoIds(*outerVolume);

  std::vector<std::shared_ptr<Experimental::DetectorVolume>> volumes = {
      gap0, layer0, gap1, outerVolume};

  auto pc = Experimental::detail::CylindricalDetectorHelper::connectInR(
      tContext, volumes);

  auto detector = Acts::Experimental::Detector::makeShared(
      "Detector", volumes, Acts::Experimental::tryRootVolumes());

  // Create a navigator and a propagator
  Experimental::DetectorNavigator::Config navConfig{detector.get()};
  Experimental::DetectorNavigator navigator(navConfig);

  using StraightLinePropagator =
      Propagator<StraightLineStepper, Experimental::DetectorNavigator>;

  StraightLineStepper stepper;
  StraightLinePropagator propagator(stepper, navigator);

  using PropagationMaterialAssigner =
      PropagatorMaterialAssigner<StraightLinePropagator>;

  PropagationMaterialAssigner pmAssigner(propagator);
  auto [surfaceCandides, volumeCandidates] = pmAssigner.assignmentCandidates(
      tContext, mContext, Vector3(0, 0, 0), Vector3(1, 1, 0).normalized());

  BOOST_CHECK_EQUAL(surfaceCandides.size(), 1u);
  BOOST_CHECK_EQUAL(volumeCandidates.size(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
