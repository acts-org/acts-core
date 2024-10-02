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
#include "Acts/Detector/DetectorBuilder.hpp"
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/interface/IDetectorComponentBuilder.hpp"
#include "Acts/Detector/interface/IGeometryIdGenerator.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <memory>
#include <stdexcept>
#include <vector>

class CompBuilder final : public Acts::Experimental::IDetectorComponentBuilder {
 public:
  CompBuilder(
      const std::vector<std::shared_ptr<Acts::Surface>>& sensitives = {})
      : m_sensitives(sensitives) {}

  Acts::Experimental::DetectorComponent construct(
      const Acts::GeometryContext& gctx) const final {
    auto bounds = std::make_unique<Acts::CuboidVolumeBounds>(10., 10., 10.);
    // Construct the DetectorVolume
    auto dVolume =
        m_sensitives.empty()
            ? Acts::Experimental::DetectorVolumeFactory::construct(
                  Acts::Experimental::defaultPortalGenerator(), gctx,
                  "TestVolume", Acts::Transform3::Identity(), std::move(bounds),
                  Acts::Experimental::tryAllPortals())
            : Acts::Experimental::DetectorVolumeFactory::construct(
                  Acts::Experimental::defaultPortalGenerator(), gctx,
                  "TestVolumeWithSurfaces", Acts::Transform3::Identity(),
                  std::move(bounds), m_sensitives, {},
                  Acts::Experimental::tryNoVolumes(),
                  Acts::Experimental::tryAllPortalsAndSurfaces());

    // Fill the portal container
    Acts::Experimental::DetectorComponent::PortalContainer portalContainer;
    for (auto [ip, p] : Acts::enumerate(dVolume->portalPtrs())) {
      portalContainer[ip] = p;
    }

    Acts::GeometryIdentifier geoID;
    geoID.setVolume(1);
    dVolume->assignGeometryId(geoID);

    return Acts::Experimental::DetectorComponent{
        {dVolume},
        portalContainer,
        {{dVolume}, Acts::Experimental::tryRootVolumes()}};
  }

 private:
  std::vector<std::shared_ptr<Acts::Surface>> m_sensitives;
};

class SurfaceGeoIdGenerator : public Acts::Experimental::IGeometryIdGenerator {
 public:
  Acts::Experimental::IGeometryIdGenerator::GeoIdCache generateCache()
      const final {
    return std::any();
  }

  void assignGeometryId(
      Acts::Experimental::IGeometryIdGenerator::GeoIdCache& /*cache*/,
      Acts::Experimental::DetectorVolume& dVolume) const final {
    for (auto [is, s] : Acts::enumerate(dVolume.surfacePtrs())) {
      Acts::GeometryIdentifier geoID;
      geoID.setSensitive(is + 1);
      s->assignGeometryId(geoID);
    }
  }

  void assignGeometryId(
      Acts::Experimental::IGeometryIdGenerator::GeoIdCache& /*cache*/,
      Acts::Experimental::Portal& /*portal*/) const final {}

  void assignGeometryId(
      Acts::Experimental::IGeometryIdGenerator::GeoIdCache& /*cache*/,
      Acts::Surface& /*surface*/) const final {}
};

Acts::GeometryContext tContext;

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(DetectorBuilder_Misconfigured) {
  // Detector builder
  Acts::Experimental::DetectorBuilder::Config dCfg;
  dCfg.auxiliary = "*** Test X * Misconfigued ***";
  dCfg.name = "EmptyCylinder";
  dCfg.builder = nullptr;

  BOOST_CHECK_THROW(auto a = Acts::Experimental::DetectorBuilder(dCfg),
                    std::invalid_argument);

  // Detector builder with sensitives but no assigned geometry ids to them
  Acts::Test::DetectorElementStub detElement0(
      Acts::Transform3::Identity(),
      std::make_shared<Acts::RectangleBounds>(5., 5.), 0.1);
  Acts::Test::DetectorElementStub detElement1(
      Acts::Transform3::Identity(),
      std::make_shared<Acts::RectangleBounds>(5., 5.), 0.1);

  std::vector<std::shared_ptr<Acts::Surface>> sensitives;
  sensitives.push_back(detElement0.surface().getSharedPtr());
  sensitives.push_back(detElement1.surface().getSharedPtr());
  dCfg.builder = std::make_shared<CompBuilder>(sensitives);

  BOOST_CHECK_THROW(
      Acts::Experimental::DetectorBuilder(dCfg).construct(tContext),
      std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(DetectorBuilder_test) {
  // Detector builder
  Acts::Experimental::DetectorBuilder::Config dCfg;
  dCfg.auxiliary = "*** Test : Detector ***";
  dCfg.name = "TestDetector";
  dCfg.builder = std::make_shared<CompBuilder>();

  BOOST_CHECK_NO_THROW(auto a = Acts::Experimental::DetectorBuilder(dCfg));

  auto detector = Acts::Experimental::DetectorBuilder(dCfg).construct(tContext);

  BOOST_CHECK_EQUAL(detector->name(), "TestDetector");
  BOOST_CHECK_EQUAL(detector->rootVolumes().size(), 1);
}

BOOST_AUTO_TEST_CASE(DetectorBuilder_testWithSurfaces) {
  // Detector builder
  Acts::Experimental::DetectorBuilder::Config dCfg;
  dCfg.auxiliary = "*** Test : Detector ***";
  dCfg.name = "TestDetectorWithSurfaces";
  dCfg.builder = std::make_shared<CompBuilder>();

  // Test detector with surfaces
  Acts::Test::DetectorElementStub detElement0(
      Acts::Transform3::Identity(),
      std::make_shared<Acts::RectangleBounds>(5., 5.), 0.1);
  Acts::Test::DetectorElementStub detElement1(
      Acts::Transform3::Identity(),
      std::make_shared<Acts::RectangleBounds>(5., 5.), 0.1);

  std::vector<std::shared_ptr<Acts::Surface>> sensitives;
  sensitives.push_back(detElement0.surface().getSharedPtr());
  sensitives.push_back(detElement1.surface().getSharedPtr());
  dCfg.builder = std::make_shared<CompBuilder>(sensitives);
  dCfg.geoIdGenerator = std::make_shared<SurfaceGeoIdGenerator>();

  auto detector = Acts::Experimental::DetectorBuilder(dCfg).construct(tContext);
  BOOST_CHECK_EQUAL(detector->name(), "TestDetectorWithSurfaces");
  BOOST_CHECK_EQUAL(
      detector->volumes()[0]->surfaces()[0]->geometryId().sensitive(), 1);
  BOOST_CHECK_EQUAL(
      detector->volumes()[0]->surfaces()[1]->geometryId().sensitive(), 2);
}

BOOST_AUTO_TEST_SUITE_END()
