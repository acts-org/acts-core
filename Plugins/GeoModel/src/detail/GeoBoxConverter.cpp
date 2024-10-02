// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/detail/GeoBoxConverter.hpp"

#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConversionError.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoShape.h>
#include <GeoModelKernel/Units.h>

Acts::Result<Acts::GeoModelSensitiveSurface>
Acts::detail::GeoBoxConverter::operator()(const PVConstLink& geoPV,
                                          const GeoBox& geoBox,
                                          const Transform3& absTransform,
                                          bool sensitive) const {
  /// auto-calculate the unit length conversion
  static constexpr ActsScalar unitLength =
      Acts::UnitConstants::mm / GeoModelKernelUnits::millimeter;

  // Create the surface transform
  Transform3 transform = Transform3::Identity();
  transform.translation() = unitLength * absTransform.translation();
  auto rotation = absTransform.rotation();
  // Get the half lengths
  std::vector<ActsScalar> halfLengths = {geoBox.getXHalfLength(),
                                         geoBox.getYHalfLength(),
                                         geoBox.getZHalfLength()};
  // Create the surface
  auto minElement = std::min_element(halfLengths.begin(), halfLengths.end());
  auto zIndex = std::distance(halfLengths.begin(), minElement);
  std::size_t yIndex = zIndex > 0u ? zIndex - 1u : 2u;
  std::size_t xIndex = yIndex > 0u ? yIndex - 1u : 2u;

  Vector3 colX = rotation.col(xIndex);
  Vector3 colY = rotation.col(yIndex);
  Vector3 colZ = rotation.col(zIndex);
  rotation.col(0) = colX;
  rotation.col(1) = colY;
  rotation.col(2) = colZ;
  transform.linear() = rotation;

  // Create the surface bounds
  ActsScalar halfX = unitLength * halfLengths[xIndex];
  ActsScalar halfY = unitLength * halfLengths[yIndex];
  auto rectangleBounds = std::make_shared<Acts::RectangleBounds>(halfX, halfY);
  if (!sensitive) {
    auto surface =
        Surface::makeShared<PlaneSurface>(transform, rectangleBounds);
    return std::make_tuple(nullptr, surface);
  }
  // Create the element and the surface
  auto detectorElement =
      GeoModelDetectorElement::createDetectorElement<PlaneSurface>(
          geoPV, rectangleBounds, transform,
          2 * unitLength * halfLengths[zIndex]);
  auto surface = detectorElement->surface().getSharedPtr();
  // Return the detector element and surface
  return std::make_tuple(detectorElement, surface);
}
