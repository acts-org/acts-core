// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/DetectorVolumeBuilder.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/interface/IExternalStructureBuilder.hpp"
#include "Acts/Detector/interface/IGeometryIdGenerator.hpp"
#include "Acts/Detector/interface/IInternalStructureBuilder.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace Acts;
using namespace Acts::Experimental;

GeometryContext tContext;

/// @brief Mockup external structure builder
/// @tparam bounds_type the volume bounds type that is constructed
template <typename bounds_type>
class ExternalsBuilder : public IExternalStructureBuilder {
 public:
  ExternalsBuilder(const Transform3& transform, const bounds_type& bounds)
      : IExternalStructureBuilder(),
        m_transform(transform),
        m_bounds(std::move(bounds)) {}

  ExternalStructure construct(
      [[maybe_unused]] const GeometryContext& gctx) const final {
    return {m_transform, std::make_unique<bounds_type>(m_bounds),
            defaultPortalGenerator()};
  }

 private:
  Transform3 m_transform = Transform3::Identity();
  bounds_type m_bounds;
};

/// @brief  Mockup internal surface builder
/// @tparam surface_type the surface type to be constructed
/// @tparam bounds_type the bounds type that is constructed
template <typename surface_type, typename bounds_type>
class InternalSurfaceBuilder : public IInternalStructureBuilder {
 public:
  InternalSurfaceBuilder(const Transform3& transform, const bounds_type& bounds)
      : IInternalStructureBuilder(),
        m_transform(transform),
        m_bounds(std::move(bounds)) {}

  InternalStructure construct(
      [[maybe_unused]] const GeometryContext& gctx) const final {
    auto surface = Surface::makeShared<surface_type>(
        m_transform, std::make_shared<bounds_type>(m_bounds));
    return {{surface}, {}, tryAllPortalsAndSurfaces(), tryNoVolumes()};
  }

 private:
  Transform3 m_transform = Transform3::Identity();
  bounds_type m_bounds;
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
      geoID.setPassive(is + 1);
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

/// @brief  Mockup internal surface builder
/// @tparam surface_type the surface type to be constructed
/// @tparam bounds_type the bounds type that is constructed
template <typename bounds_type>
class InternalVolumeBuilder : public IInternalStructureBuilder {
 public:
  InternalVolumeBuilder(const Transform3& transform, const bounds_type& bounds)
      : IInternalStructureBuilder(),
        m_transform(transform),
        m_bounds(std::move(bounds)) {}

  InternalStructure construct(
      [[maybe_unused]] const GeometryContext& gctx) const final {
    auto bounds = std::make_unique<bounds_type>(m_bounds);
    auto portalGenerator = defaultPortalGenerator();
    auto volume = DetectorVolumeFactory::construct(
        portalGenerator, tContext, "InternalVolume", m_transform,
        std::move(bounds), tryAllPortals());
    return {{}, {volume}, tryAllPortals(), tryRootVolumes()};
  }

