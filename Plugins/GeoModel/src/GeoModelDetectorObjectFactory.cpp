// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/GeoModelDetectorObjectFactory.hpp"

#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConverters.hpp"
#include "Acts/Plugins/GeoModel/IGeoShapeConverter.hpp"

#include <algorithm>
#include <iostream>
#include <typeinfo>

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoPcon.h>
#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoShapeUnion.h>
#include <GeoModelKernel/GeoSimplePolygonBrep.h>
#include <GeoModelKernel/GeoTrd.h>
#include <GeoModelKernel/GeoTube.h>
#include <GeoModelKernel/GeoTubs.h>

namespace {
std::string recType(const GeoShapeShift &gshift);
std::string recType(const GeoShapeUnion &gunion);
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

namespace Acts {
Acts::GeoModelDetectorObjectFactory::GeoModelDetectorObjectFactory(
    const Config &cfg, std::unique_ptr<const Logger> mlogger)
    : m_logger(std::move(mlogger)), m_cfg(cfg) {}

void Acts::GeoModelDetectorObjectFactory::construct(
    Cache &cache, const GeometryContext &gctx, const GeoModelTree &geoModelTree,
    const Options &options) {
  if (geoModelTree.geoReader == nullptr) {
    throw std::invalid_argument("GeoModelTree has no GeoModelReader");
  }
  for (const auto &q : options.queries) {
    ACTS_VERBOSE("Constructing detector elements for query " << q);
    // load data from database according to querie (Muon)
    auto qFPV = geoModelTree.geoReader
                    ->getPublishedNodes<std::string, GeoFullPhysVol *>(q);

    // go through each fpv
    for (const auto &[name, fpv] : qFPV) {
      PVConstLink physVol{fpv};
      // if the match lambda returns false skip the rest of the loop
      if (!matches(name, physVol)) {
        continue;
      }
      convertFpv(name, fpv, cache, gctx);
    }
  }
}

void Acts::GeoModelDetectorObjectFactory::convertSensitive(
    const PVConstLink &geoPV, const Acts::Transform3 &transform,
    std::vector<GeoModelSensitiveSurface> &sensitives) {
  const GeoLogVol *logVol = geoPV->getLogVol();
  const GeoShape *shape = logVol->getShape();
  int shapeId = shape->typeID();
  std::string name = logVol->getName();
  std::shared_ptr<const Acts::IGeoShapeConverter> converter =
      Acts::geoShapesConverters(shapeId);
  if (converter == nullptr) {
    throw std::runtime_error("The converter for " + recType(*shape) +
                             " is nullptr");
  }

  auto converted = converter->toSensitiveSurface(geoPV, transform);
  if (converted.ok()) {
    sensitives.push_back(converted.value());
    const auto &[el, sf] = converted.value();
    auto ss = std::get<1>(converted.value());

    ACTS_VERBOSE("(successfully converted: "
                 << name << " / " << recType(*shape) << " / "
                 << logVol->getMaterial()->getName() << ")");

    if (!el || !sf) {
      throw std::runtime_error(
          "The Detector Element or the Surface is nullptr");
    }
    return;
  }
  ACTS_ERROR(name << " / " << recType(*shape)
                  << ") could not be converted by any converter");
}

std::vector<GeoChildNodeWithTrf>
Acts::GeoModelDetectorObjectFactory::findAllSubVolumes(const PVConstLink &vol) {
  std::vector<GeoChildNodeWithTrf> subvolumes = getChildrenWithRef(vol, false);
  std::vector<GeoChildNodeWithTrf> sensitives;
  for (auto subvolume : subvolumes) {
    if (matches(subvolume.nodeName, subvolume.volume)) {
      sensitives.push_back(subvolume);
    }
    std::vector<GeoChildNodeWithTrf> senssubsubvolumes =
        findAllSubVolumes(subvolume.volume);
    std::transform(std::make_move_iterator(senssubsubvolumes.begin()),
                   std::make_move_iterator(senssubsubvolumes.end()),
                   std::back_inserter(sensitives),
                   [&subvolume](GeoChildNodeWithTrf &&volume) {
                     volume.transform = subvolume.transform * volume.transform;
                     return volume;
                   });
  }
  return sensitives;
}

bool Acts::GeoModelDetectorObjectFactory::convertBox(std::string name) {
  auto convB = std::ranges::any_of(m_cfg.convertBox, [&](const auto &n) {
    return name.find(n) != std::string::npos;
  });
  return convB;
}

void Acts::GeoModelDetectorObjectFactory::convertFpv(
    const std::string &name, GeoFullPhysVol *fpv, Cache &cache,
    const GeometryContext &gctx) {
  const auto prevSize = cache.sensitiveSurfaces.size();
  PVConstLink physVol{fpv};

  // get children
  std::vector<GeoChildNodeWithTrf> subvolumes =
      getChildrenWithRef(physVol, false);
  // vector containing all subvolumes to be converted to surfaces
  std::vector<GeoChildNodeWithTrf> surfaces = findAllSubVolumes(physVol);
  std::vector<GeoModelSensitiveSurface> sensitives;

  for (const auto &surface : surfaces) {
    const Transform3 &transform =
        fpv->getAbsoluteTransform() * surface.transform;
    convertSensitive(surface.volume, transform, sensitives);
  }
  cache.sensitiveSurfaces.insert(cache.sensitiveSurfaces.end(),
                                 sensitives.begin(), sensitives.end());
  if (convertBox(name)) {
    const GeoLogVol *logVol =
        physVol->getLogVol();  // get logVol for the shape of the volume
    const GeoShape *shape = logVol->getShape();  // get shape
    const Acts::Transform3 &fpvtransform = fpv->getAbsoluteTransform(nullptr);

    // convert bounding boxes with surfaces inside
    std::shared_ptr<Experimental::DetectorVolume> box =
        Acts::GeoModel::convertDetectorVolume(gctx, *shape, name, fpvtransform,
                                              sensitives);
    cache.boundingBoxes.push_back(box);
  }
  // If fpv has no subs and should not be converted to volume convert to surface
  else if (subvolumes.empty()) {
    // convert fpvs to surfaces
    const Transform3 &transform = fpv->getAbsoluteTransform();
    convertSensitive(fpv, transform, cache.sensitiveSurfaces);
  }

  // Set the corresponding database entry name to all sensitive surfaces
  for (auto i = prevSize; i < cache.sensitiveSurfaces.size(); ++i) {
    auto &[detEl, _] = cache.sensitiveSurfaces[i];
    detEl->setDatabaseEntryName(name);
  }
}
// function to determine if object fits query
bool Acts::GeoModelDetectorObjectFactory::matches(const std::string &name,
                                                  const PVConstLink &physvol) {
  if (m_cfg.nameList.empty() && m_cfg.materialList.empty()) {
    return true;
  }

  auto matchName = std::ranges::any_of(m_cfg.nameList, [&](const auto &n) {
    return name.find(n) != std::string::npos;
  });

  std::string matStr = physvol->getLogVol()->getMaterial()->getName();

  auto matchMaterial = std::ranges::any_of(
      m_cfg.materialList,
      [&](const auto &m) { return matStr.find(m) != std::string::npos; });

  bool match = matchMaterial && matchName;
  GeoIntrusivePtr<const GeoVFullPhysVol> fullVol =
      dynamic_pointer_cast<const GeoVFullPhysVol>(physvol);

  // for the fullphysvol we only check the name
  if (m_cfg.nameList.empty()) {
    return matchMaterial;
  }

  // if no material specified or we're looking at fpv judge by name only
  if (m_cfg.materialList.empty() || !(fullVol == nullptr)) {
    return matchName;
  }
  return match;
}
}  // namespace Acts
