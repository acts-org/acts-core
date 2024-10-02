// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/CylinderVolumeHelper.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/BoundarySurfaceT.hpp"
#include "Acts/Geometry/CylinderLayer.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/DiscLayer.hpp"
#include "Acts/Geometry/GlueVolumesDescriptor.hpp"
#include "Acts/Geometry/ILayerArrayCreator.hpp"
#include "Acts/Geometry/ITrackingVolumeArrayCreator.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinnedArray.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <utility>

namespace Acts {
class DiscBounds;
}  // namespace Acts

Acts::CylinderVolumeHelper::CylinderVolumeHelper(
    const Acts::CylinderVolumeHelper::Config& cvhConfig,
    std::unique_ptr<const Logger> logger)
    : Acts::ITrackingVolumeHelper(), m_cfg(), m_logger(std::move(logger)) {
  setConfiguration(cvhConfig);
}

// configuration
void Acts::CylinderVolumeHelper::setConfiguration(
    const Acts::CylinderVolumeHelper::Config& cvhConfig) {
  // @todo check consistency
  // copy the configuration
  m_cfg = cvhConfig;
}

void Acts::CylinderVolumeHelper::setLogger(
    std::unique_ptr<const Logger> newLogger) {
  m_logger = std::move(newLogger);
}

std::shared_ptr<Acts::TrackingVolume>
Acts::CylinderVolumeHelper::createTrackingVolume(
    const GeometryContext& gctx, const LayerVector& layers,
    std::shared_ptr<const IVolumeMaterial> volumeMaterial,
    std::shared_ptr<VolumeBounds> volumeBounds,
    MutableTrackingVolumeVector mtvVector, const Transform3& transform,
    const std::string& volumeName, BinningType bType) const {
  // the final one to build / sensitive Volume / Bounds
  MutableTrackingVolumePtr tVolume = nullptr;
  // the layer array
  std::unique_ptr<const LayerArray> layerArray = nullptr;

  // Cases are:
  // (1) volumeBounds && transform : use both information
  // (2) volumeBounds && transform==identity : centered around 0, but with
  //     given bounds
  // (3) !volumeBounds && transform : estimate size from layers,
  //     use transform
  // (4) !volumeBounds && transform==identity : estimate size &
  //     translation from layers
  bool idTrf = transform.isApprox(Transform3::Identity());

  auto cylinderBounds =
      std::dynamic_pointer_cast<CylinderVolumeBounds>(volumeBounds);
  // this is the implementation of CylinderVolumeHelper
  if (volumeBounds != nullptr && cylinderBounds == nullptr) {
    ACTS_WARNING(
        "[!] Problem: given bounds are not cylindrical - return nullptr");
    return tVolume;
  }
  // this is only needed if layers are provided
  if (!layers.empty()) {
    // the raw data
    double rMinRaw = 0.;
    double rMaxRaw = 0.;
    double zMinRaw = 0.;
    double zMaxRaw = 0.;

    BinningValue bValue = BinningValue::binR;

    // check the dimension and fill raw data
    if (!estimateAndCheckDimension(gctx, layers, cylinderBounds, transform,
                                   rMinRaw, rMaxRaw, zMinRaw, zMaxRaw, bValue,
                                   bType)) {
      ACTS_WARNING(
          "[!] Problem with given dimensions - return nullptr and "
          "delete provided objects");
      return tVolume;
    }
    // we might have overwritten the bounds in estimateAndCheckDimension
    volumeBounds = cylinderBounds;
    // get the zMin/Max
    double zMin =
        (!idTrf ? transform.translation().z() : 0.) +
        (cylinderBounds != nullptr
             ? -cylinderBounds->get(CylinderVolumeBounds::eHalfLengthZ)
             : 0.);
    double zMax = (!idTrf ? transform.translation().z() : 0.) +
                  (cylinderBounds != nullptr
                       ? cylinderBounds->get(CylinderVolumeBounds::eHalfLengthZ)
                       : 0.);
    // get the rMin/rmAx
    double rMin = cylinderBounds != nullptr
                      ? cylinderBounds->get(CylinderVolumeBounds::eMinR)
                      : rMinRaw;
    double rMax = cylinderBounds != nullptr
                      ? cylinderBounds->get(CylinderVolumeBounds::eMaxR)
                      : rMaxRaw;

    ACTS_VERBOSE(
        "Filling the layers into an appropriate layer array - with "
        "binningValue = "
        << bValue);

    // create the Layer Array
    layerArray = (bValue == BinningValue::binR)
                     ? m_cfg.layerArrayCreator->layerArray(gctx, layers, rMin,
                                                           rMax, bType, bValue)
                     : m_cfg.layerArrayCreator->layerArray(gctx, layers, zMin,
                                                           zMax, bType, bValue);

  }  // layers are created and done
  // finally create the TrackingVolume
  tVolume = std::make_shared<TrackingVolume>(
      transform, volumeBounds, volumeMaterial, std::move(layerArray), nullptr,
      mtvVector, volumeName);
  // screen output
  ACTS_VERBOSE(
      "Created cylindrical volume at z-position :" << tVolume->center().z());
  ACTS_VERBOSE("   created bounds : " << tVolume->volumeBounds());
  // return the constructed TrackingVolume
  return tVolume;
}

