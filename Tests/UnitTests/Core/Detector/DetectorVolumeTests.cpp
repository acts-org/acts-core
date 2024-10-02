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
#include "Acts/Detector/Portal.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/HomogeneousVolumeMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

/// Unpack to shared - simply to test the getSharedPtr mechanism
///
/// @tparam referenced_type is the type of the referenced object
///
/// @param rt is the referenced object
///
/// @returns a shared pointer
template <typename referenced_type>
std::shared_ptr<referenced_type> unpackToShared(referenced_type& rt) {
  return rt.getSharedPtr();
}

using namespace Acts::Experimental;

Acts::GeometryContext tContext;

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(SurfaceVolumeContainment) {
  // Create a surface that is placed way off
  auto surfaceOutOfBounds = Acts::Surface::makeShared<Acts::CylinderSurface>(
      Acts::Transform3::Identity() * Acts::Translation3(-1000, 0., 0.),
      std::make_shared<Acts::CylinderBounds>(1, 1));

  auto vBounds = Acts::CuboidVolumeBounds(10.0, 10.0, 10.0);
  auto portalGenerator =
      Acts::Experimental::defaultPortalAndSubPortalGenerator();
  BOOST_CHECK_THROW(
      Acts::Experimental::DetectorVolumeFactory::construct(
          portalGenerator, Acts::GeometryContext(),
          "CubeWithOutofBoundsSurface", Acts::Transform3::Identity(),
          std::make_shared<Acts::CuboidVolumeBounds>(vBounds),
          {surfaceOutOfBounds}, {}, Acts::Experimental::tryAllSubVolumes(),
          Acts::Experimental::tryAllPortalsAndSurfaces(), 1000),
      std::invalid_argument);

  // Create a surface that is too big
  auto surfaceTooBig = Acts::Surface::makeShared<Acts::PlaneSurface>(
      Acts::Transform3::Identity() * Acts::Translation3(0, 0., 0.),
      std::make_shared<Acts::RectangleBounds>(1, 100));

  BOOST_CHECK_THROW(
      Acts::Experimental::DetectorVolumeFactory::construct(
          portalGenerator, Acts::GeometryContext(), "CubeWithSurfaceTooBig",
          Acts::Transform3::Identity(),
          std::make_shared<Acts::CuboidVolumeBounds>(vBounds), {surfaceTooBig},
          {}, Acts::Experimental::tryAllSubVolumes(),
          Acts::Experimental::tryAllPortalsAndSurfaces(), 1000),
      std::invalid_argument);

  // Envelope a bigger volume into a smaller one
  auto bigVolume = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, Acts::GeometryContext(), "BigCube",
      Acts::Transform3::Identity(),
      std::make_shared<Acts::CuboidVolumeBounds>(vBounds), {}, {},
      Acts::Experimental::tryAllSubVolumes(),
      Acts::Experimental::tryAllPortalsAndSurfaces(), 1000);

  auto smallBounds = Acts::CuboidVolumeBounds(1.0, 1.0, 1.0);
  BOOST_CHECK_THROW(
      Acts::Experimental::DetectorVolumeFactory::construct(
          portalGenerator, Acts::GeometryContext(),
          "SmallCubeWithBigCubeInside", Acts::Transform3::Identity(),
          std::make_shared<Acts::CuboidVolumeBounds>(smallBounds), {},
          {bigVolume}, Acts::Experimental::tryAllSubVolumes(),
          Acts::Experimental::tryAllPortalsAndSurfaces(), 1000),
      std::invalid_argument);

  // Envelope a misaligned subvolume
  auto smallVolumeMisaligned =
      Acts::Experimental::DetectorVolumeFactory::construct(
          portalGenerator, Acts::GeometryContext(), "SmallCubeMisaligned",
          Acts::Transform3::Identity() * Acts::Translation3(9.5, 0., 0.),
          std::make_shared<Acts::CuboidVolumeBounds>(smallBounds), {}, {},
          Acts::Experimental::tryAllSubVolumes(),
          Acts::Experimental::tryAllPortalsAndSurfaces(), 1000);

  BOOST_CHECK_THROW(
      Acts::Experimental::DetectorVolumeFactory::construct(
          portalGenerator, Acts::GeometryContext(), "CubeWithMisalignedVolume",
          Acts::Transform3::Identity(),
          std::make_shared<Acts::CuboidVolumeBounds>(vBounds), {},
          {smallVolumeMisaligned}, Acts::Experimental::tryAllSubVolumes(),
          Acts::Experimental::tryAllPortalsAndSurfaces(), 1000),
      std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(CylindricalDetectorVolumePortals) {
  Acts::ActsScalar rInner = 10.;
  Acts::ActsScalar rOuter = 100.;
  Acts::ActsScalar zHalfL = 200.;

  Acts::Transform3 nominal = Acts::Transform3::Identity();

  auto fullCylinderBounds =
      std::make_unique<Acts::CylinderVolumeBounds>(0., rOuter, zHalfL);

  auto portalGenerator = defaultPortalGenerator();

  // Misconfigured - null pointer for bounds
  BOOST_CHECK_THROW(
      DetectorVolumeFactory::construct(portalGenerator, tContext,
                                       "MisconfiguredFullCylinderVolume",
                                       nominal, nullptr, tryAllPortals()),
      std::invalid_argument);

  // Misconfigured - portal generator not connected
  PortalGenerator unconnected;
  BOOST_CHECK_THROW(
      DetectorVolumeFactory::construct(unconnected, tContext,
                                       "MisconfiguredFullCylinderVolume",
                                       nominal, nullptr, tryAllPortals()),
      std::invalid_argument);

  // A full cylinder
  auto fullCylinderVolume = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "FullCylinderVolume", nominal,
      std::move(fullCylinderBounds), tryAllPortals());

  BOOST_CHECK_EQUAL(fullCylinderVolume,
                    unpackToShared<DetectorVolume>(*fullCylinderVolume));
  BOOST_CHECK_EQUAL(fullCylinderVolume,
                    unpackToShared<const DetectorVolume>(*fullCylinderVolume));

  BOOST_CHECK(fullCylinderVolume->surfaces().empty());
  BOOST_CHECK(fullCylinderVolume->volumes().empty());
  BOOST_CHECK_EQUAL(fullCylinderVolume->portals().size(), 3u);

  // A tube cylinder
  auto tubeCylinderBounds =
      std::make_unique<Acts::CylinderVolumeBounds>(rInner, rOuter, zHalfL);

  auto tubeCylinderVolume = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "TubeCylinderVolume", nominal,
      std::move(tubeCylinderBounds), tryAllPortals());

  BOOST_CHECK(tubeCylinderVolume->surfaces().empty());
  BOOST_CHECK(tubeCylinderVolume->volumes().empty());
  BOOST_CHECK_EQUAL(tubeCylinderVolume->portals().size(), 4u);

  // Let's test the resizing, first inside test: OK
  BOOST_CHECK(tubeCylinderVolume->inside(tContext, Acts::Vector3(50., 0., 0.)));
  // Outside
  BOOST_CHECK(
      !tubeCylinderVolume->inside(tContext, Acts::Vector3(150., 0., 0.)));

  // Check the extent
  auto volumeExtent = tubeCylinderVolume->extent(tContext, 1);
  CHECK_CLOSE_ABS(volumeExtent.min(Acts::BinningValue::binR), 10., 10e-5);
  CHECK_CLOSE_ABS(volumeExtent.max(Acts::BinningValue::binR), 100., 10e-5);
  CHECK_CLOSE_ABS(volumeExtent.min(Acts::BinningValue::binZ), -200., 10e-5);
  CHECK_CLOSE_ABS(volumeExtent.max(Acts::BinningValue::binZ), 200., 10e-5);
}

