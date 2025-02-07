// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/MuonSpectrometerMockupDetector/MockupSectorBuilder.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Plugins/Geant4/Geant4Converters.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorElement.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorSurfaceFactory.hpp"
#include "Acts/Plugins/Geant4/Geant4PhysicalVolumeSelectors.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/ViewConfig.hpp"
#include "ActsExamples/Geant4Detector/GdmlDetectorConstruction.hpp"
#include "ActsExamples/Geant4Detector/Geant4Detector.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ActsExamples {

MockupSectorBuilder::MockupSectorBuilder(
    const MockupSectorBuilder::Config& config) {
  mCfg = config;
  GdmlDetectorConstruction geo_gdml(mCfg.gdmlPath, {});
  g4World = geo_gdml.Construct();
}

std::shared_ptr<Acts::Experimental::DetectorVolume>
MockupSectorBuilder::buildChamber(
    const MockupSectorBuilder::ChamberConfig& chamberConfig) {
  if (g4World == nullptr) {
    throw std::invalid_argument("MockupSector: No g4World initialized");
  }

  const Acts::GeometryContext gctx;

  // Geant4Detector Config creator with the g4world from the gdml file
  auto g4WorldConfig = Geant4Detector::Config();
  g4WorldConfig.name = "Chamber";
  g4WorldConfig.g4World = g4World;

  // Get the sensitive and passive surfaces and pass to the g4World Config
  auto g4Sensitive =
      std::make_shared<Acts::Geant4PhysicalVolumeSelectors::NameSelector>(
          chamberConfig.SensitiveNames);
  auto g4Passive =
      std::make_shared<Acts::Geant4PhysicalVolumeSelectors::NameSelector>(
          chamberConfig.PassiveNames);

  auto g4SurfaceOptions = Acts::Geant4DetectorSurfaceFactory::Options();
  g4SurfaceOptions.sensitiveSurfaceSelector = g4Sensitive;
  g4SurfaceOptions.passiveSurfaceSelector = g4Passive;
  g4WorldConfig.g4SurfaceOptions = g4SurfaceOptions;

  auto g4Detector = Geant4Detector(g4WorldConfig);
  // Trigger the build of the detector
  auto [surface, elements] = Geant4Detector::buildGeant4Volumes(
      g4WorldConfig,
      *Acts::getDefaultLogger("MockupSectorBuilder", Acts::Logging::INFO));

  // The vector that holds the converted sensitive surfaces of the chamber
  std::vector<std::shared_ptr<Acts::Surface>> strawSurfaces;

  std::array<std::pair<float, float>, 3> min_max;
  std::fill(min_max.begin(), min_max.end(),
            std::make_pair<float, float>(std::numeric_limits<float>::max(),
                                         -std::numeric_limits<float>::max()));

  // Convert the physical volumes of the detector elements to straw surfaces
  for (const auto& detectorElement : elements) {
    auto context = Acts::GeometryContext();
    auto g4conv = Acts::Geant4PhysicalVolumeConverter();

    g4conv.forcedType = Acts::Surface::SurfaceType::Straw;
    auto g4ConvSurf = g4conv.Geant4PhysicalVolumeConverter::surface(
        detectorElement->g4PhysicalVolume(),
        detectorElement->transform(context));

    strawSurfaces.push_back(g4ConvSurf);

    min_max[0].first = std::min(
        min_max[0].first, static_cast<float>(g4ConvSurf->center(context).x()));
    min_max[0].second = std::max(
        min_max[0].second, static_cast<float>(g4ConvSurf->center(context).x()));

    min_max[1].first = std::min(
        min_max[1].first, static_cast<float>(g4ConvSurf->center(context).y()));
    min_max[1].second = std::max(
        min_max[1].second, static_cast<float>(g4ConvSurf->center(context).y()));

    min_max[2].first = std::min(
        min_max[2].first, static_cast<float>(g4ConvSurf->center(context).z()));
    min_max[2].second = std::max(
        min_max[2].second, static_cast<float>(g4ConvSurf->center(context).z()));
  }

  // Create the bounds of the detector volumes
  float radius = strawSurfaces.front()->bounds().values()[0];

  Acts::Vector3 minValues = {min_max[0].first, min_max[1].first,
                             min_max[2].first};
  Acts::Vector3 maxValues = {min_max[0].second, min_max[1].second,
                             min_max[2].second};

  double hx =
      strawSurfaces.front()->bounds().values()[1] + mCfg.toleranceOverlap;
  double hy = 0.5 * ((maxValues.y() + radius) - (minValues.y() - radius)) +
              mCfg.toleranceOverlap;
  double hz = 0.5 * ((maxValues.z() + radius) - (minValues.z() - radius)) +
              mCfg.toleranceOverlap;

  auto detectorVolumeBounds =
      std::make_shared<Acts::CuboidVolumeBounds>(hx, hy, hz);

  Acts::Vector3 chamber_position = {(maxValues.x() + minValues.x()) / 2,
                                    (maxValues.y() + minValues.y()) / 2,
                                    (maxValues.z() + minValues.z()) / 2};

  // create the detector volume for the chamber
  auto detectorVolume = Acts::Experimental::DetectorVolumeFactory::construct(
      Acts::Experimental::defaultPortalAndSubPortalGenerator(), gctx,
      chamberConfig.name,
      Acts::Transform3(Acts::Translation3(chamber_position)),
      std::move(detectorVolumeBounds), strawSurfaces,
      std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>{},
      Acts::Experimental::tryAllSubVolumes(),
      Acts::Experimental::tryAllPortalsAndSurfaces());

  return detectorVolume;
}

