// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/detail/GeoShiftConverter.hpp"

#include "Acts/Plugins/GeoModel/GeoModelConversionError.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoBoxConverter.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoTrdConverter.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoTubeConverter.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"

#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/GeoTube.h>

namespace Acts::detail {

namespace {

template <typename ContainedShape, typename Converter, typename Surface,
          typename Bounds>
Result<GeoModelSensitiveSurface> impl(PVConstLink geoPV,
                                      const GeoShapeShift& geoShift,
                                      const Transform3& absTransform,
                                      bool sensitive) {
  auto trd = dynamic_cast<const ContainedShape*>(geoShift.getOp());

  if (trd == nullptr) {
    return GeoModelConversionError::WrongShapeForConverter;
    ;
  }

  const Transform3& shift = geoShift.getX();

  const auto& conversionRes =
      Converter{}(geoPV, *trd, absTransform * shift, sensitive);
  if (!conversionRes.ok()) {
    return conversionRes.error();
  }
  auto [el, surface] = conversionRes.value();

  // Use knowledge from GeoTrdConverter to make shared bounds object
  const auto& bounds = static_cast<const Bounds&>(surface->bounds());
  auto sharedBounds = std::make_shared<const Bounds>(bounds);

  // TODO this procedure could be stripped from all converters because it is
  // pretty generic
  if (!sensitive) {
    auto newSurface = Surface::template makeShared<Surface>(
        surface->transform({}), sharedBounds);
    return std::make_tuple(nullptr, newSurface);
  }

  auto newEl = GeoModelDetectorElement::createDetectorElement<Surface>(
      el->physicalVolume(), sharedBounds, el->transform({}), el->thickness());
  auto newSurface = newEl->surface().getSharedPtr();
  return std::make_tuple(newEl, newSurface);
}

}  // namespace

Result<GeoModelSensitiveSurface> GeoShiftConverter::operator()(
    const PVConstLink& geoPV, const GeoShapeShift& geoShift,
    const Transform3& absTransform, bool sensitive) const {
  auto r = impl<GeoTrd, detail::GeoTrdConverter, PlaneSurface, TrapezoidBounds>(
      geoPV, geoShift, absTransform, sensitive);

  if (r.ok()) {
    return r;
  }

  r = impl<GeoBox, detail::GeoBoxConverter, PlaneSurface, RectangleBounds>(
      geoPV, geoShift, absTransform, sensitive);

  if (r.ok()) {
    return r;
  }

  // For now this does straw by default
  r = impl<GeoTube, detail::GeoTubeConverter, StrawSurface, LineBounds>(
      geoPV, geoShift, absTransform, sensitive);

  if (r.ok()) {
    return r;
  }

  return GeoModelConversionError::WrongShapeForConverter;
}

}  // namespace Acts::detail
