// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/HomogeneousVolumeMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"

#include <memory>
#include <stdexcept>
#include <string>
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

Acts::GeometryContext tContext;

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(DetectorConstruction) {
  Acts::ActsScalar r0 = 0.;
  Acts::ActsScalar r1 = 10.;
  Acts::ActsScalar r2 = 100.;
  Acts::ActsScalar r3 = 200.;
  Acts::ActsScalar zHalfL = 200.;

  Acts::Transform3 nominal = Acts::Transform3::Identity();

  // Create a bunch of volumes
  auto cyl0Bounds =
      std::make_unique<Acts::CylinderVolumeBounds>(r0, r1, zHalfL);

  auto cyl0BoundsCopy =
      std::make_unique<Acts::CylinderVolumeBounds>(r0, r1, zHalfL);

  auto cyl1Bounds =
      std::make_unique<Acts::CylinderVolumeBounds>(r1, r2, zHalfL);

  auto cyl2Bounds =
      std::make_unique<Acts::CylinderVolumeBounds>(r2, r3, zHalfL);

  auto portalGenerator = Acts::Experimental::defaultPortalGenerator();

  auto cyl0 = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Cyl0", nominal, std::move(cyl0Bounds),
      Acts::Experimental::tryAllPortals());

  auto cyl0nameDup = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Cyl0", nominal, std::move(cyl0BoundsCopy),
      Acts::Experimental::tryAllPortals());

  auto cyl1 = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Cyl1", nominal, std::move(cyl1Bounds),
      Acts::Experimental::tryAllPortals());

  auto cyl2 = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "Cyl2", nominal, std::move(cyl2Bounds),
      Acts::Experimental::tryAllPortals());

  std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> volumes012 =
      {cyl0, cyl1, cyl2};

  Acts::Experimental::GeometryIdGenerator::Config generatorConfig;
  Acts::Experimental::GeometryIdGenerator generator(
      generatorConfig,
      Acts::getDefaultLogger("SequentialIdGenerator", Acts::Logging::VERBOSE));
  auto cache = generator.generateCache();
  for (auto& vol : volumes012) {
    generator.assignGeometryId(cache, *vol);
  }

  auto det012 = Acts::Experimental::Detector::makeShared(
      "Det012", volumes012, Acts::Experimental::tryRootVolumes());

  // Check the basic return functions
  BOOST_CHECK_EQUAL(det012->name(), "Det012");
  BOOST_CHECK_EQUAL(det012->volumes().size(), 3u);
  BOOST_CHECK_EQUAL(det012->volumePtrs().size(), 3u);

  // Check the shared pointer mechanism
  BOOST_CHECK_EQUAL(det012,
                    unpackToShared<Acts::Experimental::Detector>(*det012));
  BOOST_CHECK_EQUAL(
      det012, unpackToShared<const Acts::Experimental::Detector>(*det012));

  // Check surface visiting
  // Test the visitor pattern for surfaces
  std::size_t nSurfaces = 0;
  det012->visitSurfaces([&nSurfaces](const auto* s) {
    if (s != nullptr) {
      nSurfaces++;
    }
  });
  BOOST_CHECK_EQUAL(nSurfaces, 11u);

  // Check the volume visiting
  std::size_t nVolumes = 0;
  det012->visitVolumes([&nVolumes](const auto* v) {
    if (v != nullptr) {
      nVolumes++;
    }
  });
  BOOST_CHECK_EQUAL(nVolumes, 3u);

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
    void operator()(Acts::Experimental::DetectorVolume* v) {
      if (v != nullptr) {
        v->assignVolumeMaterial(volumeMaterial);
      }
    }
  };

  SetMaterial setMaterial;
  det012->visitMutableSurfaces(setMaterial);
  det012->visitMutableVolumes(setMaterial);

  // Count surfaces with material
  std::size_t nSurfacesWithMaterial = 0;
  det012->visitSurfaces([&nSurfacesWithMaterial](const auto* s) {
    if (s != nullptr && s->surfaceMaterial() != nullptr) {
      nSurfacesWithMaterial++;
    }
  });
  BOOST_CHECK_EQUAL(nSurfacesWithMaterial, 11u);

  // Count volumes with material
  std::size_t nVolumesWithMaterial = 0;

  det012->visitVolumes([&nVolumesWithMaterial](const auto* v) {
    if (v != nullptr && v->volumeMaterial() != nullptr) {
      nVolumesWithMaterial++;
    }
  });
  BOOST_CHECK_EQUAL(nVolumesWithMaterial, 3u);

  // Check the inside function with positions
  Acts::Experimental::NavigationState nState;
  nState.position = Acts::Vector3(5., 0., 0.);
  nState.currentDetector = det012.get();
  det012->updateDetectorVolume(tContext, nState);
  BOOST_CHECK_EQUAL(nState.currentVolume, cyl0.get());

  auto find1 = det012->findDetectorVolume(tContext, Acts::Vector3(15., 0., 0.));
  BOOST_CHECK_EQUAL(find1, cyl1.get());

  auto find2 =
      det012->findDetectorVolume(tContext, Acts::Vector3(150., 0., 0.));
  BOOST_CHECK_EQUAL(find2, cyl2.get());

  auto findNull =
      det012->findDetectorVolume(tContext, Acts::Vector3(1500., 0., 0.));
  BOOST_CHECK_EQUAL(findNull, nullptr);

  /// Find by name
  auto find0 = det012->findDetectorVolume("Cyl0");
  BOOST_CHECK_EQUAL(find0, cyl0.get());

  findNull = det012->findDetectorVolume("Null");
  BOOST_CHECK_EQUAL(findNull, nullptr);

  // Misconfigured - unkonnected finder
  Acts::Experimental::ExternalNavigationDelegate unconnected;
  BOOST_CHECK_THROW(
      Acts::Experimental::Detector::makeShared("Det012_unconnected", volumes012,
                                               std::move(unconnected)),
      std::invalid_argument);

  generator.assignGeometryId(cache, *cyl0nameDup);

  // Misconfigured - duplicate name
  std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> volumes002 =
      {cyl0, cyl0nameDup, cyl2};
  BOOST_CHECK_THROW(Acts::Experimental::Detector::makeShared(
                        "Det002_name_duplicate", volumes002,
                        Acts::Experimental::tryRootVolumes()),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(DetectorConstructionWithHierarchyMap) {
  auto portalGenerator = Acts::Experimental::defaultPortalGenerator();

  std::vector<std::unique_ptr<Acts::Test::DetectorElementStub>> detStore;
  std::vector<Acts::ActsScalar> radii = {100, 102, 104, 106, 108, 110};
  auto cylinderVoumeBounds =
      std::make_unique<Acts::CylinderVolumeBounds>(80, 130, 200);
  std::vector<std::shared_ptr<Acts::Surface>> surfaces = {};
  for (auto [ir, r] : Acts::enumerate(radii)) {
    auto detElement = std::make_unique<Acts::Test::DetectorElementStub>(
        Acts::Transform3::Identity(),
        std::make_shared<Acts::CylinderBounds>(r, 190.), 0.1);
    auto surface = detElement->surface().getSharedPtr();
    surface->assignGeometryId(Acts::GeometryIdentifier{}.setSensitive(ir + 1));
    surfaces.push_back(std::move(surface));
    detStore.push_back(std::move(detElement));
  }

  auto cylVolume = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "CylinderVolume", Acts::Transform3::Identity(),
      std::move(cylinderVoumeBounds), surfaces, {},
      Acts::Experimental::tryNoVolumes(),
      Acts::Experimental::tryAllPortalsAndSurfaces());

  Acts::Experimental::GeometryIdGenerator::Config generatorConfig;
  Acts::Experimental::GeometryIdGenerator generator(
      generatorConfig,
      Acts::getDefaultLogger("SequentialIdGenerator", Acts::Logging::VERBOSE));

  auto cache = generator.generateCache();
  generator.assignGeometryId(cache, *cylVolume);

  auto det = Acts::Experimental::Detector::makeShared(
      "DetWithSurfaces", {cylVolume}, Acts::Experimental::tryRootVolumes());

  const auto& sensitiveHierarchyMap = det->sensitiveHierarchyMap();

  const Acts::Surface* surface0 =
      det->findSurface(Acts::GeometryIdentifier{}.setSensitive(1));

  BOOST_CHECK_EQUAL(sensitiveHierarchyMap.size(), 6u);
  BOOST_CHECK_NE(surface0, nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