std::shared_ptr<Acts::TrackingVolume>
Acts::CylinderVolumeHelper::createTrackingVolume(
    const GeometryContext& gctx, const LayerVector& layers,
    MutableTrackingVolumeVector mtvVector,
    std::shared_ptr<const IVolumeMaterial> volumeMaterial, double rMin,
    double rMax, double zMin, double zMax, const std::string& volumeName,
    BinningType bType) const {
  // Screen output
  ACTS_VERBOSE("Create cylindrical TrackingVolume '" << volumeName << "'.");
  ACTS_VERBOSE("    -> with given dimensions of (rMin/rMax/zMin/Max) = "
               << rMin << " / " << rMax << " / " << zMin << " / " << zMax);

  // check for consistency
  if (zMin > zMax || rMin > rMax) {
    ACTS_WARNING("Inconsistent dimensions given :"
                 << ((zMin > zMax) ? " zMin > zMax (" : " rMin > rMax (")
                 << ((zMin > zMax) ? zMin : rMin) << " > "
                 << ((zMin > zMax) ? zMax : rMax) << " ) - return 0");
    return nullptr;
  }

  // create a Transform3 and VolumeBounds out of the zMin/zMax
  double halflengthZ = 0.5 * (zMax - zMin);
  double zPosition = 0.5 * (zMin + zMax);
  zPosition = std::abs(zPosition) < 0.1 ? 0. : zPosition;

  // now create the cylinder volume bounds
  auto cBounds =
      std::make_shared<CylinderVolumeBounds>(rMin, rMax, halflengthZ);

  // transform
  const Transform3 transform = Transform3(Translation3(0., 0., zPosition));
  // call to the creation method with Bounds & Translation3
  return createTrackingVolume(gctx, layers, volumeMaterial, cBounds, mtvVector,
                              transform, volumeName, bType);
}

std::shared_ptr<Acts::TrackingVolume>
Acts::CylinderVolumeHelper::createGapTrackingVolume(
    const GeometryContext& gctx, MutableTrackingVolumeVector& mtvVector,
    std::shared_ptr<const IVolumeMaterial> volumeMaterial, double rMin,
    double rMax, double zMin, double zMax, unsigned int materialLayers,
    bool cylinder, const std::string& volumeName) const {
  // screen output
  ACTS_VERBOSE("Create cylindrical gap TrackingVolume '"
               << volumeName << "' with (rMin/rMax/zMin/Max) = ");
  ACTS_VERBOSE("\t" << rMin << " / " << rMax << " / " << zMin << " / " << zMax);

  // assign min/max
  double min = cylinder ? rMin : zMin;
  double max = cylinder ? rMax : zMax;

  // create the layer r/z positions
  std::vector<double> layerPositions;
  if (materialLayers > 1) {
    double step = (max - min) / (materialLayers - 1);
    for (unsigned int il = 0; il < materialLayers; ++il) {
      layerPositions.push_back(min + il * step);
    }
  } else {
    layerPositions.push_back(0.5 * (min + max));
  }

  // now call the main method
  return createGapTrackingVolume(gctx, mtvVector, volumeMaterial, rMin, rMax,
                                 zMin, zMax, layerPositions, cylinder,
                                 volumeName, arbitrary);
}

std::shared_ptr<Acts::TrackingVolume>
Acts::CylinderVolumeHelper::createGapTrackingVolume(
    const GeometryContext& gctx, MutableTrackingVolumeVector& mtvVector,
    std::shared_ptr<const IVolumeMaterial> volumeMaterial, double rMin,
    double rMax, double zMin, double zMax,
    const std::vector<double>& layerPositions, bool cylinder,
    const std::string& volumeName, BinningType bType) const {
  // screen output
  ACTS_VERBOSE("Create cylindrical gap TrackingVolume '"
               << volumeName << "' with (rMin/rMax/zMin/Max) = ");
  ACTS_VERBOSE("\t" << rMin << " / " << rMax << " / " << zMin << " / " << zMax);

  // create the layers
  LayerVector layers;
  layers.reserve(layerPositions.size());

  std::vector<double>::const_iterator layerPropIter = layerPositions.begin();
  std::vector<double>::const_iterator layerPropEnd = layerPositions.end();
  for (; layerPropIter != layerPropEnd; ++layerPropIter) {
    // create cylinder layers
    if (cylinder) {
      // take envelopes into account
      double zMinLayer = zMin;
      double zMaxLayer = zMax;
      // create the layer
      layers.push_back(createCylinderLayer(
          0.5 * (zMinLayer + zMaxLayer), (*layerPropIter),
          std::abs(0.5 * (zMaxLayer - zMinLayer)), m_cfg.passiveLayerThickness,
          m_cfg.passiveLayerPhiBins, m_cfg.passiveLayerRzBins));

    } else {
      // take the envelopes into account
      double rMinLayer = rMin;
      double rMaxLayer = rMax;
      // create the layer
      layers.push_back(createDiscLayer(
          (*layerPropIter), rMinLayer, rMaxLayer, m_cfg.passiveLayerThickness,
          m_cfg.passiveLayerPhiBins, m_cfg.passiveLayerRzBins));
    }
  }
  // now call the createTrackingVolume() method
  return createTrackingVolume(gctx, layers, mtvVector, volumeMaterial, rMin,
                              rMax, zMin, zMax, volumeName, bType);
}