std::shared_ptr<Acts::Experimental::DetectorVolume>
MockupSectorBuilder::buildSector(
    std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>
        detVolumes) {
  if (mCfg.NumberOfSectors > maxNumberOfSectors) {
    throw std::invalid_argument("MockupSector:Number of max sectors exceeded");
  }

  const Acts::GeometryContext gctx;

  // sort the detector volumes by their radial distance (from
  // innermost---->outermost)
  std::ranges::sort(detVolumes, {},
                    [](const auto& detVol) { return detVol->center().y(); });

  auto xA = detVolumes.back()->center().x() +
            detVolumes.back()->volumeBounds().values()[0];
  auto yA = detVolumes.back()->center().y() -
            detVolumes.back()->volumeBounds().values()[1];
  auto zA = detVolumes.back()->center().z();

  auto xB = detVolumes.back()->center().x() -
            detVolumes.back()->volumeBounds().values()[0];
  auto yB = detVolumes.back()->center().y() -
            detVolumes.back()->volumeBounds().values()[1];
  auto zB = detVolumes.back()->center().z();

  Acts::Vector3 pointA = {xA, yA, zA};
  Acts::Vector3 pointB = {xB, yB, zB};

  // calculate the phi angles of the vectors
  auto phiA = Acts::VectorHelpers::phi(pointA);
  auto phiB = Acts::VectorHelpers::phi(pointB);
  double sectorAngle = std::numbers::pi;

  double halfPhi = std::numbers::pi / mCfg.NumberOfSectors;

  if (mCfg.NumberOfSectors == 1) {
    halfPhi = (phiB - phiA) / 2;
    sectorAngle = halfPhi;
  }

  const int detVolumesSize = detVolumes.size();

  std::vector<float> rmins(detVolumesSize);
  std::vector<float> rmaxs(detVolumesSize);
  std::vector<float> halfZ(detVolumesSize);
  std::vector<std::shared_ptr<Acts::CylinderVolumeBounds>>
      cylinderVolumesBounds(detVolumesSize);

  for (int i = 0; i < detVolumesSize; i++) {
    const auto& detVol = detVolumes[i];
    rmins[i] = detVol->center().y() - detVol->volumeBounds().values()[1] -
               mCfg.toleranceOverlap;
    rmaxs[i] = std::sqrt(std::pow(detVol->volumeBounds().values()[0], 2) +
                         std::pow(detVol->center().y() +
                                      detVol->volumeBounds().values()[1],
                                  2)) +
               mCfg.toleranceOverlap;
    halfZ[i] = detVol->volumeBounds().values()[2];

    cylinderVolumesBounds[i] = std::make_shared<Acts::CylinderVolumeBounds>(
        rmins[i], rmaxs[i], halfZ[i], sectorAngle);
  }

  const Acts::Vector3 pos = {0., 0., 0.};

  // the transform of the cylinder volume
  Acts::AngleAxis3 rotZ(std::numbers::pi / 2., Acts::Vector3(0., 0., 1));
  auto transform = Acts::Transform3(Acts::Translation3(pos));
  transform *= rotZ;

  // create a vector for the shifted surfaces of each chamber
  std::vector<std::shared_ptr<Acts::Surface>> shiftedSurfaces = {};

  // creare an array of vectors that holds all the chambers of each sector
  std::vector<std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>>
      chambersOfSectors(detVolumesSize);

  std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>
      detectorCylinderVolumesOfSector = {};

  for (int i = 0; i < mCfg.NumberOfSectors; i++) {
    Acts::AngleAxis3 rotation(2 * i * halfPhi, Acts::Vector3(0., 0., 1.));

    for (int itr = 0; itr < detVolumesSize; itr++) {
      const auto& detVol = detVolumes[itr];

      auto shift_vol =
          rotation * Acts::Transform3(Acts::Translation3(detVol->center()));

      for (auto& detSurf : detVol->surfaces()) {
        auto shift_surf = Acts::Transform3::Identity() * rotation;

        // create the shifted surfaces by creating copied surface objects
        auto strawSurfaceObject = Acts::Surface::makeShared<Acts::StrawSurface>(
            detSurf->transform(Acts::GeometryContext()),
            detSurf->bounds().values()[0], detSurf->bounds().values()[1]);

        auto copiedTransformStrawSurface =
            Acts::Surface::makeShared<Acts::StrawSurface>(
                Acts::GeometryContext(), *strawSurfaceObject, shift_surf);

        shiftedSurfaces.push_back(copiedTransformStrawSurface);
      }

      // create the bounds of the volumes of each chamber
      auto bounds = std::make_unique<Acts::CuboidVolumeBounds>(
          detVol->volumeBounds().values()[0],
          detVol->volumeBounds().values()[1],
          detVol->volumeBounds().values()[2]);
      // create the shifted chamber
      auto detectorVolumeSec =
          Acts::Experimental::DetectorVolumeFactory::construct(
              Acts::Experimental::defaultPortalAndSubPortalGenerator(), gctx,
              "detectorVolumeChamber_" + std::to_string(itr), shift_vol,
              std::move(bounds), shiftedSurfaces,
              std::vector<
                  std::shared_ptr<Acts::Experimental::DetectorVolume>>{},
              Acts::Experimental::tryAllSubVolumes(),
              Acts::Experimental::tryAllPortalsAndSurfaces());

      chambersOfSectors[itr].push_back(detectorVolumeSec);

      shiftedSurfaces.clear();

    }  // end of detector volumes

  }  // end of number of sectors

  for (std::size_t i = 0; i < cylinderVolumesBounds.size(); ++i) {
    detectorCylinderVolumesOfSector.push_back(
        Acts::Experimental::DetectorVolumeFactory::construct(
            Acts::Experimental::defaultPortalAndSubPortalGenerator(), gctx,
            "cylinder_volume_" + std::to_string(i), transform,
            std::move(cylinderVolumesBounds[i]),
            std::vector<std::shared_ptr<Acts::Surface>>{}, chambersOfSectors[i],
            Acts::Experimental::tryAllSubVolumes(),
            Acts::Experimental::tryAllPortalsAndSurfaces()));

  }  // end of cylinder volumes

  auto cylinderVolumesBoundsOfMother =
      std::make_shared<Acts::CylinderVolumeBounds>(
          rmins.front(), rmaxs.back(),
          *std::max_element(halfZ.begin(), halfZ.end()), sectorAngle);

  // creation of the mother volume
  auto detectorVolume = Acts::Experimental::DetectorVolumeFactory::construct(
      Acts::Experimental::defaultPortalAndSubPortalGenerator(), gctx,
      "detectorVolumeSector", transform,
      std::move(cylinderVolumesBoundsOfMother),
      std::vector<std::shared_ptr<Acts::Surface>>{},
      detectorCylinderVolumesOfSector, Acts::Experimental::tryAllSubVolumes(),
      Acts::Experimental::tryAllPortalsAndSurfaces());

  return detectorVolume;
}

void MockupSectorBuilder::drawSector(
    const std::shared_ptr<Acts::Experimental::DetectorVolume>&
        detectorVolumeSector,
    const std::string& nameObjFile) {
  Acts::ViewConfig sConfig = Acts::s_viewSensitive;

  Acts::ObjVisualization3D objSector;

  Acts::GeometryView3D::drawDetectorVolume(
      objSector, *detectorVolumeSector, Acts::GeometryContext(),
      Acts::Transform3::Identity(), sConfig);

  objSector.write(nameObjFile);
}

}  // namespace ActsExamples
