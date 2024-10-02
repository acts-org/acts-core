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
#include "Acts/Geometry/CylinderLayer.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBuilder.hpp"
#include "Acts/Geometry/CylinderVolumeHelper.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/PassiveLayerBuilder.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/TrackingVolumeArrayCreator.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"

#include <functional>
#include <vector>

namespace Acts::Test {

struct CylindricalTrackingGeometry {
  std::reference_wrapper<const GeometryContext> geoContext;

  /// Only allowed constructor with reference wrapper
  CylindricalTrackingGeometry(const GeometryContext& gctx) : geoContext(gctx) {}

  using DetectorStore = std::vector<std::unique_ptr<const DetectorElementStub>>;

  /// The detector store for memory management
  DetectorStore detectorStore = {};

  /// Generator of surfaces for a ring
  ///
  /// @param detStore The DetectorStore for storing the modules
  /// @param moduleHalfXminY The half length in X (at Y min) of the module
  /// @param moduleHalfXmaxY The half length in X (at Y max) of the module
  /// @param moduleHalfY The half length in Y of the module
  /// @param moduleThickness The module thickness
  /// @param moduleTilt The tilt out of the plane for discs
  /// @param ringRadius The central radius of the ring
  /// @param ringZ The z position of the ring
  /// @param zStagger The z offset of phi modules
  /// @param nPhi The number of phi modules
  ///
  /// @return A vector of Surfaces
  std::vector<const Surface*> surfacesRing(
      DetectorStore& detStore, double moduleHalfXminY, double moduleHalfXmaxY,
      double moduleHalfY, double moduleThickness, double moduleTilt,
      double ringRadius, double ringZ, double zStagger, int nPhi) {
    std::vector<const Surface*> layerSurfaces;

    // Module material from input
    MaterialSlab moduleMaterial(makeSilicon(), moduleThickness);

    // Create a new surface material
    std::shared_ptr<const ISurfaceMaterial> moduleMaterialPtr =
        std::shared_ptr<const ISurfaceMaterial>(
            new Acts::HomogeneousSurfaceMaterial(moduleMaterial));

    // The rectangle/trapezoid bounds for all modules
    std::shared_ptr<PlanarBounds> mBounds = nullptr;
    if (moduleHalfXminY == moduleHalfXmaxY) {
      mBounds = std::make_shared<RectangleBounds>(moduleHalfXminY, moduleHalfY);
    } else {
      mBounds = std::make_shared<TrapezoidBounds>(moduleHalfXminY,
                                                  moduleHalfXmaxY, moduleHalfY);
    }

    double phiStep = 2 * M_PI / nPhi;

    for (int im = 0; im < nPhi; ++im) {
      // Get the moduleTransform
      double phi = -M_PI + im * phiStep;
      auto mModuleTransform = Transform3(
          Translation3(ringRadius * std::cos(phi), ringRadius * std::sin(phi),
                       ringZ + (im % 2) * zStagger) *
          AngleAxis3(phi - 0.5 * M_PI, Vector3::UnitZ()) *
          AngleAxis3(moduleTilt, Vector3::UnitY()));

      // Create the detector element
      auto detElement = std::make_unique<const DetectorElementStub>(
          mModuleTransform, mBounds, moduleThickness, moduleMaterialPtr);

      layerSurfaces.push_back(&detElement->surface());
      detStore.push_back(std::move(detElement));
    }

    return layerSurfaces;
  }

  /// Generator of surfaces for a cylindrical layer
  ///
  /// @param detStore The DetectorStore for storing the modules
  /// @param moduleHalfX The half length in X of the module
  /// @param moduleHalfY The half length in Y of the module
  /// @param moduleThickness The module thickness
  /// @param moduleTilePhi The tilt in phi direction of the module
  /// @param layerRadius The radius of the cylindrical layer
  /// @param radialStagger The radial delta of modules next in z
  /// @param longitudinalOverlap The z overlap of modules next in z
  /// @param binningSchema The number of bins in phi/z
  ///
  /// @return A vector of Surfaces
  std::vector<const Surface*> surfacesCylinder(
      DetectorStore& detStore, double moduleHalfX, double moduleHalfY,
      double moduleThickness, double moduleTiltPhi, double layerRadius,
      double radialStagger, double longitudinalOverlap,
      const std::pair<int, int>& binningSchema) {
    std::vector<const Surface*> layerSurfaces;

    // Module material from input
    MaterialSlab moduleMaterial(makeSilicon(), moduleThickness);

    // Create a new surface material
    std::shared_ptr<const ISurfaceMaterial> moduleMaterialPtr =
        std::shared_ptr<const ISurfaceMaterial>(
            new Acts::HomogeneousSurfaceMaterial(moduleMaterial));

    // The rectangle bounds for all modules
    auto mBounds = std::make_shared<RectangleBounds>(moduleHalfX, moduleHalfY);

    // Create the module centers
    auto moduleCenters =
        modulePositionsCylinder(layerRadius, radialStagger, moduleHalfY,
                                longitudinalOverlap, binningSchema);

    for (auto& mCenter : moduleCenters) {
      // The association transform
      double modulePhi = VectorHelpers::phi(mCenter);
      // Local z axis is the normal vector
      Vector3 moduleLocalZ(cos(modulePhi + moduleTiltPhi),
                           sin(modulePhi + moduleTiltPhi), 0.);
      // Local y axis is the global z axis
      Vector3 moduleLocalY(0., 0., 1);
      // Local x axis the normal to local y,z
      Vector3 moduleLocalX(-sin(modulePhi + moduleTiltPhi),
                           cos(modulePhi + moduleTiltPhi), 0.);
      // Create the RotationMatrix
      RotationMatrix3 moduleRotation;
      moduleRotation.col(0) = moduleLocalX;
      moduleRotation.col(1) = moduleLocalY;
      moduleRotation.col(2) = moduleLocalZ;
      // Get the moduleTransform
      auto mModuleTransform =
          Transform3(Translation3(mCenter) * moduleRotation);
      // Create the detector element
      auto detElement = std::make_unique<const DetectorElementStub>(
          mModuleTransform, mBounds, moduleThickness, moduleMaterialPtr);

      layerSurfaces.push_back(&detElement->surface());
      detStore.push_back(std::move(detElement));
    }
    return layerSurfaces;
  }