std::shared_ptr<Acts::TrackingVolume>
Acts::CylinderVolumeHelper::createContainerTrackingVolume(
    const GeometryContext& gctx, const TrackingVolumeVector& volumes) const {
  // check if you have more than one volume
  if (volumes.size() <= std::size_t{1}) {
    ACTS_WARNING(
        "None (only one) TrackingVolume given to create container "
        "volume (min required: 2) - returning 0 ");
    return nullptr;
  }
  // screen output
  std::string volumeName = "{ ";
  ACTS_VERBOSE("[start] Creating a container volume with " << volumes.size()
                                                           << " sub volumes:");
  // volumes need to be sorted in either r or z - both increasing
  // set the iterator to the volumes, the first and the end
  auto firstVolume = volumes.begin();
  auto lastVolume = volumes.end();

  for (std::size_t ivol = 0; firstVolume != lastVolume; ++firstVolume, ++ivol) {
    if (*firstVolume == nullptr) {
      ACTS_ERROR("Volume " << ivol << " is nullptr, return nullptr");
      return nullptr;
    }
    ACTS_VERBOSE("   - volume (" << ivol
                                 << ") is : " << (*firstVolume)->volumeName());
    ACTS_VERBOSE("     at position : " << (*firstVolume)->center().x() << ", "
                                       << (*firstVolume)->center().y() << ", "
                                       << (*firstVolume)->center().z());

    ACTS_VERBOSE("     with bounds : " << (*firstVolume)->volumeBounds());
    // put the name together
    volumeName += (*firstVolume)->volumeName();
    if (ivol + 1 < volumes.size()) {
      volumeName += " | ";
    }
  }
  // close the volume name
  volumeName += " }";
  // reset the iterator -----
  firstVolume = volumes.begin();
  --lastVolume;  // set to the last volume

  if (firstVolume == lastVolume) {
    ACTS_WARNING(
        "Only one TrackingVolume given to create Top level volume "
        "(min required: 2) - returning 0 ");
    return nullptr;
  }
  // get the bounds
  const CylinderVolumeBounds* firstVolumeBounds =
      dynamic_cast<const CylinderVolumeBounds*>(
          &((*firstVolume)->volumeBounds()));
  const CylinderVolumeBounds* lastVolumeBounds =
      dynamic_cast<const CylinderVolumeBounds*>(
          &((*lastVolume)->volumeBounds()));
  // check the dynamic cast
  if ((firstVolumeBounds == nullptr) || (lastVolumeBounds == nullptr)) {
    ACTS_WARNING(
        "VolumeBounds given are not of type: CylinderVolumeBounds "
        "(required) - returning 0 ");
    return nullptr;
  }
  // Check whether it is an r-binned case or a z-binned case
  bool rCase =
      std::abs(firstVolumeBounds->get(CylinderVolumeBounds::eMinR) -
               lastVolumeBounds->get(CylinderVolumeBounds::eMinR)) > 0.1;

  // Fill these ones depending on the rCase though assignment
  double zMin = 0.;
  double zMax = 0.;
  double rMin = 0.;
  double rGlueMin = 0.;
  double rMax = 0.;
  double zSep1 = 0.;
  double zSep2 = 0.;
  if (rCase) {
    zMin = (*firstVolume)->center().z() -
           firstVolumeBounds->get(CylinderVolumeBounds::eHalfLengthZ);
    zMax = (*firstVolume)->center().z() +
           firstVolumeBounds->get(CylinderVolumeBounds::eHalfLengthZ);
    zSep1 = zMin;
    zSep2 = zMax;
    rMin = firstVolumeBounds->get(CylinderVolumeBounds::eMinR);
    rGlueMin = firstVolumeBounds->get(CylinderVolumeBounds::eMaxR);
    rMax = lastVolumeBounds->get(CylinderVolumeBounds::eMaxR);
  } else {
    zMin = (*firstVolume)->center().z() -
           firstVolumeBounds->get(CylinderVolumeBounds::eHalfLengthZ);
    zMax = (*lastVolume)->center().z() +
           lastVolumeBounds->get(CylinderVolumeBounds::eHalfLengthZ);
    zSep1 = (*firstVolume)->center().z() +
            firstVolumeBounds->get(CylinderVolumeBounds::eHalfLengthZ);
    zSep2 = zSep1;
    rMin = firstVolumeBounds->get(CylinderVolumeBounds::eMinR);
    rMax = firstVolumeBounds->get(CylinderVolumeBounds::eMaxR);
  }
  // Estimate the z - position
  double zPos = 0.5 * (zMin + zMax);
  // Create the transform from the stuff known so far
  const Transform3 topVolumeTransform = Transform3(Translation3(0., 0., zPos));
  // Create the bounds from the information gathered so far
  auto topVolumeBounds = std::make_shared<CylinderVolumeBounds>(
      rMin, rMax, 0.5 * std::abs(zMax - zMin));

  // some screen output
  ACTS_VERBOSE("Container volume bounds are " << (*topVolumeBounds));

  // create the volume array with the ITrackingVolumeArrayCreator
  std::shared_ptr<const TrackingVolumeArray> volumeArray =
      (rCase) ? m_cfg.trackingVolumeArrayCreator->trackingVolumeArray(
                    gctx, volumes, BinningValue::binR)
              : m_cfg.trackingVolumeArrayCreator->trackingVolumeArray(
                    gctx, volumes, BinningValue::binZ);
  if (volumeArray == nullptr) {
    ACTS_WARNING(
        "Creation of TrackingVolume array did not succeed - returning 0 ");
    return nullptr;
  }
  // we have the bounds and the volume array, create the volume
  std::shared_ptr<TrackingVolume> topVolume = std::make_shared<TrackingVolume>(
      topVolumeTransform, topVolumeBounds, nullptr, nullptr, volumeArray,
      MutableTrackingVolumeVector{}, volumeName);
  // glueing section
  // --------------------------------------------------------------------------------------
  if (!interGlueTrackingVolume(gctx, topVolume, rCase, rMin, rGlueMin, rMax,
                               zSep1, zSep2)) {
    ACTS_WARNING(
        "Problem with inter-glueing of TrackingVolumes (needed) - "
        "returning 0 ");
    return nullptr;
  }

  ACTS_VERBOSE(
      "[ end ] return newly created container : " << topVolume->volumeName());

  return topVolume;
}

/** private helper method to estimate and check the dimensions of a tracking
 * volume */
