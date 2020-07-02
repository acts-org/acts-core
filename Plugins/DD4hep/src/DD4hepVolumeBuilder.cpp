// This file is part of the Acts project.
//
// Copyright (C) 2018 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/DD4hep/DD4hepVolumeBuilder.hpp"

#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Material/HomogeneousVolumeMaterial.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"
#include "Acts/Plugins/TGeo/TGeoPrimitivesHelper.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Utilities/Units.hpp"

#include "DD4hep/Detector.h"

Acts::DD4hepVolumeBuilder::DD4hepVolumeBuilder(
    const Acts::DD4hepVolumeBuilder::Config& config,
    std::unique_ptr<const Logger> logger)
    : m_cfg(), m_logger(std::move(logger)) {
  setConfiguration(config);
}

Acts::DD4hepVolumeBuilder::~DD4hepVolumeBuilder() = default;

void Acts::DD4hepVolumeBuilder::setConfiguration(
    const Acts::DD4hepVolumeBuilder::Config& config) {
  m_cfg = config;
}

std::vector<std::shared_ptr<Acts::TrackingVolume>>
Acts::DD4hepVolumeBuilder::centralVolumes() const {
  if (m_cfg.centralVolumes.empty()) {
    ACTS_VERBOSE("[L] No layers handed over for central volume!");
    return {};
  }

  ACTS_VERBOSE(
      "[L] Received layers for central volume -> creating "
      "cylindrical layers");

  // Resulting volumes
  MutableTrackingVolumeVector volumes;
  // Inner/outer radius and half length of the barrel
  double rMin, rMax, dz;

  // Go through volumes
  for (auto& detElement : m_cfg.centralVolumes) {
    // Access the global transformation matrix of the volume
    auto transform =
        convertTransform(&(detElement.nominal().worldTransformation()));
    // Get the shape of the volume
    TGeoShape* geoShape = detElement.placement().ptr()->GetVolume()->GetShape();

    if (geoShape != nullptr) {
      TGeoTubeSeg* tube = dynamic_cast<TGeoTubeSeg*>(geoShape);
      if (tube == nullptr)
        ACTS_ERROR(
            "[L] Cylinder layer has wrong shape - needs to be TGeoTubeSeg!");

      // Extract the boundaries
      rMin = tube->GetRmin() * units::_cm;
      rMax = tube->GetRmax() * units::_cm;
      dz = tube->GetDz() * units::_cm;

    } else {
      throw std::logic_error(
          std::string("Volume DetElement: ") + detElement.name() +
          std::string(" has not a shape "
                      "added to its extension. Please check your detector "
                      "constructor!"));
    }
    // Build boundaries
    CylinderVolumeBounds cvBounds(rMin, rMax, dz);
    volumes.push_back(TrackingVolume::create(
        transform, std::make_shared<const CylinderVolumeBounds>(cvBounds)));
  }
  return volumes;
}

std::shared_ptr<const Acts::Transform3D>
Acts::DD4hepVolumeBuilder::convertTransform(const TGeoMatrix* tGeoTrans) const {
  // Get the placement and orientation in respect to its mother
  const Double_t* rotation = tGeoTrans->GetRotationMatrix();
  const Double_t* translation = tGeoTrans->GetTranslation();
  auto transform =
      std::make_shared<const Transform3D>(TGeoPrimitivesHelper::makeTransform(
          Acts::Vector3D(rotation[0], rotation[3], rotation[6]),
          Acts::Vector3D(rotation[1], rotation[4], rotation[7]),
          Acts::Vector3D(rotation[2], rotation[5], rotation[8]),
          Acts::Vector3D(translation[0] * units::_cm,
                         translation[1] * units::_cm,
                         translation[2] * units::_cm)));
  return (transform);
}
