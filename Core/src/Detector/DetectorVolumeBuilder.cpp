// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/DetectorVolumeBuilder.hpp"

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/detail/ProtoMaterialHelper.hpp"
#include "Acts/Detector/interface/IExternalStructureBuilder.hpp"
#include "Acts/Detector/interface/IGeometryIdGenerator.hpp"
#include "Acts/Detector/interface/IInternalStructureBuilder.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <iterator>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

Acts::Experimental::DetectorVolumeBuilder::DetectorVolumeBuilder(
    const Acts::Experimental::DetectorVolumeBuilder::Config& cfg,
    std::unique_ptr<const Acts::Logger> mlogger)
    : IDetectorComponentBuilder(), m_cfg(cfg), m_logger(std::move(mlogger)) {
  if (m_cfg.externalsBuilder == nullptr) {
    throw std::invalid_argument(
        "DetectorVolumeBuilder: no external structure builder defined.");
  }
}

Acts::Experimental::DetectorComponent
Acts::Experimental::DetectorVolumeBuilder::construct(
    const GeometryContext& gctx) const {
  // The outgoing root volumes
  std::vector<std::shared_ptr<DetectorVolume>> rootVolumes;
  // Screen printout of the auxiliary information
  if (!m_cfg.auxiliary.empty()) {
    ACTS_DEBUG(m_cfg.auxiliary);
  }
  ACTS_DEBUG("Building a volume with name '" << m_cfg.name << "'.");

  // Get transform and bounds from the volume
  auto [transform, bounds, portalGenerator] =
      m_cfg.externalsBuilder->construct(gctx);

  // Although only a single volume, describe it as a
  // container shell for further processing
  DetectorComponent::PortalContainer portalContainer;
  // The detector volume to be constructed
  std::shared_ptr<DetectorVolume> dVolume = nullptr;
  // If there are no internals, the volume is fully defined
  if (m_cfg.internalsBuilder == nullptr) {
    ACTS_VERBOSE("No internal structure present.");
    // Construct the DetectorVolume
    dVolume = DetectorVolumeFactory::construct(
        portalGenerator, gctx, m_cfg.name, transform, std::move(bounds),
        tryAllPortals());
  } else {
    // Internal structure is present
    ACTS_VERBOSE("Internal structure is being built.");
    auto [surfaces, volumes, surfacesUpdater, volumeUpdater] =
        m_cfg.internalsBuilder->construct(gctx);

    // Add the internally created volumes as root volumes
    if (m_cfg.addInternalsToRoot) {
      for (const auto& v : volumes) {
        rootVolumes.push_back(v);
      }
    }
    // Construct the DetectorVolume
    dVolume = DetectorVolumeFactory::construct(
        portalGenerator, gctx, m_cfg.name, transform, std::move(bounds),
        surfaces, volumes, std::move(volumeUpdater),
        std::move(surfacesUpdater));
  }
  // All portals are defined and build the current shell
  for (const auto& [ip, p] : enumerate(dVolume->portalPtrs())) {
    portalContainer[ip] = p;
  }

  // Assign the geometry ids if configured to do so
  if (m_cfg.geoIdGenerator != nullptr) {
    ACTS_DEBUG("Assigning geometry ids to the detector volume");
    auto cache = m_cfg.geoIdGenerator->generateCache();
    m_cfg.geoIdGenerator->assignGeometryId(cache, *dVolume);
  }

  // Assign the proto material if configured to do so
  for (const auto& [ip, bDescription] : m_cfg.portalMaterialBinning) {
    if (portalContainer.contains(ip)) {
      auto bd = detail::ProtoMaterialHelper::attachProtoMaterial(
          gctx, portalContainer[ip]->surface(), bDescription);
      ACTS_VERBOSE("-> Assigning proto material to portal " << ip << " with "
                                                            << bd.toString());
    }
  }

  // Add to the root volume collection if configured
  rootVolumes.push_back(dVolume);
  // The newly built volume is the single produced volume
  return Acts::Experimental::DetectorComponent{
      {dVolume},
      portalContainer,
      RootDetectorVolumes{rootVolumes, tryRootVolumes()}};
}