  /// Helper method for cylinder layer
  /// create the positions for module surfaces on a cylinder
  std::vector<Vector3> modulePositionsCylinder(
      double radius, double zStagger, double moduleHalfLength, double lOverlap,
      const std::pair<int, int>& binningSchema) {
    int nPhiBins = binningSchema.first;
    int nZbins = binningSchema.second;
    // prepare the return value
    std::vector<Vector3> mPositions;
    mPositions.reserve(nPhiBins * nZbins);
    // prep work
    double phiStep = 2 * M_PI / (nPhiBins);
    double minPhi = -M_PI + 0.5 * phiStep;
    double zStart = -0.5 * (nZbins - 1) * (2 * moduleHalfLength - lOverlap);
    double zStep = 2 * std::abs(zStart) / (nZbins - 1);
    // loop over the bins
    for (std::size_t zBin = 0; zBin < static_cast<std::size_t>(nZbins);
         ++zBin) {
      // prepare z and r
      double moduleZ = zStart + zBin * zStep;
      double moduleR =
          (zBin % 2) != 0u ? radius - 0.5 * zStagger : radius + 0.5 * zStagger;
      for (std::size_t phiBin = 0; phiBin < static_cast<std::size_t>(nPhiBins);
           ++phiBin) {
        // calculate the current phi value
        double modulePhi = minPhi + phiBin * phiStep;
        mPositions.push_back(Vector3(moduleR * cos(modulePhi),
                                     moduleR * sin(modulePhi), moduleZ));
      }
    }
    return mPositions;
  }

