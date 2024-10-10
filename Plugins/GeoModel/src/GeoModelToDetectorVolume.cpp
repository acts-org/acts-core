// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/GeoModelToDetectorVolume.hpp"

#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoPcon.h>
#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoShapeUnion.h>
#include <GeoModelKernel/GeoSimplePolygonBrep.h>
#include <GeoModelKernel/GeoTrd.h>
#include <GeoModelKernel/GeoTube.h>
#include <GeoModelKernel/GeoTubs.h>

namespace Acts::GeoModel {
Volume convertVolume(const Transform3& trf, const GeoShape& shape) {
  std::shared_ptr<VolumeBounds> bounds;
  GeoTrf::Transform3D newTrf = trf;
  if (shape.typeID() == GeoTube::getClassTypeID()) {
    const GeoTube* tube = dynamic_cast<const GeoTube*>(&shape);
    bounds = std::make_shared<CylinderVolumeBounds>(
        tube->getRMin(), tube->getRMax(), tube->getZHalfLength());
  } else if (shape.typeID() == GeoTubs::getClassTypeID()) {
    const GeoTubs* tubs = dynamic_cast<const GeoTubs*>(&shape);
    bounds = std::make_shared<CylinderVolumeBounds>(
        tubs->getRMin(), tubs->getRMax(), tubs->getZHalfLength(),
        tubs->getDPhi() / 2);
    newTrf = trf * GeoTrf::RotateZ3D(tubs->getSPhi() + 0.5 * tubs->getDPhi());
  } else if (shape.typeID() == GeoBox::getClassTypeID()) {
    const GeoBox* box = dynamic_cast<const GeoBox*>(&shape);
    bounds = std::make_shared<CuboidVolumeBounds>(
        box->getXHalfLength(), box->getYHalfLength(), box->getZHalfLength());
  } else if (shape.typeID() == GeoTrd::getClassTypeID()) {
    const GeoTrd* trd = dynamic_cast<const GeoTrd*>(&shape);
    double x1 = trd->getXHalfLength1();
    double x2 = trd->getXHalfLength2();
    double y1 = trd->getYHalfLength1();
    double y2 = trd->getYHalfLength2();
    double z = trd->getZHalfLength();

    if (y1 == y2) {
      if (x1 <= x2) {
        // y axis in ACTS is z axis in geomodel
        bounds = std::make_shared<TrapezoidVolumeBounds>(x1, x2, z, y1);
        constexpr double rotationAngle = M_PI / 2;
        newTrf = trf * GeoTrf::RotateX3D(rotationAngle);
      } else {
        bounds = std::make_shared<TrapezoidVolumeBounds>(x2, x1, z, y1);
        constexpr double rotationAngle = M_PI;
        newTrf = trf * GeoTrf::RotateY3D(rotationAngle) *
                 GeoTrf::RotateZ3D(rotationAngle);
      }
    } else if (x1 == x2) {
      if (y1 < y2) {
        bounds = std::make_shared<TrapezoidVolumeBounds>(y1, y2, z, x1);
        auto rotationAngle = M_PI / 2;
        newTrf = trf * GeoTrf::RotateZ3D(rotationAngle) *
                 GeoTrf::RotateX3D(rotationAngle);
      } else {
        bounds = std::make_shared<TrapezoidVolumeBounds>(y2, y1, z, x1);
        auto rotationAngle = M_PI;
        newTrf = trf * GeoTrf::RotateX3D(rotationAngle) *
                 GeoTrf::RotateZ3D(rotationAngle / 2) *
                 GeoTrf::RotateX3D(rotationAngle / 2);
      }
    } else {
      throw std::runtime_error("FATAL: Translating GeoTrd to ACTS failed");
    }
  }

  else if (shape.typeID() == GeoShapeUnion::getClassTypeID()) {
    const GeoShapeUnion* unionShape =
        dynamic_cast<const GeoShapeUnion*>(&shape);
    double xmin{0}, xmax{0}, ymin{0}, ymax{0}, zmin{0}, zmax{0};
    unionShape->extent(xmin, ymin, zmin, xmax, ymax, zmax);
    bounds = std::make_shared<CuboidVolumeBounds>(
        (xmax - xmin) / 2, (ymax - ymin) / 2, (zmax - zmin) / 2);
  } else if (shape.typeID() == GeoShapeSubtraction::getClassTypeID()) {
    // Go down the left side (opA) of the subtraction until we reach a normal
    // shape
    const GeoShapeSubtraction* subtractionShape =
        dynamic_cast<const GeoShapeSubtraction*>(&shape);
    const GeoShape* shapeA = subtractionShape->getOpA();
    return convertVolume(trf, *shapeA);
  } else if (shape.typeID() == GeoShapeSubtraction::getClassTypeID()) {
    // Go down the left side (opA) of the subtraction until we reach a normal
    // shape
    const GeoShapeSubtraction* subtractionShape =
        dynamic_cast<const GeoShapeSubtraction*>(&shape);
    const GeoShape* shapeA = subtractionShape->getOpA();
    return convertVolume(trf, *shapeA);
  } else if (shape.typeID() == GeoPcon::getClassTypeID()) {
    // Will change in future, get bounding box for now
    double xmin{0}, xmax{0}, ymin{0}, ymax{0}, zmin{0}, zmax{0};
    const GeoPcon* pcon = dynamic_cast<const GeoPcon*>(&shape);
    pcon->extent(xmin, ymin, zmin, xmax, ymax, zmax);
    bounds = std::make_shared<CuboidVolumeBounds>(
        (xmax - xmin) / 2, (ymax - ymin) / 2, (zmax - zmin) / 2);
  } else if (shape.typeID() == GeoShapeShift::getClassTypeID()) {
    const GeoShapeShift* shiftShape =
        dynamic_cast<const GeoShapeShift*>(&shape);
    const GeoShape* shapeOp = shiftShape->getOp();
    newTrf = trf * shiftShape->getX();
    return convertVolume(trf, *shapeOp);
  } else {
    throw std::runtime_error("FATAL: Unsupported GeoModel shape");
  }
  return Volume(newTrf, bounds);
}

std::shared_ptr<Experimental::DetectorVolume> convertDetectorVolume(
    const GeometryContext& context, const GeoShape& shape,
    const std::string& name, const GeoTrf::Transform3D& transform,
    const std::vector<GeoModelSensitiveSurface>& sensitives) {
  // type conversion from GeoModelSensitiveSurface to Surface
  std::vector<std::shared_ptr<Surface>> sensSurfaces(sensitives.size());
  std::transform(sensitives.begin(), sensitives.end(), sensSurfaces.begin(),
                 [](const std::tuple<std::shared_ptr<GeoModelDetectorElement>,
                                     std::shared_ptr<Surface>>& t) {
                   return std::get<1>(t);
                 });
  auto portalGenerator = Experimental::defaultPortalAndSubPortalGenerator();
  Volume vol = convertVolume(transform, shape);
  return Experimental::DetectorVolumeFactory::construct(
      portalGenerator, context, name, vol.transform(), vol.volumeBoundsPtr(),
      sensSurfaces,
      std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>{},
      Experimental::tryNoVolumes(), Experimental::tryAllPortalsAndSurfaces());
}

}  // namespace Acts::GeoModel
