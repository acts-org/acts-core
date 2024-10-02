// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// Workaround for building on clang+libstdc++
#include "Acts/Utilities/detail/ReferenceWrapperAnyCompat.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/PlaneLayer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "Acts/Utilities/BinnedArray.hpp"
#include "Acts/Utilities/BinnedArrayXD.hpp"

#include <functional>
#include <vector>

namespace Acts::Test {

struct CubicTrackingGeometry {
  /// Default constructor for the Cubic tracking geometry
  ///
  /// @param gctx the geometry context for this geometry at building time
  CubicTrackingGeometry(const GeometryContext& gctx) : geoContext(gctx) {
    using namespace UnitLiterals;

    // Construct the rotation
    double rotationAngle = 90_degree;
    Vector3 xPos(cos(rotationAngle), 0., sin(rotationAngle));
    Vector3 yPos(0., 1., 0.);
    Vector3 zPos(-sin(rotationAngle), 0., cos(rotationAngle));
    rotation.col(0) = xPos;
    rotation.col(1) = yPos;
    rotation.col(2) = zPos;

    // Boundaries of the surfaces
    rBounds =
        std::make_shared<const RectangleBounds>(RectangleBounds(0.5_m, 0.5_m));

    // Material of the surfaces
    MaterialSlab matProp(makeBeryllium(), 0.5_mm);
    surfaceMaterial = std::make_shared<HomogeneousSurfaceMaterial>(matProp);
  }

