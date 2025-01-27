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

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Detector/ProtoBinning.hpp"
#include "Acts/Geometry/Blueprint.hpp"
#include "Acts/Geometry/CylinderContainerBlueprintNode.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeStack.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerBlueprintNode.hpp"
#include "Acts/Geometry/MaterialDesignatorBlueprintNode.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/VolumeAttachmentStrategy.hpp"
#include "Acts/Geometry/VolumeResizeStrategy.hpp"
#include "Acts/Navigation/INavigationPolicy.hpp"
#include "Acts/Navigation/NavigationStream.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"

#include <fstream>
#include <random>
#include <vector>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

auto logger = Acts::getDefaultLogger("UnitTests", Acts::Logging::DEBUG);

GeometryContext gctx;

inline std::vector<std::shared_ptr<Surface>> makeFanLayer(
    const Transform3& base,
    std::vector<std::unique_ptr<DetectorElementBase>>& elements,
    double r = 300_mm, std::size_t nSensors = 8, double thickness = 0) {
  auto recBounds = std::make_shared<RectangleBounds>(40_mm, 60_mm);

  double deltaPhi = 2 * std::numbers::pi / nSensors;
  std::vector<std::shared_ptr<Surface>> surfaces;
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
  return surfaces;
}

inline std::vector<std::shared_ptr<Surface>> makeBarrelLayer(
    const Transform3& base,
    std::vector<std::unique_ptr<DetectorElementBase>>& elements,
    double r = 300_mm, std::size_t nStaves = 10, int nSensorsPerStave = 8,
    double thickness = 0, double hlPhi = 40_mm, double hlZ = 60_mm) {
  auto recBounds = std::make_shared<RectangleBounds>(hlPhi, hlZ);

  double deltaPhi = 2 * std::numbers::pi / nStaves;
  std::vector<std::shared_ptr<Surface>> surfaces;

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

  return surfaces;
}

BOOST_AUTO_TEST_SUITE(Geometry);

BOOST_AUTO_TEST_SUITE(BlueprintApiTest);

void pseudoNavigation(const TrackingGeometry& trackingGeometry,
                      Vector3 position, const Vector3& direction,
                      std::ostream& csv, std::size_t run,
                      std::size_t substepsPerCm, const Logger& logger) {
  ACTS_VERBOSE("start navigation " << run);
  ACTS_VERBOSE("dir: " << direction.transpose());
  ACTS_VERBOSE(direction.norm());

  std::mt19937 rng{static_cast<unsigned int>(run)};
  std::uniform_real_distribution<> dist{0.01, 0.99};

  const auto* volume = trackingGeometry.lowestTrackingVolume(gctx, position);
  BOOST_REQUIRE_NE(volume, nullptr);
  ACTS_VERBOSE(volume->volumeName());

  NavigationStream main;
  const TrackingVolume* currentVolume = volume;

  csv << run << "," << position[0] << "," << position[1] << "," << position[2];
  csv << "," << volume->geometryId().volume();
  csv << "," << volume->geometryId().boundary();
  csv << "," << volume->geometryId().sensitive();
  csv << std::endl;

  ACTS_VERBOSE("start pseudo navigation");

  for (std::size_t i = 0; i < 100; i++) {
    main = NavigationStream{};
    AppendOnlyNavigationStream stream{main};

    currentVolume->initializeNavigationCandidates(
        {.position = position, .direction = direction}, stream, logger);

    ACTS_VERBOSE(main.candidates().size() << " candidates");

    for (const auto& candidate : main.candidates()) {
      ACTS_VERBOSE(" -> " << candidate.surface().geometryId());
      ACTS_VERBOSE("    " << candidate.surface().toStream(gctx));
    }

    ACTS_VERBOSE("initializing candidates");
    main.initialize(gctx, {position, direction}, BoundaryTolerance::None());

    ACTS_VERBOSE(main.candidates().size() << " candidates remaining");

    for (const auto& candidate : main.candidates()) {
      ACTS_VERBOSE(" -> " << candidate.surface().geometryId());
      ACTS_VERBOSE("    " << candidate.surface().toStream(gctx));
    }

    if (main.currentCandidate().surface().isOnSurface(gctx, position,
                                                      direction)) {
      ACTS_VERBOSE("Already on surface at initialization, skipping candidate");

      auto id = main.currentCandidate().surface().geometryId();
      csv << run << "," << position[0] << "," << position[1] << ","
          << position[2];
      csv << "," << id.volume();
      csv << "," << id.boundary();
      csv << "," << id.sensitive();
      csv << std::endl;
      if (!main.switchToNextCandidate()) {
        ACTS_WARNING("candidates exhausted unexpectedly");
        break;
      }
    }

    auto writeIntersection = [&](const Vector3& pos, const Surface& surface) {
      csv << run << "," << pos[0] << "," << pos[1] << "," << pos[2];
      csv << "," << surface.geometryId().volume();
      csv << "," << surface.geometryId().boundary();
      csv << "," << surface.geometryId().sensitive();
      csv << std::endl;
    };

    bool terminated = false;
    while (main.remainingCandidates() > 0) {
      const auto& candidate = main.currentCandidate();

      ACTS_VERBOSE(candidate.portal);
      ACTS_VERBOSE(candidate.intersection.position().transpose());

      ACTS_VERBOSE("moving to position: " << position.transpose() << " (r="
                                          << VectorHelpers::perp(position)
                                          << ")");

      Vector3 delta = candidate.intersection.position() - position;

      std::size_t substeps =
          std::max(1l, std::lround(delta.norm() / 10_cm * substepsPerCm));

      for (std::size_t j = 0; j < substeps; j++) {
        // position += delta / (substeps + 1);
        Vector3 subpos = position + dist(rng) * delta;
        csv << run << "," << subpos[0] << "," << subpos[1] << "," << subpos[2];
        csv << "," << currentVolume->geometryId().volume();
        csv << ",0,0";  // zero boundary and sensitive ids
        csv << std::endl;
      }

      position = candidate.intersection.position();
      ACTS_VERBOSE("                 -> "
                   << position.transpose()
                   << " (r=" << VectorHelpers::perp(position) << ")");

      writeIntersection(position, candidate.surface());

      if (candidate.portal != nullptr) {
        ACTS_VERBOSE(
            "On portal: " << candidate.portal->surface().toStream(gctx));
        currentVolume =
            candidate.portal->resolveVolume(gctx, position, direction).value();

        if (currentVolume == nullptr) {
          ACTS_VERBOSE("switched to nullptr -> we're done");
          terminated = true;
        }
        break;

      } else {
        ACTS_VERBOSE("Not on portal");
      }

      main.switchToNextCandidate();
    }

    if (terminated) {
      ACTS_VERBOSE("Terminate pseudo navigation");
      break;
    }

    ACTS_VERBOSE("switched to " << currentVolume->volumeName());

    ACTS_VERBOSE("-----");
  }
}

