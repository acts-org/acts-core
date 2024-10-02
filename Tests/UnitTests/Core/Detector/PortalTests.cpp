// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <array>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Acts::Experimental {

/// a simple link to volume struct
class LinkToVolumeImpl : public IExternalNavigation {
 public:
  std::shared_ptr<DetectorVolume> dVolume = nullptr;

  /// Constructor from volume
  LinkToVolumeImpl(std::shared_ptr<DetectorVolume> dv)
      : dVolume(std::move(dv)) {}

  /// @return the link to the contained volume
  /// @note the parameters are ignored
  void link(const GeometryContext& /*gctx*/, NavigationState& nState) const {
    nState.currentVolume = dVolume.get();
  }
};

}  // namespace Acts::Experimental

using namespace Acts::Experimental;

// A test context
Acts::GeometryContext tContext;

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(PortalTest) {
  auto dTransform = Acts::Transform3::Identity();
  auto pGenerator = defaultPortalGenerator();
  auto volumeA = DetectorVolumeFactory::construct(
      pGenerator, tContext, "dummyA", dTransform,
      std::make_unique<Acts::CuboidVolumeBounds>(1, 1, 1),
      tryAllPortalsAndSurfaces());
  auto volumeB = DetectorVolumeFactory::construct(
      pGenerator, tContext, "dummyB", dTransform,
      std::make_unique<Acts::CuboidVolumeBounds>(1, 1, 1),
      tryAllPortalsAndSurfaces());

  // A rectangle bound surface
  auto rectangle = std::make_shared<Acts::RectangleBounds>(10., 100.);
  auto surface =
      Acts::Surface::makeShared<Acts::PlaneSurface>(dTransform, rectangle);

  // Create a portal out of it
  auto portalA = std::make_shared<Portal>(surface);

  BOOST_CHECK_EQUAL(&(portalA->surface()), surface.get());

  portalA->assignGeometryId(Acts::GeometryIdentifier{5});
  BOOST_CHECK_EQUAL(portalA->surface().geometryId(),
                    Acts::GeometryIdentifier{5});

  // Create a links to volumes
  auto linkToAImpl = std::make_unique<const LinkToVolumeImpl>(volumeA);
  ExternalNavigationDelegate linkToA;
  linkToA.connect<&LinkToVolumeImpl::link>(std::move(linkToAImpl));
  portalA->assignPortalNavigation(Acts::Direction::Positive, std::move(linkToA),
                                  {volumeA});

  auto attachedDetectorVolumes = portalA->attachedDetectorVolumes();
  BOOST_CHECK(attachedDetectorVolumes[0u].empty());
  BOOST_CHECK_EQUAL(attachedDetectorVolumes[1u].size(), 1u);
  BOOST_CHECK_EQUAL(attachedDetectorVolumes[1u][0u], volumeA);

  NavigationState nState;
  nState.position = Acts::Vector3(0., 0., 0.);
  nState.direction = Acts::Vector3(0., 0., 1.);
  // The next volume in positive should be volume A
  portalA->updateDetectorVolume(tContext, nState);
  BOOST_CHECK_EQUAL(nState.currentVolume, volumeA.get());
  // negative should yield nullptr
  nState.direction = Acts::Vector3(0., 0., -1.);
  portalA->updateDetectorVolume(tContext, nState);
  BOOST_CHECK_EQUAL(nState.currentVolume, nullptr);

  auto portalB = std::make_shared<Portal>(surface);
  ExternalNavigationDelegate linkToB;
  auto linkToBImpl = std::make_unique<const LinkToVolumeImpl>(volumeB);
  linkToB.connect<&LinkToVolumeImpl::link>(std::move(linkToBImpl));
  portalB->assignPortalNavigation(Acts::Direction::Negative, std::move(linkToB),
                                  {volumeB});

  // Reverse: positive volume nullptr, negative volume volumeB
  nState.direction = Acts::Vector3(0., 0., 1.);
  portalB->updateDetectorVolume(tContext, nState);
  BOOST_CHECK_EQUAL(nState.currentVolume, nullptr);
  nState.direction = Acts::Vector3(0., 0., -1.);
  portalB->updateDetectorVolume(tContext, nState);
  BOOST_CHECK_EQUAL(nState.currentVolume, volumeB.get());

  Acts::GeometryContext gctx;
  BOOST_CHECK_EQUAL(portalA->surface().center(gctx),
                    portalB->surface().center(gctx));

  // Fuse with itself, nothing happens
  BOOST_CHECK_EQUAL(portalA, Portal::fuse(portalA, portalA));

  // Now fuse the portals together, both links valid
  portalA = Portal::fuse(portalA, portalB);

  nState.direction = Acts::Vector3(0., 0., 1.);
  portalA->updateDetectorVolume(tContext, nState);
  BOOST_CHECK_EQUAL(nState.currentVolume, volumeA.get());
  nState.direction = Acts::Vector3(0., 0., -1.);
  portalA->updateDetectorVolume(tContext, nState);
  BOOST_CHECK_EQUAL(nState.currentVolume, volumeB.get());

  // Portal A retains identical position to B
  BOOST_CHECK_EQUAL(portalA->surface().center(gctx),
                    portalB->surface().center(gctx));

  // Test visitor pattern - const access
  bool reached = false;
  const Portal* cportalB = portalB.get();
  cportalB->visitSurface([&reached](const auto* s) {
    if (s != nullptr) {
      reached = true;
    }
  });
  BOOST_CHECK(reached);

  // Test visitor pattern - non-const access
  struct SetMaterial {
    /// The material to set
    std::shared_ptr<const Acts::HomogeneousSurfaceMaterial> material =
        std::make_shared<Acts::HomogeneousSurfaceMaterial>(Acts::MaterialSlab(
            Acts::Material::fromMolarDensity(1., 2., 3., 4., 5.), 1.));
    /// The visitor call
    void operator()(Acts::Surface* s) {
      if (s != nullptr) {
        s->assignSurfaceMaterial(material);
      }
    }
  };

  SetMaterial setMaterial;
  BOOST_CHECK(portalA->surface().surfaceMaterial() == nullptr);
  portalA->visitMutableSurface(setMaterial);
  BOOST_CHECK(portalA->surface().surfaceMaterial() ==
              setMaterial.material.get());
}