 private:
  Transform3 m_transform = Transform3::Identity();
  bounds_type m_bounds;
};

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(DetectorVolumeBuilder_Misconfigured) {
  // Internal and external structure builder is empty
  DetectorVolumeBuilder::Config dvCfg;
  dvCfg.auxiliary = "*** Test X * Misconfigued ***";
  dvCfg.name = "EmptyCylinder";
  dvCfg.externalsBuilder = nullptr;
  dvCfg.internalsBuilder = nullptr;

  BOOST_CHECK_THROW(auto a = DetectorVolumeBuilder(dvCfg),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(DetectorVolumeBuilder_EmptyVolume) {
  CylinderVolumeBounds cBounds(10, 100, 200);
  auto cBuilder = std::make_shared<ExternalsBuilder<CylinderVolumeBounds>>(
      Transform3::Identity(), cBounds);

  DetectorVolumeBuilder::Config dvCfg;
  dvCfg.auxiliary = "*** Test 0 - Empty Cylinder ***";
  dvCfg.name = "EmptyCylinder";
  dvCfg.externalsBuilder = cBuilder;
  dvCfg.internalsBuilder = nullptr;

  // Assign proto material to
  dvCfg.portalMaterialBinning[2u] = BinningDescription{
      {ProtoBinning(BinningValue::binZ, Acts::AxisBoundaryType::Bound, 50),
       ProtoBinning(BinningValue::binPhi, Acts::AxisBoundaryType::Closed, -M_PI,
                    M_PI, 12)}};

  auto dvBuilder = std::make_shared<DetectorVolumeBuilder>(
      dvCfg, getDefaultLogger("DetectorVolumeBuilder", Logging::VERBOSE));

  auto [volumes, portals, roots] = dvBuilder->construct(tContext);

  BOOST_CHECK_EQUAL(volumes.size(), 1u);
  BOOST_CHECK(volumes.front()->surfaces().empty());
  BOOST_CHECK(volumes.front()->volumes().empty());

  BOOST_CHECK_EQUAL(portals.size(), 4u);

  BOOST_CHECK_EQUAL(roots.volumes.size(), 1u);
  BOOST_CHECK(roots.volumeFinder.connected());

  // Check that the outside portal has material
  BOOST_CHECK_NE(portals[2u]->surface().surfaceMaterial(), nullptr);
  // While all the others have none
  BOOST_CHECK_EQUAL(portals[0u]->surface().surfaceMaterial(), nullptr);
  BOOST_CHECK_EQUAL(portals[1u]->surface().surfaceMaterial(), nullptr);
  BOOST_CHECK_EQUAL(portals[3u]->surface().surfaceMaterial(), nullptr);
}

BOOST_AUTO_TEST_CASE(DetectorVolumeBuilder_VolumeWithSurface) {
  CylinderVolumeBounds cvBounds(10, 100, 200);
  auto cBuilder = std::make_shared<ExternalsBuilder<CylinderVolumeBounds>>(
      Transform3::Identity(), cvBounds);

  CylinderBounds csBounds(55., 195.);
  auto sBuilder =
      std::make_shared<InternalSurfaceBuilder<CylinderSurface, CylinderBounds>>(
          Transform3::Identity(), csBounds);

  DetectorVolumeBuilder::Config dvCfg;
  dvCfg.auxiliary = "*** Test 1 - Cylinder with internal Surface ***";
  dvCfg.name = "CylinderWithSurface";
  dvCfg.externalsBuilder = cBuilder;
  dvCfg.internalsBuilder = sBuilder;
  dvCfg.geoIdGenerator = std::make_shared<SurfaceGeoIdGenerator>();

  auto dvBuilder = std::make_shared<DetectorVolumeBuilder>(
      dvCfg, getDefaultLogger("DetectorVolumeBuilder", Logging::VERBOSE));

  auto [volumes, portals, roots] = dvBuilder->construct(tContext);

  BOOST_CHECK_EQUAL(volumes.size(), 1u);
  BOOST_CHECK_EQUAL(volumes.front()->surfaces().size(), 1u);

  BOOST_CHECK_EQUAL(volumes.front()->surfaces().front()->geometryId().passive(),
                    1u);
  BOOST_CHECK(volumes.front()->volumes().empty());

  BOOST_CHECK_EQUAL(portals.size(), 4u);

  BOOST_CHECK_EQUAL(roots.volumes.size(), 1u);
  BOOST_CHECK(roots.volumeFinder.connected());
}

BOOST_AUTO_TEST_CASE(DetectorVolumeBuilder_VolumeWithVolume) {
  CylinderVolumeBounds cvBounds(10, 100, 200);
  auto cBuilder = std::make_shared<ExternalsBuilder<CylinderVolumeBounds>>(
      Transform3::Identity(), cvBounds);

  CylinderVolumeBounds ciBounds(15., 95., 195.);
  auto iBuilder = std::make_shared<InternalVolumeBuilder<CylinderVolumeBounds>>(
      Transform3::Identity(), ciBounds);

  DetectorVolumeBuilder::Config dvCfg;
  dvCfg.auxiliary = "*** Test 2 - Cylinder with internal Volume ***";
  dvCfg.name = "CylinderWithVolume";
  dvCfg.externalsBuilder = cBuilder;
  dvCfg.internalsBuilder = iBuilder;
  dvCfg.addInternalsToRoot = false;

  auto dvBuilder = std::make_shared<DetectorVolumeBuilder>(
      dvCfg, getDefaultLogger("DetectorVolumeBuilder", Logging::VERBOSE));

  auto [volumes, portals, roots] = dvBuilder->construct(tContext);

  BOOST_CHECK_EQUAL(volumes.size(), 1u);
  BOOST_CHECK_EQUAL(portals.size(), 4u);
  BOOST_CHECK_EQUAL(roots.volumes.size(), 1u);
}

BOOST_AUTO_TEST_CASE(DetectorVolumeBuilder_VolumeWithVolumeToRoot) {
  CylinderVolumeBounds cvBounds(10, 100, 200);
  auto cBuilder = std::make_shared<ExternalsBuilder<CylinderVolumeBounds>>(
      Transform3::Identity(), cvBounds);

  CylinderVolumeBounds ciBounds(15., 95., 195.);
  auto iBuilder = std::make_shared<InternalVolumeBuilder<CylinderVolumeBounds>>(
      Transform3::Identity(), ciBounds);

  DetectorVolumeBuilder::Config dvCfg;
  dvCfg.auxiliary =
      "*** Test 3 - Cylinder with internal Volume, adding to root ***";
  dvCfg.name = "CylinderWithVolume";
  dvCfg.externalsBuilder = cBuilder;
  dvCfg.internalsBuilder = iBuilder;
  dvCfg.addInternalsToRoot = true;

  auto dvBuilder = std::make_shared<DetectorVolumeBuilder>(
      dvCfg, getDefaultLogger("DetectorVolumeBuilder", Logging::VERBOSE));

  auto [volumes, portals, roots] = dvBuilder->construct(tContext);

  BOOST_CHECK_EQUAL(volumes.size(), 1u);
  BOOST_CHECK(volumes.front()->surfaces().empty());
  BOOST_CHECK_EQUAL(volumes.front()->volumes().size(), 1u);

  BOOST_CHECK_EQUAL(portals.size(), 4u);

  BOOST_CHECK_EQUAL(roots.volumes.size(), 2u);
  BOOST_CHECK(roots.volumeFinder.connected());
}

BOOST_AUTO_TEST_SUITE_END()
