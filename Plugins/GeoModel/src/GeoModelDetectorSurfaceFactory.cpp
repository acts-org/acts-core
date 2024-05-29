// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/GeoModelDetectorSurfaceFactory.hpp"

#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"
#include "Acts/Plugins/GeoModel/GeoModelTree.hpp"

#include <GeoModelKernel/GeoShapeUnion.h>
#include <GeoModelKernel/GeoShapeShift.h>

Acts::GeoModelDetectorSurfaceFactory::GeoModelDetectorSurfaceFactory(
    const Config& cfg, std::unique_ptr<const Logger> mlogger)
    : m_cfg(cfg), m_logger(std::move(mlogger)) {}

void Acts::GeoModelDetectorSurfaceFactory::construct(
    Cache& cache, const GeometryContext&, const GeoModelTree& geoModelTree,
    const Options& options) {
  if (geoModelTree.geoReader == nullptr) {
    throw std::invalid_argument("GeoModelTree has no GeoModelReader");
  }

  for (const auto& q : options.queries) {
    ACTS_VERBOSE("Constructing detector elements for query " << q);
    auto qFPV =
        geoModelTree.geoReader->getPublishedNodes<std::string, GeoFullPhysVol*>(
            q);

    for (auto& [name, fpv] : qFPV) {
      // Convert using the list of converters
      bool success = false;
      for (const auto& converter : m_cfg.shapeConverters) {
        auto converted = converter->toSensitiveSurface(*fpv);
        if (converted.ok()) {
          // Add the element and surface to the cache
          cache.sensitiveSurfaces.push_back(converted.value());
          success = true;
          ACTS_VERBOSE("successfully converted " << name << " (" << fpv->getLogVol()->getName() << " / " << fpv->getLogVol()->getShape()->type() << ")");
          break;
        }
      }

      if (!success) {
        ACTS_DEBUG(name << " (" << fpv->getLogVol()->getName() << " / " << fpv->getLogVol()->getShape()->type() << ") could not be converted by any converter");
        if(fpv->getLogVol()->getShape()->type() == "Union") {
          auto u = static_cast<const GeoShapeUnion *>(fpv->getLogVol()->getShape());
          ACTS_DEBUG(" -> Union A " << u->getOpA()->type());
          ACTS_DEBUG(" -> Union B " << u->getOpB()->type());
        }
        if(fpv->getLogVol()->getShape()->type() == "Shift") {
          auto s = static_cast<const GeoShapeShift *>(fpv->getLogVol()->getShape());
          ACTS_DEBUG(" -> Shift Op " << s->getOp()->type());
        }
      }
    }
    ACTS_VERBOSE("Found " << qFPV.size()
                          << " full physical volumes matching the query.");
  }
  ACTS_DEBUG("Constructed "
             << cache.sensitiveSurfaces.size() << " sensitive elements and "
             << cache.passiveSurfaces.size() << " passive elements");
}