BOOST_AUTO_TEST_CASE(PortalMaterialTest) {
  // Volume A and B
  auto dTransform = Acts::Transform3::Identity();
  auto pGenerator = defaultPortalGenerator();
  auto volumeA = DetectorVolumeFactory::construct(
      pGenerator, tContext, "dummyA", dTransform,
      std::make_unique<Acts::CuboidVolumeBounds>(1, 1, 1),
      tryAllPortalsAndSurfaces());
  auto volumeB = DetectorVolumeFactory::construct(
      pGenerator, tContext, "dummyB", dTransform,
      std::make_unique<Acts::CuboidVolumeBounds>(1, 1, 1),
      tryAllPortalsAndSurfaces());

  // Create some material
  auto materialSlab = Acts::MaterialSlab(
      Acts::Material::fromMolarDensity(1., 2., 3., 4., 5.), 1.);
  auto materialA =
      std::make_shared<Acts::HomogeneousSurfaceMaterial>(materialSlab);
  auto materialB =
      std::make_shared<Acts::HomogeneousSurfaceMaterial>(materialSlab);

  // A few portals
  auto rectangle = std::make_shared<Acts::RectangleBounds>(10., 100.);

  auto surfaceA = Acts::Surface::makeShared<Acts::PlaneSurface>(
      Acts::Transform3::Identity(), rectangle);
  surfaceA->assignSurfaceMaterial(materialA);
  auto portalA = std::make_shared<Portal>(surfaceA);

  ExternalNavigationDelegate linkToA;
  auto linkToAImpl = std::make_unique<const LinkToVolumeImpl>(volumeA);
  linkToA.connect<&LinkToVolumeImpl::link>(std::move(linkToAImpl));
  portalA->assignPortalNavigation(Acts::Direction::Positive, std::move(linkToA),
                                  {volumeA});

  auto surfaceB = Acts::Surface::makeShared<Acts::PlaneSurface>(
      Acts::Transform3::Identity(), rectangle);
  auto portalB = std::make_shared<Portal>(surfaceB);
  ExternalNavigationDelegate linkToB;
  auto linkToBImpl = std::make_unique<const LinkToVolumeImpl>(volumeB);
  linkToB.connect<&LinkToVolumeImpl::link>(std::move(linkToBImpl));
  portalB->assignPortalNavigation(Acts::Direction::Negative, std::move(linkToB),
                                  {volumeB});

  // Portal A fuses with B
  // - has material and keeps it
  portalA = Portal::fuse(portalA, portalB);
  BOOST_CHECK_EQUAL(portalA->surface().surfaceMaterial(), materialA.get());

  // Remake portal B
  portalB = std::make_shared<Portal>(surfaceB);
  ExternalNavigationDelegate linkToB2;
  auto linkToB2Impl = std::make_unique<const LinkToVolumeImpl>(volumeB);
  linkToB2.connect<&LinkToVolumeImpl::link>(std::move(linkToB2Impl));
  portalB->assignPortalNavigation(Acts::Direction::Negative,
                                  std::move(linkToB2), {volumeB});

  // Portal B fuses with A
  // - A has material, portal B gets it from A
  BOOST_REQUIRE_NE(portalA, portalB);

  // This fails because A has accumulated volumes on both sides through fusing
  BOOST_CHECK_THROW(Portal::fuse(portalB, portalA), std::invalid_argument);
  // Remove Negative volume on A
  portalA->assignPortalNavigation(Acts::Direction::Negative,
                                  ExternalNavigationDelegate{}, {});

  portalB = Portal::fuse(portalB, portalA);
  BOOST_CHECK_EQUAL(portalB->surface().surfaceMaterial(), materialA.get());

  // Remake portal A and B, this time both with material
  portalA = std::make_shared<Portal>(surfaceA);
  ExternalNavigationDelegate linkToA2;
  auto linkToA2Impl = std::make_unique<const LinkToVolumeImpl>(volumeA);
  linkToA2.connect<&LinkToVolumeImpl::link>(std::move(linkToA2Impl));
  portalA->assignPortalNavigation(Acts::Direction::Positive,
                                  std::move(linkToA2), {volumeA});

  surfaceB->assignSurfaceMaterial(materialB);
  portalB = std::make_shared<Portal>(surfaceB);
  ExternalNavigationDelegate linkToB3;
  auto linkToB3Impl = std::make_unique<const LinkToVolumeImpl>(volumeB);
  linkToB3.connect<&LinkToVolumeImpl::link>(std::move(linkToB3Impl));
  portalB->assignPortalNavigation(Acts::Direction::Negative,
                                  std::move(linkToB3), {volumeB});

  // Portal A fuses with B - both have material, throw exception
  BOOST_CHECK_THROW(Portal::fuse(portalA, portalB), std::runtime_error);
  // Same in reverse
  BOOST_CHECK_THROW(Portal::fuse(portalB, portalA), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
