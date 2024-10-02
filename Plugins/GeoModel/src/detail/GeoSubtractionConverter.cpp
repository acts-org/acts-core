// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/detail/GeoSubtractionConverter.hpp"

#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConversionError.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConverters.hpp"
#include "Acts/Plugins/GeoModel/IGeoShapeConverter.hpp"
#include "Acts/Surfaces/DiamondBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoShape.h>
#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/Units.h>

Acts::Result<Acts::GeoModelSensitiveSurface>
Acts::detail::GeoSubtractionConverter::operator()(
    [[maybe_unused]] const PVConstLink& geoPV,
    const GeoShapeSubtraction& geoSub, const Transform3& absTransform,
    [[maybe_unused]] bool sensitive) const {
  const GeoShape* shapeA = geoSub.getOpA();
  int shapeId = shapeA->typeID();
  std::shared_ptr<const Acts::IGeoShapeConverter> converter =
      Acts::geoShapesConverters(shapeId);
  if (converter == nullptr) {
    throw std::runtime_error("The converter for " + shapeA->type() +
                             " is nullptr");
  }
  // Material and name for the PVConstLink declaration are dummie variables
  GeoIntrusivePtr<GeoMaterial> material(new GeoMaterial("Material", 1.0));
  GeoIntrusivePtr<GeoLogVol> logA(new GeoLogVol("", shapeA, material));
  PVConstLink pvA = new GeoVPhysVol(logA);

  // recursively call the the converter
  auto converted = converter->toSensitiveSurface(pvA, absTransform);
  if (converted.ok()) {
    return converted.value();
  } else {
    throw std::runtime_error("Unexpected error in the conversion of " +
                             shapeA->type());
  }
}