bool Acts::CylinderVolumeHelper::estimateAndCheckDimension(
    const GeometryContext& gctx, const LayerVector& layers,
    std::shared_ptr<CylinderVolumeBounds>& cylinderVolumeBounds,
    const Transform3& transform, double& rMinClean, double& rMaxClean,
    double& zMinClean, double& zMaxClean, BinningValue& bValue,
    BinningType /*bType*/) const {
  // some verbose output

  ACTS_VERBOSE("Parsing the " << layers.size()
                              << " layers to gather overall dimensions");
  if (cylinderVolumeBounds != nullptr) {
    ACTS_VERBOSE(
        "Cylinder volume bounds are given: (rmin/rmax/dz) = "
        << "(" << cylinderVolumeBounds->get(CylinderVolumeBounds::eMinR) << "/"
        << cylinderVolumeBounds->get(CylinderVolumeBounds::eMaxR) << "/"
        << cylinderVolumeBounds->get(CylinderVolumeBounds::eHalfLengthZ)
        << ")");
  }

  // prepare for parsing the layers
  double layerRmin = 10e10;
  double layerRmax = 0.;
  double layerZmin = 10e10;
  double layerZmax = -10e10;
  bool radial = false;

  rMinClean = 10e10;
  rMaxClean = 0.;
  zMinClean = 10e10;
  zMaxClean = -10e10;

  // find out what is there
  for (auto& layerIter : layers) {
    // initialize
    double currentRmin = 0.;
    double currentRmax = 0.;
    double currentZmin = 0.;
    double currentZmax = 0.;
    // dynamic cast the bounds either to CylinderBounds or DiscBounds
    const CylinderBounds* cylBounds = dynamic_cast<const CylinderBounds*>(
        &(layerIter->surfaceRepresentation()).bounds());
    // cylinder bounds
    if (cylBounds != nullptr) {
      radial = true;
      // get the raw data
      double currentR = cylBounds->get(CylinderBounds::eR);
      double centerZ = (layerIter->surfaceRepresentation()).center(gctx).z();
      // check for min/max in the cylinder bounds case
      currentRmin = currentR - (0.5 * (layerIter)->thickness());
      currentRmax = currentR + (0.5 * (layerIter)->thickness());
      currentZmin = centerZ - cylBounds->get(CylinderBounds::eHalfLengthZ);
      currentZmax = centerZ + cylBounds->get(CylinderBounds::eHalfLengthZ);
    }
    // dynamic cast to the DiscBounds
    const RadialBounds* discBounds = dynamic_cast<const RadialBounds*>(
        &(layerIter->surfaceRepresentation()).bounds());
    if (discBounds != nullptr) {
      // check for min/max in the cylinder bounds case
      double centerZ = (layerIter->surfaceRepresentation()).center(gctx).z();
      currentRmin = discBounds->rMin();
      currentRmax = discBounds->rMax();
      currentZmin = centerZ - (0.5 * (layerIter)->thickness());
      currentZmax = centerZ + (0.5 * (layerIter)->thickness());
    }
    // the raw data
    rMinClean = std::min(rMinClean, currentRmin);
    rMaxClean = std::max(rMaxClean, currentRmax);
    zMinClean = std::min(zMinClean, currentZmin);
    zMaxClean = std::max(zMaxClean, currentZmax);
    // assign if they overrule the minima/maxima (with layers thicknesses)
    layerRmin = std::min(layerRmin, currentRmin);
    layerRmax = std::max(layerRmax, currentRmax);
    layerZmin = std::min(layerZmin, currentZmin);
    layerZmax = std::max(layerZmax, currentZmax);
  }

  // set the binning value
  bValue = radial ? BinningValue::binR : BinningValue::binZ;

  ACTS_VERBOSE(
      "Estimate/check CylinderVolumeBounds from/w.r.t. enclosed "
      "layers + envelope covers");
  // the z from the layers w and w/o envelopes
  double zEstFromLayerEnv = 0.5 * (layerZmax + layerZmin);
  double halflengthFromLayer = 0.5 * std::abs(layerZmax - layerZmin);

  // using `sqrt(0.001) = 0.031622777` because previously it compared to
  // `zEstFromLayerEnv * zEstFromLayerEnv < 0.001` which was changed due to
  // underflow/overflow concerns
  bool concentric = std::abs(zEstFromLayerEnv) < 0.031622777;

  bool idTrf = transform.isApprox(Transform3::Identity());

  Transform3 vtransform = Transform3::Identity();
  // no CylinderBounds and Translation given - make it
  if ((cylinderVolumeBounds == nullptr) && idTrf) {
    // create the CylinderBounds from parsed layer inputs
    cylinderVolumeBounds = std::make_shared<CylinderVolumeBounds>(
        layerRmin, layerRmax, halflengthFromLayer);
    // and the transform
    vtransform = concentric ? Transform3(Translation3(0., 0., zEstFromLayerEnv))
                            : Transform3::Identity();
  } else if ((cylinderVolumeBounds != nullptr) && idTrf && !concentric) {
    vtransform = Transform3(Translation3(0., 0., zEstFromLayerEnv));
  } else if (!idTrf && (cylinderVolumeBounds == nullptr)) {
    // create the CylinderBounds from parsed layer inputs
    cylinderVolumeBounds = std::make_shared<CylinderVolumeBounds>(
        layerRmin, layerRmax, halflengthFromLayer);
  }

  ACTS_VERBOSE("    -> dimensions from layers   (rMin/rMax/zMin/zMax) = "
               << layerRmin << " / " << layerRmax << " / " << layerZmin << " / "
               << layerZmax);

  double zFromTransform = !idTrf ? transform.translation().z() : 0.;
  ACTS_VERBOSE(
      "    -> while created bounds are (rMin/rMax/zMin/zMax) = "
      << cylinderVolumeBounds->get(CylinderVolumeBounds::eMinR) << " / "
      << cylinderVolumeBounds->get(CylinderVolumeBounds::eMaxR) << " / "
      << zFromTransform -
             cylinderVolumeBounds->get(CylinderVolumeBounds::eHalfLengthZ)
      << " / "
      << zFromTransform +
             cylinderVolumeBounds->get(CylinderVolumeBounds::eHalfLengthZ));

  // both is NOW given --- check it -----------------------------
  if (cylinderVolumeBounds != nullptr) {
    // only check
    if (zFromTransform -
                cylinderVolumeBounds->get(CylinderVolumeBounds::eHalfLengthZ) <=
            layerZmin &&
        zFromTransform +
                cylinderVolumeBounds->get(CylinderVolumeBounds::eHalfLengthZ) >=
            layerZmax &&
        cylinderVolumeBounds->get(CylinderVolumeBounds::eMinR) <= layerRmin &&
        cylinderVolumeBounds->get(CylinderVolumeBounds::eMaxR) >= layerRmax) {
      return true;
    } else {
      ACTS_WARNING(
          "Provided layers are not contained by volume ! Bailing out. ");
      ACTS_WARNING("- zFromTransform: " << zFromTransform);
      ACTS_WARNING("- volumeZmin:"
                   << zFromTransform - cylinderVolumeBounds->get(
                                           CylinderVolumeBounds::eHalfLengthZ)
                   << ", layerZmin: " << layerZmin);
      ACTS_WARNING("- volumeZmax: "
                   << zFromTransform + cylinderVolumeBounds->get(
                                           CylinderVolumeBounds::eHalfLengthZ)
                   << ", layerZmax: " << layerZmax);
      ACTS_WARNING("- volumeRmin: "
                   << cylinderVolumeBounds->get(CylinderVolumeBounds::eMinR)
                   << ", layerRmin: " << layerRmin);
      ACTS_WARNING("- volumeRmax: "
                   << cylinderVolumeBounds->get(CylinderVolumeBounds::eMaxR)
                   << ", layerRmax: " << layerRmax);
      return false;
    }
  }

  ACTS_VERBOSE("Created/Checked " << *cylinderVolumeBounds);
  return true;
}