BOOST_AUTO_TEST_CASE(UpdatePortal) {
  Acts::Transform3 nominal = Acts::Transform3::Identity();

  auto fullCylinderBounds =
      std::make_unique<Acts::CylinderVolumeBounds>(0., 10., 100.);

  auto portalGenerator = defaultPortalGenerator();

  auto fullCylinderVolume = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "FullCylinderVolume", nominal,
      std::move(fullCylinderBounds), tryAllPortals());

  auto cylinderSurface =
      Acts::Surface::makeShared<Acts::CylinderSurface>(nominal, 10., 100.);

  auto cylinderPortal =
      std::make_shared<Acts::Experimental::Portal>(cylinderSurface);

  fullCylinderVolume->updatePortal(cylinderPortal, 2u);

  BOOST_CHECK_EQUAL(fullCylinderVolume->portals()[2u], cylinderPortal.get());
}

BOOST_AUTO_TEST_CASE(CuboidWithCuboid) {
  Acts::ActsScalar bigBox = 100.;
  Acts::ActsScalar smallBox = 10.;

  Acts::Transform3 nominal = Acts::Transform3::Identity();

  auto bigBoxBounds =
      std::make_unique<Acts::CuboidVolumeBounds>(bigBox, bigBox, bigBox);

  auto smallBoxBounds =
      std::make_unique<Acts::CuboidVolumeBounds>(smallBox, smallBox, smallBox);

  auto portals = defaultPortalGenerator();
  auto generatePortalsUpdateInternals = defaultPortalAndSubPortalGenerator();

  // Create the inner box
  auto innerBox = DetectorVolumeFactory::construct(
      portals, tContext, "InnerBox", nominal, std::move(smallBoxBounds),
      tryAllPortals());

  std::vector<std::shared_ptr<Acts::Surface>> surfaces = {};
  std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> volumes = {
      innerBox};

  // Create the outer box and insert the inner box, use a portal generator
  // with sub portal registration
  auto outerBox = DetectorVolumeFactory::construct(
      generatePortalsUpdateInternals, tContext, "OuterBox", nominal,
      std::move(bigBoxBounds), surfaces, volumes, tryAllSubVolumes(),
      tryAllPortals());

  // Check that we are within the outer box
  Acts::Experimental::NavigationState nState;
  nState.position = Acts::Vector3(-50., 5., 0.);
  nState.direction = Acts::Vector3(1., 0., 0.);

  BOOST_CHECK(outerBox->inside(tContext, nState.position));
  nState.currentVolume = outerBox.get();

  outerBox->updateNavigationState(tContext, nState);

  // We should have 12 candidates, 6 inner, 6 outer portals but only 3 are
  // reachable
  BOOST_CHECK_EQUAL(nState.surfaceCandidates.size(), 3u);

  // Check surface visiting - const access
  // Test the visitor pattern for surfaces
  std::size_t nSurfaces = 0;
  outerBox->visitSurfaces([&nSurfaces](const auto* s) {
    if (s != nullptr) {
      nSurfaces++;
    }
  });
  // 6 portlas outer box, 6 portals inner box
  BOOST_CHECK_EQUAL(nSurfaces, 12u);

  // Check volume visiting - const access
  std::size_t nVolumes = 0;
  outerBox->visitVolumes([&nVolumes](const auto* v) {
    if (v != nullptr) {
      nVolumes++;
    }
  });
  BOOST_CHECK_EQUAL(nVolumes, 2u);

  // Check surface visiting - non-const access
  // Test visitor pattern - non-const access
  struct SetMaterial {
    /// The material to set
    std::shared_ptr<const Acts::HomogeneousSurfaceMaterial> surfaceMaterial =
        std::make_shared<Acts::HomogeneousSurfaceMaterial>(Acts::MaterialSlab(
            Acts::Material::fromMolarDensity(1., 2., 3., 4., 5.), 1.));

    std::shared_ptr<Acts::HomogeneousVolumeMaterial> volumeMaterial =
        std::make_shared<Acts::HomogeneousVolumeMaterial>(
            Acts::Material::fromMolarDensity(1., 2., 3., 4., 5.));

    /// The visitor call: set surface material
    void operator()(Acts::Surface* s) {
      if (s != nullptr) {
        s->assignSurfaceMaterial(surfaceMaterial);
      }
    }

    /// The visitor call : set volume material
    void operator()(DetectorVolume* v) {
      if (v != nullptr) {
        v->assignVolumeMaterial(volumeMaterial);
      }
    }
  };

  SetMaterial setMaterial;
  outerBox->visitMutableSurfaces(setMaterial);
  outerBox->visitMutableVolumes(setMaterial);

  // Count surfaces with material
  std::size_t nSurfacesWithMaterial = 0;
  outerBox->visitSurfaces([&nSurfacesWithMaterial](const auto* s) {
    if (s != nullptr && s->surfaceMaterial() != nullptr) {
      nSurfacesWithMaterial++;
    }
  });
  BOOST_CHECK_EQUAL(nSurfacesWithMaterial, 12u);

  // Count volumes with material
  std::size_t nVolumesWithMaterial = 0;
  outerBox->visitVolumes([&nVolumesWithMaterial](const auto* v) {
    if (v != nullptr && v->volumeMaterial() != nullptr) {
      nVolumesWithMaterial++;
    }
  });
  BOOST_CHECK_EQUAL(nVolumesWithMaterial, 2u);
}