  /// Call operator to build the standard cubic tracking geometry
  std::shared_ptr<const TrackingGeometry> operator()() {
    using namespace UnitLiterals;

    // Set translation vectors
    double eps = 1_mm;
    std::vector<Vector3> translations;
    translations.push_back({-2_m, 0., 0.});
    translations.push_back({-1_m, 0., 0.});
    translations.push_back({1_m - eps, 0., 0.});
    translations.push_back({1_m + eps, 0., 0.});
    translations.push_back({2_m - eps, 0., 0.});
    translations.push_back({2_m + eps, 0., 0.});

    std::vector<double> rotAngle;
    rotAngle.push_back(0.);
    rotAngle.push_back(0.);
    rotAngle.push_back(0.026);
    rotAngle.push_back(-0.026);
    rotAngle.push_back(0.026);
    rotAngle.push_back(-0.026);

    // Construct surfaces
    std::array<std::shared_ptr<const Surface>, 6> surfaces;
    for (unsigned int i = 0; i < translations.size(); i++) {
      RotationMatrix3 rotation_strip;
      double angle = rotAngle[i];
      Vector3 xPos(cos(angle), sin(angle), 0.);
      Vector3 yPos(-sin(angle), cos(angle), 0.);
      Vector3 zPos(0., 0., 1.);
      rotation_strip.col(0) = xPos;
      rotation_strip.col(1) = yPos;
      rotation_strip.col(2) = zPos;

      Transform3 trafo(Transform3::Identity() * rotation * rotation_strip);
      trafo.translation() = translations[i];

      // Create the detector element
      auto detElement = std::make_unique<const DetectorElementStub>(
          trafo, rBounds, 1._um, surfaceMaterial);
      // And remember the surface
      surfaces[i] = detElement->surface().getSharedPtr();
      // Add it to the event store
      detectorStore.push_back(std::move(detElement));
    }

    // Construct layers
    std::array<LayerPtr, 6> layers{};
    for (unsigned int i = 0; i < 6; i++) {
      Transform3 trafo(Transform3::Identity() * rotation);
      trafo.translation() = translations[i];

      std::unique_ptr<SurfaceArray> surArray(new SurfaceArray(surfaces[i]));

      layers[i] =
          PlaneLayer::create(trafo, rBounds, std::move(surArray), 1._mm);

      auto mutableSurface = const_cast<Surface*>(surfaces[i].get());
      mutableSurface->associateLayer(*layers[i]);
    }

    // Build volume for surfaces with negative x-values
    Transform3 trafoVol1(Transform3::Identity());
    trafoVol1.translation() = Vector3(-1.5_m, 0., 0.);

    auto boundsVol = std::make_shared<CuboidVolumeBounds>(1.5_m, 0.5_m, 0.5_m);

    LayerArrayCreator::Config lacConfig;
    LayerArrayCreator layArrCreator(
        lacConfig, getDefaultLogger("LayerArrayCreator", Logging::INFO));

    LayerVector layVec;
    layVec.push_back(layers[0]);
    layVec.push_back(layers[1]);
    std::unique_ptr<const LayerArray> layArr1(layArrCreator.layerArray(
        geoContext, layVec, -2_m - 1._mm, -1._m + 1._mm, BinningType::arbitrary,
        BinningValue::binX));

    auto trackVolume1 = std::make_shared<TrackingVolume>(
        trafoVol1, boundsVol, nullptr, std::move(layArr1), nullptr,
        MutableTrackingVolumeVector{}, "Volume 1");

    // Build volume for surfaces with positive x-values
    Transform3 trafoVol2(Transform3::Identity());
    trafoVol2.translation() = Vector3(1.5_m, 0., 0.);

    layVec.clear();
    for (unsigned int i = 2; i < 6; i++) {
      layVec.push_back(layers[i]);
    }
    std::unique_ptr<const LayerArray> layArr2(
        layArrCreator.layerArray(geoContext, layVec, 1._m - 2._mm, 2._m + 2._mm,
                                 BinningType::arbitrary, BinningValue::binX));

    auto trackVolume2 = std::make_shared<TrackingVolume>(
        trafoVol2, boundsVol, nullptr, std::move(layArr2), nullptr,
        MutableTrackingVolumeVector{}, "Volume 2");

    // Glue volumes
    trackVolume2->glueTrackingVolume(
        geoContext, BoundarySurfaceFace::negativeFaceYZ, trackVolume1.get(),
        BoundarySurfaceFace::positiveFaceYZ);

    trackVolume1->glueTrackingVolume(
        geoContext, BoundarySurfaceFace::positiveFaceYZ, trackVolume2.get(),
        BoundarySurfaceFace::negativeFaceYZ);

    // Build world volume
    Transform3 trafoWorld(Transform3::Identity());
    trafoWorld.translation() = Vector3(0., 0., 0.);

    auto worldVolBds = std::make_shared<CuboidVolumeBounds>(3._m, 0.5_m, 0.5_m);

    std::vector<std::pair<TrackingVolumePtr, Vector3>> tapVec;

    tapVec.push_back(std::make_pair(trackVolume1, Vector3(-1.5_m, 0., 0.)));
    tapVec.push_back(std::make_pair(trackVolume2, Vector3(1.5_m, 0., 0.)));

    std::vector<float> binBoundaries = {-3._m, 0., 3._m};

    BinningData binData(BinningOption::open, BinningValue::binX, binBoundaries);
    std::unique_ptr<const BinUtility> bu(new BinUtility(binData));

    std::shared_ptr<const TrackingVolumeArray> trVolArr(
        new BinnedArrayXD<TrackingVolumePtr>(tapVec, std::move(bu)));

    MutableTrackingVolumePtr mtvpWorld(std::make_shared<TrackingVolume>(
        trafoWorld, worldVolBds, nullptr, nullptr, trVolArr,
        MutableTrackingVolumeVector{}, "World"));

    // Build and return tracking geometry
    return std::make_shared<TrackingGeometry>(mtvpWorld);
  }

  RotationMatrix3 rotation = RotationMatrix3::Identity();
  std::shared_ptr<const RectangleBounds> rBounds = nullptr;
  std::shared_ptr<const ISurfaceMaterial> surfaceMaterial = nullptr;

  std::vector<std::unique_ptr<const DetectorElementStub>> detectorStore = {};

  std::reference_wrapper<const GeometryContext> geoContext;
};
}  // namespace Acts::Test