bool Acts::CylinderVolumeHelper::interGlueTrackingVolume(
    const GeometryContext& gctx, const std::shared_ptr<TrackingVolume>& tVolume,
    bool rBinned, double rMin, double rGlueMin, double rMax, double zMin,
    double zMax) const {
  ACTS_VERBOSE("Glue contained TrackingVolumes of container '"
               << tVolume->volumeName() << "'.");

  // only go on if you have confinedVolumes
  if (tVolume->confinedVolumes()) {
    // get the glueVolumes descriptor of the top volume to register the outside
    // volumes
    GlueVolumesDescriptor& glueDescr = tVolume->glueVolumesDescriptor();

    // now retrieve the volumes
    auto& volumes = tVolume->confinedVolumes()->arrayObjects();

    // list the volume names:
    //  and make the screen output readable
    std::size_t ivol = 0;
    for (auto& vol : volumes) {
      ACTS_VERBOSE("[" << ivol++ << "] - volume : " << vol->volumeName());
    }

    // the needed iterators
    auto tVolIter = volumes.begin();
    auto tVolFirst = volumes.begin();
    auto tVolLast = volumes.end();
    --tVolLast;
    auto tVolEnd = volumes.end();

    // the glue volumes for the description
    TrackingVolumeVector glueVolumesInnerTube;
    TrackingVolumeVector glueVolumesOuterTube;
    TrackingVolumeVector glueVolumesNegativeFace;
    TrackingVolumeVector glueVolumesPositiveFace;
    // reset ivol counter
    ivol = 0;
    // volumes of increasing r
    if (rBinned) {
      // loop over the volumes -------------------------------
      for (; tVolIter != tVolEnd;) {
        // screen output
        ACTS_VERBOSE("r-binning: Processing volume [" << ivol++ << "]");
        // for the first one
        std::shared_ptr<TrackingVolume> tVol =
            std::const_pointer_cast<TrackingVolume>(*tVolIter);
        if (tVolIter == tVolFirst) {
          addFaceVolumes(tVol, tubeInnerCover, glueVolumesInnerTube);
        }
        // add this or the subvolumes to the negativeFace and positiveFace
        addFaceVolumes(tVol, negativeFaceXY, glueVolumesNegativeFace);
        addFaceVolumes(tVol, positiveFaceXY, glueVolumesPositiveFace);
        if (tVolIter == tVolLast) {
          addFaceVolumes(tVol, tubeOuterCover, glueVolumesOuterTube);
          ++tVolIter;
        } else {
          std::shared_ptr<TrackingVolume> tVol1 =
              std::const_pointer_cast<TrackingVolume>(*tVolIter);
          std::shared_ptr<TrackingVolume> tVol2 =
              std::const_pointer_cast<TrackingVolume>(*(++tVolIter));

          // re-evalueate rGlueMin
          ActsScalar rGlueR =
              0.5 * (tVol1->volumeBounds()
                         .values()[CylinderVolumeBounds::BoundValues::eMaxR] +
                     tVol2->volumeBounds()
                         .values()[CylinderVolumeBounds::BoundValues::eMinR]);

          glueTrackingVolumes(gctx, tVol1, tubeOuterCover, tVol2,
                              tubeInnerCover, rMin, rGlueR, rMax, zMin, zMax);
        }
      }
    } else {
      // Volumes in increasing z
      // Loop over the volumes
      for (; tVolIter != tVolEnd;) {
        // screen output
        ACTS_VERBOSE("z-binning: Processing volume '"
                     << (*tVolIter)->volumeName() << "'.");
        std::shared_ptr<TrackingVolume> tVol =
            std::const_pointer_cast<TrackingVolume>(*tVolIter);
        if (tVolIter == tVolFirst) {
          addFaceVolumes(tVol, negativeFaceXY, glueVolumesNegativeFace);
        }
        addFaceVolumes(tVol, tubeInnerCover, glueVolumesInnerTube);
        addFaceVolumes(tVol, tubeOuterCover, glueVolumesOuterTube);
        if (tVolIter == tVolLast) {
          addFaceVolumes(tVol, positiveFaceXY, glueVolumesPositiveFace);
          ++tVolIter;
        } else {
          std::shared_ptr<TrackingVolume> tVol1 =
              std::const_pointer_cast<TrackingVolume>(*tVolIter);
          std::shared_ptr<TrackingVolume> tVol2 =
              std::const_pointer_cast<TrackingVolume>(*(++tVolIter));
          glueTrackingVolumes(gctx, tVol1, positiveFaceXY, tVol2,
                              negativeFaceXY, rMin, rGlueMin, rMax, zMin, zMax);
        }
      }
    }
    // create BinnedArraysand register then to the glue volume descriptor for
    // upstream glueing
    if (!glueVolumesNegativeFace.empty()) {
      // create the outside volume array
      std::shared_ptr<const TrackingVolumeArray> glueVolumesNegativeFaceArray =
          m_cfg.trackingVolumeArrayCreator->trackingVolumeArray(
              gctx, glueVolumesNegativeFace, BinningValue::binR);
      // register the glue voluems
      glueDescr.registerGlueVolumes(negativeFaceXY,
                                    glueVolumesNegativeFaceArray);
    }
    if (!glueVolumesPositiveFace.empty()) {
      // create the outside volume array
      std::shared_ptr<const TrackingVolumeArray> glueVolumesPositiveFaceArray =
          m_cfg.trackingVolumeArrayCreator->trackingVolumeArray(
              gctx, glueVolumesPositiveFace, BinningValue::binR);
      // register the glue voluems
      glueDescr.registerGlueVolumes(positiveFaceXY,
                                    glueVolumesPositiveFaceArray);
    }
    if (!glueVolumesInnerTube.empty()) {
      // create the outside volume array
      std::shared_ptr<const TrackingVolumeArray> glueVolumesInnerTubeArray =
          m_cfg.trackingVolumeArrayCreator->trackingVolumeArray(
              gctx, glueVolumesInnerTube, BinningValue::binZ);
      // register the glue voluems
      glueDescr.registerGlueVolumes(tubeInnerCover, glueVolumesInnerTubeArray);
    }
    if (!glueVolumesOuterTube.empty()) {
      // create the outside volume array
      std::shared_ptr<const TrackingVolumeArray> glueVolumesOuterTubeArray =
          m_cfg.trackingVolumeArrayCreator->trackingVolumeArray(
              gctx, glueVolumesOuterTube, BinningValue::binZ);
      // register the glue voluems
      glueDescr.registerGlueVolumes(tubeOuterCover, glueVolumesOuterTubeArray);
    }

    ACTS_VERBOSE("[GV] Register " << glueVolumesNegativeFace.size()
                                  << " volumes at face negativeFaceXY:");
    for (tVolIter = glueVolumesNegativeFace.begin();
         tVolIter != glueVolumesNegativeFace.end(); ++tVolIter) {
      ACTS_VERBOSE("   -> volume '" << (*tVolIter)->volumeName() << "'");
    }
    ACTS_VERBOSE("[GV] Register " << glueVolumesPositiveFace.size()
                                  << " volumes at face positiveFaceXY: ");
    for (tVolIter = glueVolumesPositiveFace.begin();
         tVolIter != glueVolumesPositiveFace.end(); ++tVolIter) {
      ACTS_VERBOSE("   -> volume '" << (*tVolIter)->volumeName() << "'");
    }
    ACTS_VERBOSE("[GV] Register " << glueVolumesInnerTube.size()
                                  << " volumes at face tubeInnerCover: ");
    for (tVolIter = glueVolumesInnerTube.begin();
         tVolIter != glueVolumesInnerTube.end(); ++tVolIter) {
      ACTS_VERBOSE("   -> volume '" << (*tVolIter)->volumeName() << "'");
    }
    ACTS_VERBOSE("[GV] Register " << glueVolumesOuterTube.size()
                                  << " volumes at face tubeOuterCover:");
    for (tVolIter = glueVolumesOuterTube.begin();
         tVolIter != glueVolumesOuterTube.end(); ++tVolIter) {
      ACTS_VERBOSE("   -> volume '" << (*tVolIter)->volumeName() << "'");
    }
  }
  // return success
  return true;
}

