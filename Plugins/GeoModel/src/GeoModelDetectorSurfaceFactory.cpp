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

#include <set>

#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/GeoShapeUnion.h>

namespace {
std::string recType(const GeoShape &gshape);

std::string recType(const GeoShapeShift &gshift) {
  return "Shift[" + recType(*gshift.getOp()) + "]";
}
std::string recType(const GeoShapeUnion &gunion) {
  return "Union[" + recType(*gunion.getOpA()) + ", " +
         recType(*gunion.getOpB()) + "]";
}
std::string recType(const GeoShape &gshape) {
  if (auto ptr = dynamic_cast<const GeoShapeUnion *>(&gshape); ptr != nullptr) {
    return recType(*ptr);
  }
  if (auto ptr = dynamic_cast<const GeoShapeShift *>(&gshape); ptr != nullptr) {
    return recType(*ptr);
  }
  return gshape.type();
}

}  // namespace

Acts::GeoModelDetectorSurfaceFactory::GeoModelDetectorSurfaceFactory(
    const Config &cfg, std::unique_ptr<const Logger> mlogger)
    : m_cfg(cfg), m_logger(std::move(mlogger)) {}

void Acts::GeoModelDetectorSurfaceFactory::construct(
    Cache &cache, const GeometryContext & /*gctx*/,
    const GeoModelTree &geoModelTree, const Options &options) {
  if (geoModelTree.geoReader == nullptr) {
    throw std::invalid_argument("GeoModelTree has no GeoModelReader");
  }

  for (const auto &q : options.queries) {
    ACTS_VERBOSE("Constructing detector elements for query " << q);
    auto qFPV = geoModelTree.geoReader
                    ->getPublishedNodes<std::string, GeoFullPhysVol *>(q);

    auto matches = [&](const std::string &name, const GeoVFullPhysVol &fpv) {
      bool match = m_cfg.nameList.empty() && m_cfg.materialList.empty();
      if (match) {
        return true;
      }

      match |= std::any_of(
          m_cfg.nameList.begin(), m_cfg.nameList.end(),
          [&](const auto &n) { return name.find(n) != std::string::npos; });
      if (match) {
        return true;
      }

      const auto &matStr = fpv.getLogVol()->getMaterial()->getName();
      match |= std::any_of(
          m_cfg.materialList.begin(), m_cfg.materialList.end(),
          [&](const auto &m) { return matStr.find(m) != std::string::npos; });

      return match;
    };

    // Store stems of volume names and materials for INFO output
    std::set<std::string> volNameStems;
    std::set<std::string> materials;

    for (auto &[name, fpv] : qFPV) {
      if (fpv == nullptr) {
        ACTS_WARNING("Pointer to volume '" << name << "' is null");
        continue;
      }

      const std::string &vname = fpv->getLogVol()->getName();
      const GeoShape &shape = *fpv->getLogVol()->getShape();

      if (!matches(name, *fpv)) {
        continue;
      }

      // Convert using the list of converters
      bool success = false;

      for (const auto &converter : m_cfg.shapeConverters) {
        auto converted = converter->toSensitiveSurface(*fpv);
        if (converted.ok()) {
          // Add the element and surface to the cache
          cache.sensitiveSurfaces.push_back(converted.value());
          const auto &[el, sf] = converted.value();
          success = true;
          ACTS_VERBOSE("successfully converted "
                       << name << " (" << vname << " / " << recType(shape)
                       << " / " << fpv->getLogVol()->getMaterial()->getName()
                       << ")");
          if (!(el && sf)) {
            throw std::runtime_error("something is nullptr");
          }
          break;
        }
      }

      if (!success) {
        ACTS_DEBUG(name << " (" << vname << " / " << recType(shape)
                        << ") could not be converted by any converter");
      } else {
        volNameStems.emplace(name.substr(0, 6));
        materials.emplace(fpv->getLogVol()->getMaterial()->getName());
      }
    }
    ACTS_VERBOSE("Found " << qFPV.size()
                          << " full physical volumes matching the query.");

    auto streamVec = [](const auto &v) {
      std::stringstream ss;
      for (const auto &el : v) {
        ss << el << " ";
      }
      return ss.str();
    };

    ACTS_INFO("Converted volumes (stems): " << streamVec(volNameStems));
    ACTS_INFO("Materials of converted volumes: " << streamVec(materials));
  }
  ACTS_DEBUG("Constructed "
             << cache.sensitiveSurfaces.size() << " sensitive elements and "
             << cache.passiveSurfaces.size() << " passive elements");
}
