// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/GeoModel/detail/GeoTrdConverter.hpp"

#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConversionError.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoShape.h>
#include <GeoModelKernel/GeoTrd.h>
#include <GeoModelKernel/Units.h>

Acts::Result<Acts::GeoModelSensitiveSurface>
Acts::detail::GeoTrdConverter::operator()(const PVConstLink& geoPV,
                                          const GeoTrd& geoTrd,
                                          const Transform3& absTransform,
                                          bool sensitive) const {
  /// auto-calculate the unit length conversion
  static constexpr double unitLength =
      Acts::UnitConstants::mm / GeoModelKernelUnits::millimeter;

  // Create the surface transform
  Transform3 transform = Transform3::Identity();
  transform.translation() = unitLength * absTransform.translation();

  // GeoTrd coordinates: x is the extrusion direction, y is orthogonal to the
  // symmetry axis and z is along the symmetry axis
  double halfX1 = geoTrd.getXHalfLength1();
  double halfX2 = geoTrd.getXHalfLength2();
  double halfY1 = geoTrd.getYHalfLength1();
  double halfY2 = geoTrd.getYHalfLength2();
  double halfZ = geoTrd.getZHalfLength();

  // The diffs
  double diffX = std::abs(halfX2 - halfX1);
  double diffY = std::abs(halfY2 - halfY1);

  // If both X and Y are trapezoidal - consider
  if (diffX > 2 * std::numeric_limits<double>::epsilon() &&
      diffY > 2 * std::numeric_limits<double>::epsilon()) {
    throw std::invalid_argument(
        "GeoModelSurfaceConverter: GeoTrd conversion to Trapezoid "
        "ambiguous.");
  }

  // And its interpretation
  double minHalfX = unitLength * (diffX > diffY ? halfX1 : halfY1);
  double maxHalfX = unitLength * (diffX > diffY ? halfX2 : halfY2);
  double thickness = unitLength * (diffX > diffY ? diffY : diffX);

  // This is a convention of the TrapezoidBounds
  int swapZ = (maxHalfX < minHalfX) ? -1 : 1;
  if (swapZ < 0) {
    std::swap(minHalfX, maxHalfX);
  }

  // Adjust the rotation matrix
  RotationMatrix3 trotation = absTransform.rotation();

  if (diffX > diffY) {
    // Rotation is x, z, y ... acyclic, hence the sign change
    trotation.col(1) = swapZ * absTransform.rotation().col(2);
    trotation.col(2) = -swapZ * absTransform.rotation().col(1);
  } else {
    // Rotation is y, z, x ... cyclic, hence no sign change
    trotation.col(0) = absTransform.rotation().col(1);
    trotation.col(1) = swapZ * absTransform.rotation().col(2);
    trotation.col(2) = swapZ * absTransform.rotation().col(0);
  }
  transform.linear() = trotation;

  auto trapezoidBounds =
      std::make_shared<TrapezoidBounds>(minHalfX, maxHalfX, halfZ);

  if (!sensitive) {
    auto surface =
        Surface::makeShared<PlaneSurface>(transform, trapezoidBounds);
    return std::make_tuple(nullptr, surface);
  }

  // Create the element and the surface

  auto detectorElement =
      GeoModelDetectorElement::createDetectorElement<PlaneSurface>(
          geoPV, trapezoidBounds, transform, thickness);
  auto surface = detectorElement->surface().getSharedPtr();
  return std::make_tuple(detectorElement, surface);
}