/** private helper method to fill the glue volumes (or the volume itself in) */
void Acts::CylinderVolumeHelper::glueTrackingVolumes(
    const GeometryContext& gctx, const std::shared_ptr<TrackingVolume>& tvolOne,
    BoundarySurfaceFace faceOne, const std::shared_ptr<TrackingVolume>& tvolTwo,
    BoundarySurfaceFace faceTwo, double rMin, double rGlueMin, double rMax,
    double zMin, double zMax) const {
  // get the two gluevolume descriptors
  const GlueVolumesDescriptor& gvDescriptorOne =
      tvolOne->glueVolumesDescriptor();
  const GlueVolumesDescriptor& gvDescriptorTwo =
      tvolTwo->glueVolumesDescriptor();

  std::size_t volOneGlueVols =
      gvDescriptorOne.glueVolumes(faceOne)
          ? gvDescriptorOne.glueVolumes(faceOne)->arrayObjects().size()
          : 0;
  ACTS_VERBOSE("GlueVolumeDescriptor of volume '"
               << tvolOne->volumeName() << "' has " << volOneGlueVols << " @ "
               << faceOne);
  std::size_t volTwoGlueVols =
      gvDescriptorTwo.glueVolumes(faceTwo)
          ? gvDescriptorTwo.glueVolumes(faceTwo)->arrayObjects().size()
          : 0;
  ACTS_VERBOSE("GlueVolumeDescriptor of volume '"
               << tvolTwo->volumeName() << "' has " << volTwoGlueVols << " @ "
               << faceTwo);

  // they could still be a container though - should not happen usually
  TrackingVolumePtr glueVolOne =
      volOneGlueVols != 0u
          ? gvDescriptorOne.glueVolumes(faceOne)->arrayObjects()[0]
          : tvolOne;
  TrackingVolumePtr glueVolTwo =
      volTwoGlueVols != 0u
          ? gvDescriptorTwo.glueVolumes(faceTwo)->arrayObjects()[0]
          : tvolTwo;

  // We'll need to mutate those volumes in order to glue them together
  auto mutableGlueVolOne = std::const_pointer_cast<TrackingVolume>(glueVolOne);
  auto mutableGlueVolTwo = std::const_pointer_cast<TrackingVolume>(glueVolTwo);

  // check the cases
  if (volOneGlueVols <= 1 && volTwoGlueVols <= 1) {
    // (i) one -> one
    ACTS_VERBOSE("      glue : one[ " << glueVolOne->volumeName() << " @ "
                                      << faceOne << " ]-to-one[ "
                                      << glueVolTwo->volumeName() << " @ "
                                      << faceTwo << " ]");
    // one to one is easy
    mutableGlueVolOne->glueTrackingVolume(gctx, faceOne,
                                          mutableGlueVolTwo.get(), faceTwo);

  } else if (volOneGlueVols <= 1) {
    // (ii) one -> many
    ACTS_VERBOSE("      glue : one[ "
                 << glueVolOne->volumeName() << " @ " << faceOne
                 << " ]-to-many[ " << tvolTwo->volumeName() << " @ " << faceTwo
                 << " ]");
    auto mutableFaceTwoVolumes = std::const_pointer_cast<TrackingVolumeArray>(
        gvDescriptorTwo.glueVolumes(faceTwo));
    mutableGlueVolOne->glueTrackingVolumes(gctx, faceOne, mutableFaceTwoVolumes,
                                           faceTwo);
  } else if (volTwoGlueVols <= 1) {
    // (iii) many -> one
    ACTS_VERBOSE("      glue : many[ "
                 << tvolOne->volumeName() << " @ " << faceOne << " ]-to-one[ "
                 << glueVolTwo->volumeName() << " @ " << faceTwo << " ]");
    auto mutableFaceOneVolumes = std::const_pointer_cast<TrackingVolumeArray>(
        gvDescriptorOne.glueVolumes(faceOne));
    mutableGlueVolTwo->glueTrackingVolumes(gctx, faceTwo, mutableFaceOneVolumes,
                                           faceOne);
  } else {
    // (iv) glue array to array
    ACTS_VERBOSE("      glue : many[ "
                 << tvolOne->volumeName() << " @ " << faceOne << " ]-to-many[ "
                 << tvolTwo->volumeName() << " @ " << faceTwo << " ]");

    // Create a new BoundarySurface as shared pointer
    std::shared_ptr<const BoundarySurfaceT<TrackingVolume>> boundarySurface =
        nullptr;

    // the transform of the new boundary surface
    Transform3 transform = Transform3::Identity();
    if (std::abs(zMin + zMax) > 0.1) {
      // it's not a concentric cylinder, so create a transform
      transform =
          Transform3(Translation3(Vector3(0., 0., 0.5 * (zMin + zMax))));
    }
    // 2 cases: r-Binning and zBinning
    if (faceOne == cylinderCover || faceOne == tubeOuterCover) {
      // (1) create the Boundary CylinderSurface
      auto cBounds =
          std::make_shared<CylinderBounds>(rGlueMin, 0.5 * (zMax - zMin));
      std::shared_ptr<const RegularSurface> cSurface =
          Surface::makeShared<CylinderSurface>(transform, cBounds);
      ACTS_VERBOSE(
          "             creating a new cylindrical boundary surface "
          "with bounds = "
          << cSurface->bounds());
      ACTS_VERBOSE("             at " << cSurface->center(gctx).transpose());
      boundarySurface =
          std::make_shared<const BoundarySurfaceT<TrackingVolume>>(
              std::move(cSurface), gvDescriptorOne.glueVolumes(faceOne),
              gvDescriptorTwo.glueVolumes(faceTwo));
    } else {
      // Calculate correct position for disc surface

      // we assume it's cylinder bounds
      auto cylVolBounds = dynamic_cast<const Acts::CylinderVolumeBounds*>(
          &tvolOne->volumeBounds());
      double zPos = tvolOne->center().z();
      double zHL = cylVolBounds->get(CylinderVolumeBounds::eHalfLengthZ);
      transform = Transform3(Translation3(0, 0, zPos + zHL));
      // this puts the surface on the positive z side of the cyl vol bounds
      // iteration is from neg to pos, so it should always be in between.

      // (2) create the BoundaryDiscSurface, in that case the zMin/zMax provided
      // are both the position of the disk in question
      std::shared_ptr<const RegularSurface> dSurface =
          Surface::makeShared<DiscSurface>(transform, rMin, rMax);
      ACTS_VERBOSE(
          "             creating a new disc-like boundary surface "
          "with bounds = "
          << dSurface->bounds());
      ACTS_VERBOSE("             at " << dSurface->center(gctx).transpose());
      boundarySurface =
          std::make_shared<const BoundarySurfaceT<TrackingVolume>>(
              std::move(dSurface), gvDescriptorOne.glueVolumes(faceOne),
              gvDescriptorTwo.glueVolumes(faceTwo));
    }

    // Collect the material - might be ambiguous, first one wins
    std::shared_ptr<const ISurfaceMaterial> boundaryMaterial = nullptr;

    ACTS_VERBOSE("New Boundary surface setting for containers");
    ACTS_VERBOSE(" - at first volume: " << tvolOne->volumeName());
    // Update the volume with the boundary surface accordingly
    // it's safe to access directly, they can not be nullptr
    for (auto& oneVolume :
         gvDescriptorOne.glueVolumes(faceOne)->arrayObjects()) {
      auto mutableOneVolume =
          std::const_pointer_cast<TrackingVolume>(oneVolume);
      // Look out for surface material
      if (boundaryMaterial == nullptr) {
        auto oneBSurface = mutableOneVolume->boundarySurfaces()[faceOne];
        boundaryMaterial =
            oneBSurface->surfaceRepresentation().surfaceMaterialSharedPtr();
      }
      mutableOneVolume->updateBoundarySurface(faceOne, boundarySurface);
      ACTS_VERBOSE(" -> setting boundary surface to volume: "
                   << mutableOneVolume->volumeName());
    }
    ACTS_VERBOSE(" - at second volume: " << tvolTwo->volumeName());
    for (auto& twoVolume :
         gvDescriptorTwo.glueVolumes(faceTwo)->arrayObjects()) {
      auto mutableTwoVolume =
          std::const_pointer_cast<TrackingVolume>(twoVolume);
      // Look out for surface material
      if (boundaryMaterial == nullptr) {
        auto twoBSurface = mutableTwoVolume->boundarySurfaces()[faceTwo];
        boundaryMaterial =
            twoBSurface->surfaceRepresentation().surfaceMaterialSharedPtr();
      }
      mutableTwoVolume->updateBoundarySurface(faceTwo, boundarySurface);
      ACTS_VERBOSE(" -> setting boundary surface to volume: "
                   << mutableTwoVolume->volumeName());
    }

    // If we have boundary material, let's assign it
    if (boundaryMaterial != nullptr) {
      // Adapt the boundary material
      ACTS_VERBOSE("- the new boundary surface has boundary material: ");
      ACTS_VERBOSE("    " << *boundaryMaterial);
      RegularSurface* newSurface = const_cast<RegularSurface*>(
          &(boundarySurface->surfaceRepresentation()));
      newSurface->assignSurfaceMaterial(boundaryMaterial);
    }

  }  // end of case (iv)
}

