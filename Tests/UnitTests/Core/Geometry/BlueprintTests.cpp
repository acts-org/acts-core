// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/Blueprint.hpp"
#include "Acts/Geometry/BlueprintNode.hpp"
#include "Acts/Geometry/CylinderContainerBlueprintNode.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeStack.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerBlueprintNode.hpp"
#include "Acts/Geometry/MaterialDesignatorBlueprintNode.hpp"
#include "Acts/Geometry/StaticBlueprintNode.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/VolumeAttachmentStrategy.hpp"
#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/ProtoAxis.hpp"

#include <fstream>
#include <stdexcept>
#include <vector>

using namespace Acts::UnitLiterals;

using Acts::Experimental::Blueprint;
using Acts::Experimental::BlueprintNode;
using Acts::Experimental::BlueprintOptions;
using Acts::Experimental::LayerBlueprintNode;
using Acts::Experimental::StaticBlueprintNode;

namespace Acts::Test {

auto logger = Acts::getDefaultLogger("UnitTests", Acts::Logging::INFO);

GeometryContext gctx;

namespace {

auto nameLookup(const TrackingGeometry& geo) {
  return [&](const std::string& name) -> const TrackingVolume& {
    const TrackingVolume* volume = nullptr;

    geo.visitVolumes([&](const TrackingVolume* v) {
      if (v->volumeName() == name) {
        volume = v;
      }
    });

    if (volume == nullptr) {
      throw std::runtime_error("Volume not found: " + name);
    }
    return *volume;
  };
}

std::size_t countVolumes(const TrackingGeometry& geo) {
  std::size_t nVolumes = 0;
  geo.visitVolumes([&](const TrackingVolume* /*volume*/) { nVolumes++; });
  return nVolumes;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(Geometry);

BOOST_AUTO_TEST_SUITE(BlueprintNodeTest);

BOOST_AUTO_TEST_CASE(InvalidRoot) {
  Logging::ScopedFailureThreshold threshold{Logging::Level::FATAL};

  Blueprint::Config cfg;
  Blueprint root{cfg};
  BOOST_CHECK_THROW(root.construct({}, gctx, *logger), std::logic_error);

  // More than one child is also invalid
  auto cylBounds = std::make_shared<CylinderVolumeBounds>(10_mm, 20_mm, 100_mm);
  root.addChild(
      std::make_unique<StaticBlueprintNode>(std::make_unique<TrackingVolume>(
          Transform3::Identity(), cylBounds, "child1")));
  root.addChild(
      std::make_unique<StaticBlueprintNode>(std::make_unique<TrackingVolume>(
          Transform3::Identity(), cylBounds, "child2")));

  BOOST_CHECK_THROW(root.construct({}, gctx, *logger), std::logic_error);
}

class DummyNode : public BlueprintNode {
 public:
  explicit DummyNode(const std::string& name) : m_name(name) {}

  const std::string& name() const override { return m_name; }

  Volume& build(const BlueprintOptions& /*options*/,
                const GeometryContext& /*gctx*/,
                const Acts::Logger& /*logger*/) override {
    throw std::logic_error("Not implemented");
  }

  PortalShellBase& connect(const BlueprintOptions& /*options*/,
                           const GeometryContext& /*gctx*/,
                           const Logger& /*logger */) override {
    throw std::logic_error("Not implemented");
  }

  void finalize(const BlueprintOptions& /*options*/,
                const GeometryContext& /*gctx*/, TrackingVolume& /*parent*/,
                const Logger& /*logger*/) override {
    throw std::logic_error("Not implemented");
  }

 private:
  std::string m_name;
};

BOOST_AUTO_TEST_CASE(RootCannotBeChild) {
  auto node = std::make_shared<DummyNode>("node");
  Blueprint::Config cfg;
  auto root = std::make_shared<Blueprint>(cfg);

  BOOST_CHECK_THROW(node->addChild(root), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(AddChildInvalid) {
  auto node = std::make_shared<DummyNode>("node");

  // Add self
  BOOST_CHECK_THROW(node->addChild(node), std::invalid_argument);

  // Add nullptr
  BOOST_CHECK_THROW(node->addChild(nullptr), std::invalid_argument);

  auto nodeB = std::make_shared<DummyNode>("nodeB");
  auto nodeC = std::make_shared<DummyNode>("nodeC");

  node->addChild(nodeB);
  nodeB->addChild(nodeC);
  BOOST_CHECK_THROW(nodeC->addChild(node), std::invalid_argument);

  // already has parent, can't be added as a child anywhere else
  BOOST_CHECK_THROW(node->addChild(nodeC), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Depth) {
  auto node1 = std::make_shared<DummyNode>("node1");
  auto node2 = std::make_shared<DummyNode>("node2");
  auto node3 = std::make_shared<DummyNode>("node3");

  BOOST_CHECK_EQUAL(node1->depth(), 0);
  BOOST_CHECK_EQUAL(node2->depth(), 0);
  BOOST_CHECK_EQUAL(node3->depth(), 0);

  node2->addChild(node3);
  BOOST_CHECK_EQUAL(node2->depth(), 0);
  BOOST_CHECK_EQUAL(node3->depth(), 1);

  node1->addChild(node2);
  BOOST_CHECK_EQUAL(node1->depth(), 0);
  BOOST_CHECK_EQUAL(node2->depth(), 1);
  BOOST_CHECK_EQUAL(node3->depth(), 2);
}

BOOST_AUTO_TEST_CASE(Static) {
  Blueprint::Config cfg;
  cfg.envelope[AxisDirection::AxisZ] = {20_mm, 20_mm};
  cfg.envelope[AxisDirection::AxisR] = {1_mm, 2_mm};
  Blueprint root{cfg};

  double hlZ = 30_mm;
  auto cylBounds = std::make_shared<CylinderVolumeBounds>(10_mm, 20_mm, hlZ);
  auto cyl = std::make_unique<TrackingVolume>(Transform3::Identity(), cylBounds,
                                              "child");

  root.addStaticVolume(std::move(cyl));

  BOOST_CHECK_EQUAL(root.children().size(), 1);

  auto tGeometry = root.construct({}, gctx, *logger);

  BOOST_REQUIRE(tGeometry);

  BOOST_CHECK_EQUAL(tGeometry->highestTrackingVolume()->volumes().size(), 1);

  BOOST_CHECK_EQUAL(countVolumes(*tGeometry), 2);

  auto lookup = nameLookup(*tGeometry);
  auto actCyl =
      dynamic_cast<const CylinderVolumeBounds&>(lookup("child").volumeBounds());
  // Size as given
  BOOST_CHECK_EQUAL(actCyl.get(CylinderVolumeBounds::eMinR), 10_mm);
  BOOST_CHECK_EQUAL(actCyl.get(CylinderVolumeBounds::eMaxR), 20_mm);
  BOOST_CHECK_EQUAL(actCyl.get(CylinderVolumeBounds::eHalfLengthZ), hlZ);

  auto worldCyl =
      dynamic_cast<const CylinderVolumeBounds&>(lookup("World").volumeBounds());
  BOOST_CHECK_EQUAL(worldCyl.get(CylinderVolumeBounds::eMinR), 9_mm);
  BOOST_CHECK_EQUAL(worldCyl.get(CylinderVolumeBounds::eMaxR), 22_mm);
  BOOST_CHECK_EQUAL(worldCyl.get(CylinderVolumeBounds::eHalfLengthZ),
                    hlZ + 20_mm);

  BOOST_CHECK_EQUAL(lookup("World").portals().size(), 8);
}

BOOST_AUTO_TEST_CASE(CylinderContainer) {
  Blueprint::Config cfg;
  cfg.envelope[AxisDirection::AxisZ] = {20_mm, 20_mm};
  cfg.envelope[AxisDirection::AxisR] = {2_mm, 20_mm};
  auto root = std::make_unique<Blueprint>(cfg);

  auto& cyl = root->addCylinderContainer("Container", AxisDirection::AxisZ);
  cyl.setAttachmentStrategy(VolumeAttachmentStrategy::Gap);

  double z0 = -200_mm;
  double hlZ = 30_mm;
  auto cylBounds = std::make_shared<CylinderVolumeBounds>(10_mm, 20_mm, hlZ);
  for (std::size_t i = 0; i < 3; i++) {
    auto childCyl = std::make_unique<TrackingVolume>(
        Transform3::Identity() *
            Translation3{Vector3{0, 0, z0 + i * 2 * hlZ * 1.2}},
        cylBounds, "child" + std::to_string(i));
    cyl.addStaticVolume(std::move(childCyl));
  }

  auto tGeometry = root->construct({}, gctx, *logger);

  // 4 real volumes + 2 gaps
  BOOST_CHECK_EQUAL(countVolumes(*tGeometry), 6);

  auto lookup = nameLookup(*tGeometry);
  auto worldCyl =
      dynamic_cast<const CylinderVolumeBounds&>(lookup("World").volumeBounds());
  BOOST_CHECK_EQUAL(worldCyl.get(CylinderVolumeBounds::eMinR), 8_mm);
  BOOST_CHECK_EQUAL(worldCyl.get(CylinderVolumeBounds::eMaxR), 40_mm);
  BOOST_CHECK_EQUAL(worldCyl.get(CylinderVolumeBounds::eHalfLengthZ), 122_mm);

  BOOST_CHECK_EQUAL(lookup("World").portals().size(), 8);

  for (std::size_t i = 0; i < 3; i++) {
    auto actCyl = dynamic_cast<const CylinderVolumeBounds&>(
        lookup("child" + std::to_string(i)).volumeBounds());
    BOOST_CHECK_EQUAL(actCyl.get(CylinderVolumeBounds::eMinR), 10_mm);
    BOOST_CHECK_EQUAL(actCyl.get(CylinderVolumeBounds::eMaxR), 20_mm);
    BOOST_CHECK_EQUAL(actCyl.get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  }

  for (std::size_t i = 0; i < 2; i++) {
    auto gapCyl = dynamic_cast<const CylinderVolumeBounds&>(
        lookup("Container::Gap" + std::to_string(i + 1)).volumeBounds());
    BOOST_CHECK_EQUAL(gapCyl.get(CylinderVolumeBounds::eMinR), 10_mm);
    BOOST_CHECK_EQUAL(gapCyl.get(CylinderVolumeBounds::eMaxR), 20_mm);
    BOOST_CHECK_EQUAL(gapCyl.get(CylinderVolumeBounds::eHalfLengthZ), 6_mm);
  }
}

BOOST_AUTO_TEST_CASE(Confined) {
  Transform3 base{Transform3::Identity()};

  Blueprint::Config cfg;
  cfg.envelope[AxisDirection::AxisZ] = {20_mm, 20_mm};
  cfg.envelope[AxisDirection::AxisR] = {2_mm, 20_mm};
  auto root = std::make_unique<Blueprint>(cfg);

  root->addStaticVolume(
      base, std::make_shared<CylinderVolumeBounds>(50_mm, 400_mm, 1000_mm),
      "PixelWrapper", [&](auto& wrap) {
        double rMin = 100_mm;
        double rMax = 350_mm;
        double hlZ = 100_mm;

        wrap.addStaticVolume(
            base * Translation3{Vector3{0, 0, -600_mm}},
            std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ),
            "PixelNeg1");

        wrap.addStaticVolume(
            base * Translation3{Vector3{0, 0, -200_mm}},
            std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ),
            "PixelNeg2");

        wrap.addStaticVolume(
            base * Translation3{Vector3{0, 0, 200_mm}},
            std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ),
            "PixelPos1");

        wrap.addStaticVolume(
            base * Translation3{Vector3{0, 0, 600_mm}},
            std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ),
            "PixelPos2");
      });

  auto trackingGeometry = root->construct({}, gctx, *logger);

  // overall dimensions are the wrapper volume + envelope
  auto lookup = nameLookup(*trackingGeometry);
  auto worldCyl =
      dynamic_cast<const CylinderVolumeBounds&>(lookup("World").volumeBounds());
  BOOST_CHECK_EQUAL(worldCyl.get(CylinderVolumeBounds::eMinR), 48_mm);
  BOOST_CHECK_EQUAL(worldCyl.get(CylinderVolumeBounds::eMaxR), 420_mm);
  BOOST_CHECK_EQUAL(worldCyl.get(CylinderVolumeBounds::eHalfLengthZ), 1020_mm);

  // 4 outer portals and 4 inner
  BOOST_CHECK_EQUAL(lookup("World").portals().size(), 8);
  BOOST_CHECK_EQUAL(lookup("World").volumes().size(), 1);

  auto wrapperCyl = dynamic_cast<const CylinderVolumeBounds&>(
      lookup("PixelWrapper").volumeBounds());
  BOOST_CHECK_EQUAL(wrapperCyl.get(CylinderVolumeBounds::eMinR), 50_mm);
  BOOST_CHECK_EQUAL(wrapperCyl.get(CylinderVolumeBounds::eMaxR), 400_mm);
  BOOST_CHECK_EQUAL(wrapperCyl.get(CylinderVolumeBounds::eHalfLengthZ),
                    1000_mm);
  BOOST_CHECK_EQUAL(lookup("PixelWrapper").portals().size(), 4 + 4 * 4);
  BOOST_CHECK_EQUAL(lookup("PixelWrapper").volumes().size(), 4);

  for (const auto& name :
       {"PixelNeg1", "PixelNeg2", "PixelPos1", "PixelPos2"}) {
    auto actCyl =
        dynamic_cast<const CylinderVolumeBounds&>(lookup(name).volumeBounds());
    BOOST_CHECK_EQUAL(actCyl.get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(actCyl.get(CylinderVolumeBounds::eMaxR), 350_mm);
    BOOST_CHECK_EQUAL(actCyl.get(CylinderVolumeBounds::eHalfLengthZ), 100_mm);
    BOOST_CHECK_EQUAL(lookup(name).portals().size(), 4);
  }
}

BOOST_AUTO_TEST_CASE(DiscLayer) {
  double yrot = 45_degree;
  Transform3 base = Transform3::Identity() * AngleAxis3{yrot, Vector3::UnitY()};

  std::vector<std::shared_ptr<Surface>> surfaces;
  std::vector<std::unique_ptr<DetectorElementBase>> elements;
  double r = 300_mm;
  std::size_t nSensors = 8;
  double thickness = 2.5_mm;
  auto recBounds = std::make_shared<RectangleBounds>(40_mm, 60_mm);

  double deltaPhi = 2 * std::numbers::pi / nSensors;
  for (std::size_t i = 0; i < nSensors; i++) {
    // Create a fan of sensors

    Transform3 trf = base * AngleAxis3{deltaPhi * i, Vector3::UnitZ()} *
                     Translation3(Vector3::UnitX() * r);

    if (i % 2 == 0) {
      trf = trf * Translation3{Vector3::UnitZ() * 5_mm};
    }

    auto& element = elements.emplace_back(
        std::make_unique<DetectorElementStub>(trf, recBounds, thickness));

    element->surface().assignDetectorElement(*element);

    surfaces.push_back(element->surface().getSharedPtr());
  }

  Blueprint root{{.envelope = ExtentEnvelope{{
                      .z = {2_mm, 2_mm},
                      .r = {3_mm, 5_mm},
                  }}}};

  root.addLayer("Layer0", [&](auto& layer) {
    layer.setSurfaces(surfaces)
        .setLayerType(LayerBlueprintNode::LayerType::Disc)
        .setEnvelope(ExtentEnvelope{{
            .z = {0.1_mm, 0.1_mm},
            .r = {1_mm, 1_mm},
        }})
        .setTransform(base);
  });

  auto trackingGeometry = root.construct({}, gctx, *logger);

  std::size_t nSurfaces = 0;

  trackingGeometry->visitSurfaces([&](const Surface* surface) {
    if (surface->associatedDetectorElement() != nullptr) {
      nSurfaces++;
    }
  });

  BOOST_CHECK_EQUAL(nSurfaces, surfaces.size());
  BOOST_CHECK_EQUAL(countVolumes(*trackingGeometry), 2);
  auto lookup = nameLookup(*trackingGeometry);
  auto layerCyl = dynamic_cast<const CylinderVolumeBounds&>(
      lookup("Layer0").volumeBounds());
  BOOST_CHECK_CLOSE(layerCyl.get(CylinderVolumeBounds::eMinR), 258.9999999_mm,
                    1e-6);
  BOOST_CHECK_CLOSE(layerCyl.get(CylinderVolumeBounds::eMaxR), 346.25353003_mm,
                    1e-6);
  BOOST_CHECK_CLOSE(layerCyl.get(CylinderVolumeBounds::eHalfLengthZ), 3.85_mm,
                    1e-6);
}

BOOST_AUTO_TEST_CASE(CylinderLayer) {
  double yrot = 0_degree;
  Transform3 base = Transform3::Identity() * AngleAxis3{yrot, Vector3::UnitY()};

  std::vector<std::shared_ptr<Surface>> surfaces;
  std::vector<std::unique_ptr<DetectorElementBase>> elements;

  double r = 300_mm;
  std::size_t nStaves = 10;
  int nSensorsPerStave = 8;
  double thickness = 0;
  double hlPhi = 40_mm;
  double hlZ = 60_mm;
  auto recBounds = std::make_shared<RectangleBounds>(hlPhi, hlZ);

  double deltaPhi = 2 * std::numbers::pi / nStaves;

  for (std::size_t istave = 0; istave < nStaves; istave++) {
    for (int isensor = -nSensorsPerStave; isensor <= nSensorsPerStave;
         isensor++) {
      double z = isensor * (2 * hlZ + 5_mm);

      Transform3 trf = base * Translation3(Vector3::UnitZ() * z) *
                       AngleAxis3{deltaPhi * istave, Vector3::UnitZ()} *
                       Translation3(Vector3::UnitX() * r) *
                       AngleAxis3{10_degree, Vector3::UnitZ()} *
                       AngleAxis3{90_degree, Vector3::UnitY()} *
                       AngleAxis3{90_degree, Vector3::UnitZ()};
      auto& element = elements.emplace_back(
          std::make_unique<DetectorElementStub>(trf, recBounds, thickness));
      element->surface().assignDetectorElement(*element);
      surfaces.push_back(element->surface().getSharedPtr());
    }
  }

  Blueprint root{{.envelope = ExtentEnvelope{{
                      .z = {2_mm, 2_mm},
                      .r = {3_mm, 5_mm},
                  }}}};

  root.addLayer("Layer0", [&](auto& layer) {
    layer.setSurfaces(surfaces)
        .setLayerType(LayerBlueprintNode::LayerType::Cylinder)
        .setEnvelope(ExtentEnvelope{{
            .z = {10_mm, 10_mm},
            .r = {20_mm, 10_mm},
        }})
        .setTransform(base);
  });

  auto trackingGeometry = root.construct({}, gctx, *logger);

  std::size_t nSurfaces = 0;

  trackingGeometry->visitSurfaces([&](const Surface* surface) {
    if (surface->associatedDetectorElement() != nullptr) {
      nSurfaces++;
    }
  });

  BOOST_CHECK_EQUAL(nSurfaces, surfaces.size());
  BOOST_CHECK_EQUAL(countVolumes(*trackingGeometry), 2);
  auto lookup = nameLookup(*trackingGeometry);
  auto layerCyl = dynamic_cast<const CylinderVolumeBounds&>(
      lookup("Layer0").volumeBounds());
  BOOST_CHECK_EQUAL(lookup("Layer0").portals().size(), 4);
  BOOST_CHECK_CLOSE(layerCyl.get(CylinderVolumeBounds::eMinR), 275.6897761_mm,
                    1e-6);
  BOOST_CHECK_CLOSE(layerCyl.get(CylinderVolumeBounds::eMaxR), 319.4633358_mm,
                    1e-6);
  BOOST_CHECK_CLOSE(layerCyl.get(CylinderVolumeBounds::eHalfLengthZ), 1070_mm,
                    1e-6);
}

BOOST_AUTO_TEST_CASE(Material) {
  Blueprint::Config cfg;
  cfg.envelope[AxisDirection::AxisZ] = {20_mm, 20_mm};
  cfg.envelope[AxisDirection::AxisR] = {1_mm, 2_mm};
  Blueprint root{cfg};

  double hlZ = 30_mm;
  auto cylBounds = std::make_shared<CylinderVolumeBounds>(10_mm, 20_mm, hlZ);
  auto cyl = std::make_unique<TrackingVolume>(Transform3::Identity(), cylBounds,
                                              "child");

  using enum AxisDirection;
  using enum CylinderVolumeBounds::Face;
  using enum AxisBoundaryType;

  root.addMaterial("Material", [&](auto& mat) {
    // @TODO: This API is not great
    mat.setBinning(std::vector{
        std::tuple{NegativeDisc, ProtoAxis{AxisR, Bound, 5},
                   ProtoAxis{AxisPhi, Bound, 10}},
        std::tuple{PositiveDisc, ProtoAxis{AxisR, Bound, 15},
                   ProtoAxis{AxisPhi, Bound, 20}},
    });

    mat.addStaticVolume(std::move(cyl));
  });

  auto trackingGeometry = root.construct({}, gctx, *logger);

  BOOST_CHECK_EQUAL(countVolumes(*trackingGeometry), 2);
  auto lookup = nameLookup(*trackingGeometry);
  auto& child = lookup("child");

  const auto* negDisc = child.portals().at(0).surface().surfaceMaterial();
  const auto* posDisc = child.portals().at(1).surface().surfaceMaterial();
  BOOST_CHECK_NE(negDisc, nullptr);
  BOOST_CHECK_NE(posDisc, nullptr);

  const auto& negDiscMat =
      dynamic_cast<const ProtoGridSurfaceMaterial&>(*negDisc);
  const auto& posDiscMat =
      dynamic_cast<const ProtoGridSurfaceMaterial&>(*posDisc);

  BOOST_CHECK_EQUAL(negDiscMat.binning().at(0).getAxis().getNBins(), 5);
  BOOST_CHECK_EQUAL(negDiscMat.binning().at(1).getAxis().getNBins(), 10);
  BOOST_CHECK_EQUAL(posDiscMat.binning().at(0).getAxis().getNBins(), 15);
  BOOST_CHECK_EQUAL(posDiscMat.binning().at(1).getAxis().getNBins(), 20);

  for (std::size_t i = 2; i < child.portals().size(); i++) {
    BOOST_CHECK_EQUAL(child.portals().at(i).surface().surfaceMaterial(),
                      nullptr);
  }
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE_END();

}  // namespace Acts::Test
