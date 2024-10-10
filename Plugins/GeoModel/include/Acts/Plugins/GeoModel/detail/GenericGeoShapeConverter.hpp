// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/GeoModel/GeoModelConversionError.hpp"
#include "Acts/Plugins/GeoModel/IGeoShapeConverter.hpp"

#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoTube.h>

namespace Acts::detail {

template <typename Shape, typename Converter>
struct GenericGeoShapeConverter : public IGeoShapeConverter {
  Acts::Result<Acts::GeoModelSensitiveSurface> toSensitiveSurface(
      PVConstLink geoPV, const Transform3& transform) const override {
    // Retrieve logical volume and absolute transform
    const GeoLogVol* logVol = geoPV->getLogVol();
    if (logVol != nullptr) {
      const GeoShape* geoShape = logVol->getShape();
      auto concreteShape = dynamic_cast<const Shape*>(geoShape);
      if (concreteShape != nullptr) {
        return Converter{}(geoPV, *concreteShape, transform, true);
      }
      return Result<GeoModelSensitiveSurface>::failure(
          GeoModelConversionError::WrongShapeForConverter);
    }
    return Result<GeoModelSensitiveSurface>::failure(
        GeoModelConversionError::MissingLogicalVolume);
  }

  Acts::Result<std::shared_ptr<Acts::Surface>> toPassiveSurface(
      PVConstLink geoPV, const Transform3& transform) const override {
    // Retrieve logical volume and absolute transform
    const GeoLogVol* logVol = geoPV->getLogVol();
    if (logVol != nullptr) {
      const GeoShape* geoShape = logVol->getShape();

      auto concreteShape = dynamic_cast<const Shape*>(geoShape);
      if (concreteShape != nullptr) {
        // Conversion function call with sensitive = false
        auto res = Converter{}(geoPV, *concreteShape, transform, false);
        if (!res.ok()) {
          return res.error();
        }

        const auto& [el, surface] = res.value();

        return Result<std::shared_ptr<Surface>>::success(surface);
      }
      return Result<std::shared_ptr<Surface>>::failure(
          GeoModelConversionError::WrongShapeForConverter);
    }
    return Result<std::shared_ptr<Surface>>::failure(
        GeoModelConversionError::MissingLogicalVolume);
  }
};

}  // namespace Acts::detail