/** Private method - helper method not to duplicate code */
void Acts::CylinderVolumeHelper::addFaceVolumes(
    const std::shared_ptr<TrackingVolume>& tvol, BoundarySurfaceFace glueFace,
    TrackingVolumeVector& vols) const {
  ACTS_VERBOSE("Adding face volumes of face " << glueFace << " for the volume '"
                                              << tvol->volumeName() << "'.");
  // retrieve the gluevolume descriptor
  const GlueVolumesDescriptor& gvDescriptor = tvol->glueVolumesDescriptor();
  // if volumes are registered: take them
  if (gvDescriptor.glueVolumes(glueFace)) {
    // get the navigation level subvolumes
    auto volIter = gvDescriptor.glueVolumes(glueFace)->arrayObjects().begin();
    auto volEnd = gvDescriptor.glueVolumes(glueFace)->arrayObjects().end();
    for (; volIter != volEnd; ++volIter) {
      ACTS_VERBOSE("   -> adding : " << (*volIter)->volumeName());
      vols.push_back(*volIter);
    }
    // screen output
    ACTS_VERBOSE(vols.size()
                 << " navigation volumes registered as glue volumes.");
  } else {
    // the volume itself is on navigation level
    ACTS_VERBOSE("     -> adding only volume itself (at navigation level).");
    vols.push_back(tvol);
  }
}

