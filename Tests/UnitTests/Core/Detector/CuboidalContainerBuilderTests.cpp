// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/CuboidalContainerBuilder.hpp"
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/ProtoBinning.hpp"
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
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <memory>
#include <stdexcept>

using namespace Acts::Experimental;

auto tContext = Acts::GeometryContext();

/// @brief A mockup volume builder, it generates volumes with
/// a single surface filled in in order to use the CuboidalContainerBuilder
/// infrastructure.
class CuboidalVolumeBuilder : public IDetectorComponentBuilder {
 public:
  CuboidalVolumeBuilder(const Acts::Transform3& transform,
                        const Acts::CuboidVolumeBounds& vBounds,
                        const Acts::RectangleBounds& sBounds,
                        const std::string& vName)
      : IDetectorComponentBuilder(),
        m_transform(transform),
        m_volumeBounds(vBounds),
        m_surfaceBounds(sBounds),
        m_name(vName) {}

  DetectorComponent construct(
      [[maybe_unused]] const Acts::GeometryContext& gctx) const final {
    // The outgoing root volumes
    std::vector<std::shared_ptr<DetectorVolume>> rootVolumes;

    // Ingredients
    auto surface = Acts::Surface::makeShared<Acts::PlaneSurface>(
        (m_transform),
        std::make_shared<Acts::RectangleBounds>(m_surfaceBounds));

    auto bounds = std::make_unique<Acts::CuboidVolumeBounds>(m_volumeBounds);
    auto portalGenerator = defaultPortalAndSubPortalGenerator();
    auto volume = DetectorVolumeFactory::construct(
        portalGenerator, tContext, m_name, m_transform, std::move(bounds),
        {surface}, {}, tryAllSubVolumes(), tryAllPortalsAndSurfaces());

    // Add to the roots
    rootVolumes.push_back(volume);

    DetectorComponent::PortalContainer dContainer;
    for (auto [ip, p] : Acts::enumerate(volume->portalPtrs())) {
      dContainer[ip] = p;
    }

    return DetectorComponent{
        {volume},
        dContainer,
        RootDetectorVolumes{rootVolumes, tryRootVolumes()}};
  }

 private:
  Acts::Transform3 m_transform;
  Acts::CuboidVolumeBounds m_volumeBounds;
  Acts::RectangleBounds m_surfaceBounds;
  std::string m_name;
};

class VolumeGeoIdGenerator : public IGeometryIdGenerator {
 public:
  struct Cache {
    unsigned int volumeCount = 0;
  };

  IGeometryIdGenerator::GeoIdCache generateCache() const final {
    return Cache{0};
  }

  void assignGeometryId(IGeometryIdGenerator::GeoIdCache& cache,
                        DetectorVolume& dVolume) const final {
    auto& ccache = std::any_cast<Cache&>(cache);
    ccache.volumeCount += 1;
    Acts::GeometryIdentifier geoID;
    geoID.setVolume(ccache.volumeCount);
    dVolume.assignGeometryId(geoID);
  }

  void assignGeometryId(
      Acts::Experimental::IGeometryIdGenerator::GeoIdCache& /*cache*/,
      Acts::Experimental::Portal& /*portal*/) const final {}

  void assignGeometryId(
      Acts::Experimental::IGeometryIdGenerator::GeoIdCache& /*cache*/,
      Acts::Surface& /*surface*/) const final {}
};

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(CuboidalContainerBuilder_Misconfiguration) {
  // misconfiruation: no builders
  CuboidalContainerBuilder::Config misCfg;
  BOOST_CHECK_THROW(auto a = CuboidalContainerBuilder(misCfg),
                    std::invalid_argument);
  // misconfiguration - 1D binning not in x, y, z
  misCfg.builders = {nullptr};
  misCfg.binning = Acts::BinningValue::binR;
  BOOST_CHECK_THROW(auto b = CuboidalContainerBuilder(misCfg),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(CuboidalContainerBuildingXYZVolumes) {
  std::array<Acts::BinningValue, 3> binningValues = {Acts::BinningValue::binX,
                                                     Acts::BinningValue::binY,
                                                     Acts::BinningValue::binZ};
  for (auto bVal : binningValues) {
    // A perfect box shape
    auto box = Acts::CuboidVolumeBounds(10, 10, 10);

    // Transform the second volume away from the first one
    auto transformB = Acts::Transform3::Identity();

    Acts::Vector3 translation = Acts::Vector3::Zero();
    translation[toUnderlying(bVal)] = 20;
    transformB.pretranslate(translation);

    auto builderA = std::make_shared<CuboidalVolumeBuilder>(
        Acts::Transform3::Identity(), box, Acts::RectangleBounds(2, 2),
        "VolumeA");

    auto builderB = std::make_shared<CuboidalVolumeBuilder>(
        transformB, box, Acts::RectangleBounds(2, 2), "VolumeB");

    CuboidalContainerBuilder::Config ccbCfg;
    ccbCfg.auxiliary = "*** Build simple connection ***";
    ccbCfg.builders = {builderA, builderB};
    ccbCfg.binning = bVal;
    ccbCfg.geoIdGenerator = std::make_shared<VolumeGeoIdGenerator>();

    auto ccBuilder = std::make_shared<CuboidalContainerBuilder>(
        ccbCfg, Acts::getDefaultLogger("CuboidalContainerBuilder",
                                       Acts::Logging::VERBOSE));

    auto [volumes, portals, roots] = ccBuilder->construct(tContext);

    BOOST_CHECK_EQUAL(portals.size(), 4u);
    BOOST_CHECK_EQUAL(roots.volumes.size(), 2u);
    BOOST_CHECK_EQUAL(roots.volumes.at(0)->geometryId().volume(), 1u);
    BOOST_CHECK_EQUAL(roots.volumes.at(1)->geometryId().volume(), 2u);
    BOOST_CHECK_EQUAL(roots.volumes.at(0)
                          ->transform(tContext)
                          .translation()[toUnderlying(bVal)],
                      0);
    BOOST_CHECK_EQUAL(roots.volumes.at(1)
                          ->transform(tContext)
                          .translation()[toUnderlying(bVal)],
                      20);

    for (auto& portal : portals) {
      if (portal.second->attachedDetectorVolumes().at(0).empty()) {
        BOOST_CHECK_EQUAL(portal.second->attachedDetectorVolumes().at(1).size(),
                          2u);
      } else {
        BOOST_CHECK_EQUAL(portal.second->attachedDetectorVolumes().at(0).size(),
                          2u);
      }
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
