// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/DetectorBuilder.hpp"

#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/interface/IGeometryIdGenerator.hpp"
#include "Acts/Material/IMaterialDecorator.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"

#include <stdexcept>

Acts::Experimental::DetectorBuilder::DetectorBuilder(
    const Acts::Experimental::DetectorBuilder::Config& cfg,
    std::unique_ptr<const Acts::Logger> mlogger)
    : IDetectorBuilder(), m_cfg(cfg), m_logger(std::move(mlogger)) {
  if (m_cfg.builder == nullptr) {
    throw std::invalid_argument(
        "DetectorBuilder: no top level builder defined.");
  }
}

std::shared_ptr<const Acts::Experimental::Detector>
Acts::Experimental::DetectorBuilder::construct(
    const GeometryContext& gctx) const {
  // Screen printout of the auxiliary information
  if (!m_cfg.auxiliary.empty()) {
    ACTS_DEBUG(m_cfg.auxiliary);
  }
  ACTS_DEBUG("Building a detector with name " << m_cfg.name);

  auto [volumes, portals, roots] = m_cfg.builder->construct(gctx);

  // Assign the geometry ids to the detector - if configured
  if (m_cfg.geoIdGenerator != nullptr) {
    ACTS_DEBUG("Assigning geometry ids to the detector");
    auto cache = m_cfg.geoIdGenerator->generateCache();
    std::for_each(roots.volumes.begin(), roots.volumes.end(), [&](auto& v) {
      ACTS_VERBOSE("-> Assigning geometry id to volume " << v->name());
      m_cfg.geoIdGenerator->assignGeometryId(cache, *v);
    });
  }

  // Decorate the volumes with material - Surface material only at this moment
  if (m_cfg.materialDecorator != nullptr) {
    ACTS_DEBUG("Decorating the detector with material");
    std::for_each(volumes.begin(), volumes.end(), [&](auto& v) {
      // Assign to surfaces
      for (auto& sf : v->surfacePtrs()) {
        m_cfg.materialDecorator->decorate(*sf);
      }
      // Assign to portals
      for (auto& p : v->portalPtrs()) {
        m_cfg.materialDecorator->decorate(p->surface());
      }
    });
  }

  return Detector::makeShared(m_cfg.name, std::move(roots.volumes),
                              std::move(roots.volumeFinder));
}