std::shared_ptr<const Acts::Layer>
Acts::CylinderVolumeHelper::createCylinderLayer(double z, double r,
                                                double halflengthZ,
                                                double thickness, int binsPhi,
                                                int binsZ) const {
  ACTS_VERBOSE("Creating a CylinderLayer at position " << z << " and radius "
                                                       << r);
  // positioning
  const Transform3 transform(Translation3(0., 0., z));

  // z-binning
  BinUtility layerBinUtility(binsZ, z - halflengthZ, z + halflengthZ, open,
                             BinningValue::binZ);
  if (binsPhi == 1) {
    // the BinUtility for the material
    // ---------------------> create material for the layer surface
    ACTS_VERBOSE(" -> Preparing the binned material with " << binsZ
                                                           << " bins in Z. ");

  } else {  // break the phi symmetry
    // update the BinUtility: local position on Cylinder is rPhi, z
    BinUtility layerBinUtilityPhiZ(binsPhi, -r * M_PI, +r * M_PI, closed,
                                   BinningValue::binPhi);
    layerBinUtilityPhiZ += layerBinUtility;
    // ---------------------> create material for the layer surface
    ACTS_VERBOSE(" -> Preparing the binned material with "
                 << binsPhi << " / " << binsZ << " bins in phi / Z. ");
  }
  // @todo create the SurfaceMaterial
  // bounds for cylinderical surface
  CylinderBounds* cylinderBounds = new CylinderBounds(r, halflengthZ);
  // create the cylinder
  return CylinderLayer::create(
      transform, std::shared_ptr<const CylinderBounds>(cylinderBounds), nullptr,
      thickness);
}

std::shared_ptr<const Acts::Layer> Acts::CylinderVolumeHelper::createDiscLayer(
    double z, double rMin, double rMax, double thickness, int binsPhi,
    int binsR) const {
  ACTS_VERBOSE("Creating a DiscLayer at position " << z << " and rMin/rMax "
                                                   << rMin << " / " << rMax);

  // positioning
  const Transform3 transform(Translation3(0., 0., z));

  // R is the primary binning for the material
  BinUtility materialBinUtility(binsR, rMin, rMax, open, BinningValue::binR);
  if (binsPhi == 1) {
    ACTS_VERBOSE(" -> Preparing the binned material with " << binsR
                                                           << " bins in R. ");
  } else {
    // also binning in phi chosen
    materialBinUtility +=
        BinUtility(binsPhi, -static_cast<float>(M_PI), static_cast<float>(M_PI),
                   closed, BinningValue::binPhi);
    ACTS_VERBOSE(" -> Preparing the binned material with "
                 << binsPhi << " / " << binsR << " bins in phi / R. ");
  }

  // @todo create the SurfaceMaterial
  // bounds for disk-like surface
  RadialBounds* discBounds = new RadialBounds(rMin, rMax);
  // create the disc
  return DiscLayer::create(transform,
                           std::shared_ptr<const DiscBounds>(discBounds),
                           nullptr, thickness);
}