  // @brief Call operator for the creation method of the tracking geometry
  std::shared_ptr<const TrackingGeometry> operator()() {
    using namespace Acts::UnitLiterals;

    Logging::Level surfaceLLevel = Logging::INFO;
    Logging::Level layerLLevel = Logging::INFO;
    Logging::Level volumeLLevel = Logging::INFO;

    // configure surface array creator
    auto surfaceArrayCreator = std::make_shared<const SurfaceArrayCreator>(
        getDefaultLogger("SurfaceArrayCreator", surfaceLLevel));
    // configure the layer creator that uses the surface array creator
    LayerCreator::Config lcConfig;
    lcConfig.surfaceArrayCreator = surfaceArrayCreator;
    auto layerCreator = std::make_shared<const LayerCreator>(
        lcConfig, getDefaultLogger("LayerCreator", layerLLevel));
    // configure the layer array creator
    LayerArrayCreator::Config lacConfig;
    auto layerArrayCreator = std::make_shared<const LayerArrayCreator>(
        lacConfig, getDefaultLogger("LayerArrayCreator", layerLLevel));

    // tracking volume array creator
    TrackingVolumeArrayCreator::Config tvacConfig;
    auto tVolumeArrayCreator =
        std::make_shared<const TrackingVolumeArrayCreator>(
            tvacConfig,
            getDefaultLogger("TrackingVolumeArrayCreator", volumeLLevel));
    // configure the cylinder volume helper
    CylinderVolumeHelper::Config cvhConfig;
    cvhConfig.layerArrayCreator = layerArrayCreator;
    cvhConfig.trackingVolumeArrayCreator = tVolumeArrayCreator;
    auto cylinderVolumeHelper = std::make_shared<const CylinderVolumeHelper>(
        cvhConfig, getDefaultLogger("CylinderVolumeHelper", volumeLLevel));

    // ----------------- build a beam pipe -----------------------------------
    MaterialSlab beamPipeMaterial(makeBeryllium(), 0.8_mm);
    PassiveLayerBuilder::Config bplConfig;
    bplConfig.layerIdentification = "BeamPipe";
    bplConfig.centralLayerRadii = std::vector<double>(1, 19.);
    bplConfig.centralLayerHalflengthZ = std::vector<double>(1, 1000.);
    bplConfig.centralLayerThickness = std::vector<double>(1, 0.8);
    bplConfig.centralLayerMaterial = {
        std::make_shared<const HomogeneousSurfaceMaterial>(beamPipeMaterial)};
    auto beamPipeBuilder = std::make_shared<const PassiveLayerBuilder>(
        bplConfig, getDefaultLogger("BeamPipeLayerBuilder", layerLLevel));
    // create the volume for the beam pipe
    CylinderVolumeBuilder::Config bpvConfig;
    bpvConfig.trackingVolumeHelper = cylinderVolumeHelper;
    bpvConfig.volumeName = "BeamPipe";
    bpvConfig.layerBuilder = beamPipeBuilder;
    bpvConfig.layerEnvelopeR = {1_mm, 1_mm};
    bpvConfig.buildToRadiusZero = true;
    auto beamPipeVolumeBuilder = std::make_shared<const CylinderVolumeBuilder>(
        bpvConfig, getDefaultLogger("BeamPipeVolumeBuilder", volumeLLevel));

    // create the bounds and the volume
    auto beamPipeBounds =
        std::make_shared<const CylinderVolumeBounds>(0., 25., 1100.);
    auto beamPipeVolume = beamPipeVolumeBuilder->trackingVolume(
        geoContext, nullptr, beamPipeBounds);

    //-------------------------------------------------------------------------------------
    // some prep work for the material
    // Layer material properties - thickness, X0, L0, A, Z, Rho
    MaterialSlab lProperties(makeSilicon(), 1.5_mm);

    std::shared_ptr<const ISurfaceMaterial> layerMaterialPtr =
        std::shared_ptr<const ISurfaceMaterial>(
            new Acts::HomogeneousSurfaceMaterial(lProperties));

    std::vector<double> pLayerRadii = {32., 72., 116., 172.};
    std::vector<std::pair<int, int>> pLayerBinning = {
        {16, 14}, {32, 14}, {52, 14}, {78, 14}};
    std::vector<double> pModuleTiltPhi = {0.145, 0.145, 0.145, 0.145};
    std::vector<double> pModuleHalfX = {8.4, 8.4, 8.4, 8.4};
    std::vector<double> pModuleHalfY = {36., 36., 36., 36.};
    std::vector<double> pModuleThickness = {0.15, 0.15, 0.15, 0.15};

    std::vector<LayerPtr> pLayers;

    for (std::size_t ilp = 0; ilp < pLayerRadii.size(); ++ilp) {
      std::vector<const Surface*> layerSurfaces =
          surfacesCylinder(detectorStore, pModuleHalfX[ilp], pModuleHalfY[ilp],
                           pModuleThickness[ilp], pModuleTiltPhi[ilp],
                           pLayerRadii[ilp], 2_mm, 5_mm, pLayerBinning[ilp]);

      // Make a shared version out of it
      std::vector<std::shared_ptr<const Surface>> layerSurfacePtrs;
      layerSurfacePtrs.reserve(layerSurfaces.size());
      for (auto& sf : layerSurfaces) {
        layerSurfacePtrs.push_back(sf->getSharedPtr());
      }

      // create the layer and store it
      ProtoLayer protoLayer(geoContext, layerSurfaces);
      protoLayer.envelope[BinningValue::binR] = {0.5, 0.5};
      auto pLayer = layerCreator->cylinderLayer(
          geoContext, std::move(layerSurfacePtrs), pLayerBinning[ilp].first,
          pLayerBinning[ilp].second, protoLayer);
      auto approachSurfaces = pLayer->approachDescriptor()->containedSurfaces();
      auto mutableOuterSurface =
          const_cast<Acts::Surface*>(approachSurfaces.at(1));
      mutableOuterSurface->assignSurfaceMaterial(layerMaterialPtr);
      /// now push back the layer
      pLayers.push_back(pLayer);

    }  // loop over layers

    // layer array
    auto pLayerArray = layerArrayCreator->layerArray(
        geoContext, pLayers, 25., 300., arbitrary, BinningValue::binR);
    auto pVolumeBounds =
        std::make_shared<CylinderVolumeBounds>(25., 300., 1100.);
    // create the Tracking volume
    auto pVolume = std::make_shared<TrackingVolume>(
        Transform3::Identity(), pVolumeBounds, nullptr, std::move(pLayerArray),
        nullptr, MutableTrackingVolumeVector{}, "Pixel::Barrel");

    // The combined volume
    auto detectorVolume = cylinderVolumeHelper->createContainerTrackingVolume(
        geoContext, {beamPipeVolume, pVolume});

    // create and return the geometry
    return std::make_shared<const TrackingGeometry>(detectorVolume);
  }
};

}  // namespace Acts::Test