BOOST_AUTO_TEST_CASE(NodeApiTestContainers) {
  // Transform3 base{AngleAxis3{30_degree, Vector3{1, 0, 0}}};
  Transform3 base{Transform3::Identity()};

  std::vector<std::unique_ptr<DetectorElementBase>> detectorElements;
  auto makeFan = [&](const Transform3& layerBase, auto&&..., double r,
                     std::size_t nSensors, double thickness) {
    return makeFanLayer(layerBase, detectorElements, r, nSensors, thickness);
  };

  Blueprint::Config cfg;
  cfg.envelope[AxisDirection::AxisZ] = {20_mm, 20_mm};
  cfg.envelope[AxisDirection::AxisR] = {0_mm, 20_mm};
  auto root = std::make_unique<Blueprint>(cfg);

  root->addMaterial("GlobalMaterial", [&](MaterialDesignatorBlueprintNode&
                                              mat) {
    Experimental::ProtoBinning zBinning{AxisDirection::AxisZ,
                                        AxisBoundaryType::Bound, 20};

    Experimental::ProtoBinning rPhiBinning{AxisDirection::AxisRPhi,
                                           AxisBoundaryType::Bound, 20};

    mat.setBinning(std::vector{std::tuple{
        CylinderVolumeBounds::Face::OuterCylinder, rPhiBinning, zBinning}});

    mat.addCylinderContainer("Detector", AxisDirection::AxisR, [&](auto& det) {
      det.addCylinderContainer("Pixel", AxisDirection::AxisZ, [&](auto& cyl) {
        cyl.setAttachmentStrategy(VolumeAttachmentStrategy::Gap)
            .setResizeStrategy(VolumeResizeStrategy::Gap);

        cyl.addCylinderContainer(
            "PixelNegativeEndcap", AxisDirection::AxisZ, [&](auto& ec) {
              ec.setAttachmentStrategy(VolumeAttachmentStrategy::Gap);

              auto makeLayer = [&](const Transform3& trf, auto& layer) {
                std::vector<std::shared_ptr<Surface>> surfaces;
                auto layerSurfaces = makeFan(trf, 300_mm, 10, 2_mm);
                std::copy(layerSurfaces.begin(), layerSurfaces.end(),
                          std::back_inserter(surfaces));
                layerSurfaces = makeFan(trf, 500_mm, 16, 2_mm);
                std::copy(layerSurfaces.begin(), layerSurfaces.end(),
                          std::back_inserter(surfaces));

                layer.setSurfaces(surfaces)
                    .setLayerType(LayerBlueprintNode::LayerType::Disc)
                    .setEnvelope(ExtentEnvelope{{
                        .z = {5_mm, 5_mm},
                        .r = {10_mm, 20_mm},
                    }})
                    .setTransform(base);
              };

              ec.addLayer("PixelNeg1", [&](auto& layer) {
                makeLayer(base * Translation3{Vector3{0, 0, -700_mm}}, layer);
              });

              ec.addLayer("PixelNeg2", [&](auto& layer) {
                makeLayer(base * Translation3{Vector3{0, 0, -500_mm}}, layer);
              });
            });

        cyl.addCylinderContainer(
            "PixelBarrel", AxisDirection::AxisR, [&](auto& brl) {
              brl.setAttachmentStrategy(VolumeAttachmentStrategy::Gap)
                  .setResizeStrategy(VolumeResizeStrategy::Gap);

              auto makeLayer = [&](const std::string& name, double r,
                                   std::size_t nStaves, int nSensorsPerStave) {
                brl.addLayer(name, [&](auto& layer) {
                  std::vector<std::shared_ptr<Surface>> surfaces =
                      makeBarrelLayer(base, detectorElements, r, nStaves,
                                      nSensorsPerStave, 2.5_mm, 10_mm, 20_mm);

                  layer.setSurfaces(surfaces)
                      .setLayerType(LayerBlueprintNode::LayerType::Cylinder)
                      .setEnvelope(ExtentEnvelope{{
                          .z = {5_mm, 5_mm},
                          .r = {1_mm, 1_mm},
                      }})
                      .setTransform(base);
                });
              };

              makeLayer("PixelLayer0", 30_mm, 18, 5);
              makeLayer("PixelLayer1", 90_mm, 30, 6);

              brl.addStaticVolume(base,
                                  std::make_shared<CylinderVolumeBounds>(
                                      100_mm, 110_mm, 250_mm),
                                  "PixelSupport");

              makeLayer("PixelLayer2", 150_mm, 40, 7);
              makeLayer("PixelLayer3", 250_mm, 70, 8);
            });

        auto& ec =
            cyl.addCylinderContainer("PixelPosWrapper", AxisDirection::AxisR);
        ec.setResizeStrategy(VolumeResizeStrategy::Gap);
        ec.addStaticVolume(std::make_unique<TrackingVolume>(
            base * Translation3{Vector3{0, 0, 600_mm}},
            std::make_shared<CylinderVolumeBounds>(150_mm, 390_mm, 200_mm),
            "PixelPositiveEndcap"));
      });

      det.addStaticVolume(
          base, std::make_shared<CylinderVolumeBounds>(0_mm, 23_mm, 1000_mm),
          "BeamPipe");
    });
  });

  std::ofstream dot{"api_test_container.dot"};
  root->graphviz(dot);

  auto trackingGeometry = root->construct({}, gctx, *logger);

  trackingGeometry->visitVolumes([&](const TrackingVolume* volume) {
    std::cout << volume->volumeName() << std::endl;
    std::cout << " -> id: " << volume->geometryId() << std::endl;
    std::cout << " -> " << volume->portals().size() << " portals" << std::endl;
  });

  ObjVisualization3D vis;

  trackingGeometry->visualize(vis, gctx, {}, {});

  vis.write("api_test_container.obj");

  Vector3 position = Vector3::Zero();
  std::ofstream csv{"api_test_container.csv"};
  csv << "x,y,z,volume,boundary,sensitive" << std::endl;

  std::mt19937 rnd{42};

  std::uniform_real_distribution<> dist{-1, 1};

  double etaWidth = 3;
  double thetaMin = 2 * std::atan(std::exp(-etaWidth));
  double thetaMax = 2 * std::atan(std::exp(etaWidth));
  std::uniform_real_distribution<> thetaDist{thetaMin, thetaMax};

  using namespace Acts::UnitLiterals;

  for (std::size_t i = 0; i < 5000; i++) {
    double theta = thetaDist(rnd);
    double phi = 2 * std::numbers::pi * dist(rnd);

    Vector3 direction;
    direction[0] = std::sin(theta) * std::cos(phi);
    direction[1] = std::sin(theta) * std::sin(phi);
    direction[2] = std::cos(theta);

    pseudoNavigation(*trackingGeometry, position, direction, csv, i, 2,
                     *logger->clone(std::nullopt, Logging::DEBUG));
  }
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE_END();

}  // namespace Acts::Test