BOOST_AUTO_TEST_CASE(CylinderWithSurfacesTestExtractors) {
  auto portalGenerator = defaultPortalGenerator();

  std::vector<Acts::ActsScalar> radii = {100, 102, 104, 106, 108, 110};
  auto cylinderVoumeBounds =
      std::make_unique<Acts::CylinderVolumeBounds>(80, 130, 200);
  std::vector<std::shared_ptr<Acts::Surface>> surfaces = {};
  for (const auto& r : radii) {
    surfaces.push_back(Acts::Surface::makeShared<Acts::CylinderSurface>(
        Acts::Transform3::Identity(),
        std::make_shared<Acts::CylinderBounds>(r, 190)));
  }

  // A full cylinder
  auto cylinderVolume = DetectorVolumeFactory::construct(
      portalGenerator, tContext, "CylinderVolume", Acts::Transform3::Identity(),
      std::move(cylinderVoumeBounds), surfaces, {}, tryNoVolumes(),
      tryAllPortalsAndSurfaces());

  // The navigation state
  NavigationState nState;
  AllPortalsExtractor allPortals;
  AllSurfacesExtractor allSurfaces;
  IndexedSurfacesExtractor indexedSurfaces;

  // First check exception behaviour
  BOOST_CHECK_THROW(allPortals.extract(tContext, nState), std::runtime_error);
  BOOST_CHECK_THROW(allSurfaces.extract(tContext, nState), std::runtime_error);
  BOOST_CHECK_THROW(indexedSurfaces.extract(tContext, nState, {0u, 1u}),
                    std::runtime_error);

  // A volume needs to be set
  nState.currentVolume = cylinderVolume.get();

  // This extracts all portals as candidates
  auto eportals = allPortals.extract(tContext, nState);
  BOOST_CHECK_EQUAL(eportals.size(), 4u);

  auto esurfaces = allSurfaces.extract(tContext, nState);
  BOOST_CHECK_EQUAL(esurfaces.size(), 6u);

  esurfaces = indexedSurfaces.extract(tContext, nState, {2u, 4u});
  BOOST_CHECK_EQUAL(esurfaces.size(), 2u);
  BOOST_CHECK_EQUAL(esurfaces[0u], surfaces[2u].get());
  BOOST_CHECK_EQUAL(esurfaces[1u], surfaces[4u].get());

  // Test the visitor pattern for surfaces
  struct CountSurfaces {
    unsigned int counter = 0;

    void operator()(const Acts::Surface* s) {
      if (s != nullptr) {
        counter++;
      }
    }
  };

  CountSurfaces countSurfaces;
  cylinderVolume->visitSurfaces(countSurfaces);

  // 6 internal surfaces, 4 portals -> 10 surfaces counted
  BOOST_CHECK_EQUAL(countSurfaces.counter, 10u);
}

BOOST_AUTO_TEST_SUITE_END()
