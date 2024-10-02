// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TelescopeDetector/BuildTelescopeDetector.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/DiscLayer.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ILayerArrayCreator.hpp"
#include "Acts/Geometry/ITrackingVolumeHelper.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/PlaneLayer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

std::unique_ptr<const Acts::TrackingGeometry>
ActsExamples::Telescope::buildDetector(
    const typename ActsExamples::Telescope::TelescopeDetectorElement::
        ContextType& gctx,
    std::vector<
        std::shared_ptr<ActsExamples::Telescope::TelescopeDetectorElement>>&
        detectorStore,
    const std::vector<double>& positions,
    const std::vector<double>& stereoAngles,
    const std::array<double, 2>& offsets, const std::array<double, 2>& bounds,
    double thickness, ActsExamples::Telescope::TelescopeSurfaceType surfaceType,
    Acts::BinningValue binValue) {
  using namespace Acts::UnitLiterals;

  // The rectangle bounds for plane surface
  const auto pBounds =
      std::make_shared<const Acts::RectangleBounds>(bounds[0], bounds[1]);
  // The radial bounds for disc surface
  const auto rBounds =
      std::make_shared<const Acts::RadialBounds>(bounds[0], bounds[1]);

  // Material of the surfaces
  Acts::Material silicon = Acts::Material::fromMassDensity(
      9.370_cm, 46.52_cm, 28.0855, 14, 2.329_g / 1_cm3);
  Acts::MaterialSlab matProp(silicon, thickness);
  const auto surfaceMaterial =
      std::make_shared<Acts::HomogeneousSurfaceMaterial>(matProp);

  // Construct the rotation
  // This assumes the binValue is binX, binY or binZ. No reset is necessary in
  // case of binZ
  Acts::RotationMatrix3 rotation = Acts::RotationMatrix3::Identity();
  if (binValue == Acts::BinningValue::binX) {
    rotation.col(0) = Acts::Vector3(0, 0, -1);
    rotation.col(1) = Acts::Vector3(0, 1, 0);
    rotation.col(2) = Acts::Vector3(1, 0, 0);
  } else if (binValue == Acts::BinningValue::binY) {
    rotation.col(0) = Acts::Vector3(1, 0, 0);
    rotation.col(1) = Acts::Vector3(0, 0, -1);
    rotation.col(2) = Acts::Vector3(0, 1, 0);
  }

  // Construct the surfaces and layers
  std::size_t nLayers = positions.size();
  std::vector<Acts::LayerPtr> layers(nLayers);
  for (unsigned int i = 0; i < nLayers; ++i) {
    // The translation without rotation yet
    Acts::Translation3 trans(offsets[0], offsets[1], positions[i]);
    // The entire transformation (the coordinate system, whose center is defined
    // by trans, will be rotated as well)
    Acts::Transform3 trafo(rotation * trans);

    // rotate around local z axis by stereo angle
    auto stereo = stereoAngles[i];
    trafo *= Acts::AngleAxis3(stereo, Acts::Vector3::UnitZ());

    // Create the detector element
    std::shared_ptr<TelescopeDetectorElement> detElement = nullptr;
    if (surfaceType == TelescopeSurfaceType::Plane) {
      detElement = std::make_shared<TelescopeDetectorElement>(
          std::make_shared<const Acts::Transform3>(trafo), pBounds, 1._um,
          surfaceMaterial);
    } else {
      detElement = std::make_shared<TelescopeDetectorElement>(
          std::make_shared<const Acts::Transform3>(trafo), rBounds, 1._um,
          surfaceMaterial);
    }
    // Get the surface
    auto surface = detElement->surface().getSharedPtr();
    // Add the detector element to the detector store
    detectorStore.push_back(std::move(detElement));
    // Construct the surface array (one surface contained)
    std::unique_ptr<Acts::SurfaceArray> surArray(
        new Acts::SurfaceArray(surface));
    // Construct the layer
    if (surfaceType == TelescopeSurfaceType::Plane) {
      layers[i] =
          Acts::PlaneLayer::create(trafo, pBounds, std::move(surArray), 1._mm);
    } else {
      layers[i] =
          Acts::DiscLayer::create(trafo, rBounds, std::move(surArray), 1._mm);
    }
    // Associate the layer to the surface
    auto mutableSurface = const_cast<Acts::Surface*>(surface.get());
    mutableSurface->associateLayer(*layers[i]);
  }

  // The volume transform
  Acts::Translation3 transVol(offsets[0], offsets[1],
                              (positions.front() + positions.back()) * 0.5);
  Acts::Transform3 trafoVol(rotation * transVol);

  // The volume bounds is set to be a bit larger than either cubic with planes
  // or cylinder with discs
  auto length = positions.back() - positions.front();
  std::shared_ptr<Acts::VolumeBounds> boundsVol = nullptr;
  if (surfaceType == TelescopeSurfaceType::Plane) {
    boundsVol = std::make_shared<Acts::CuboidVolumeBounds>(
        bounds[0] + 5._mm, bounds[1] + 5._mm, length + 10._mm);
  } else {
    boundsVol = std::make_shared<Acts::CylinderVolumeBounds>(
        std::max(bounds[0] - 5.0_mm, 0.), bounds[1] + 5._mm, length + 10._mm);
  }

  Acts::LayerArrayCreator::Config lacConfig;
  Acts::LayerArrayCreator layArrCreator(
      lacConfig,
      Acts::getDefaultLogger("LayerArrayCreator", Acts::Logging::INFO));
  Acts::LayerVector layVec;
  for (unsigned int i = 0; i < nLayers; i++) {
    layVec.push_back(layers[i]);
  }
  // Create the layer array
  Acts::GeometryContext genGctx{gctx};
  std::unique_ptr<const Acts::LayerArray> layArr(layArrCreator.layerArray(
      genGctx, layVec, positions.front() - 2._mm, positions.back() + 2._mm,
      Acts::BinningType::arbitrary, binValue));

  // Build the tracking volume
  auto trackVolume = std::make_shared<Acts::TrackingVolume>(
      trafoVol, boundsVol, nullptr, std::move(layArr), nullptr,
      Acts::MutableTrackingVolumeVector{}, "Telescope");

  // Build and return tracking geometry
  return std::make_unique<Acts::TrackingGeometry>(trackVolume);
}
