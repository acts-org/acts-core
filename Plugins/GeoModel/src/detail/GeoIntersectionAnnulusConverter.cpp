// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/detail/GeoIntersectionAnnulusConverter.hpp"

#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConversionError.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/detail/AnnulusBoundsHelper.hpp"

#include <algorithm>

#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoGenericTrap.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoShape.h>
#include <GeoModelKernel/GeoShapeIntersection.h>
#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/GeoTubs.h>

Acts::Result<Acts::GeoModelSensitiveSurface>
Acts::detail::GeoIntersectionAnnulusConverter::operator()(
    const PVConstLink& geoPV, const GeoShapeIntersection& geoIntersection,
    const Transform3& absTransform, bool sensitive) const {
  /// auto-calculate the unit length conversion
  static constexpr ActsScalar unitLength =
      Acts::UnitConstants::mm / GeoModelKernelUnits::millimeter;

  // Returns the first operand being ANDed
  const GeoShape* opA = geoIntersection.getOpA();
  const GeoShape* opB = geoIntersection.getOpB();

  const GeoTubs* tubs = dynamic_cast<const GeoTubs*>(opA);
  if (tubs != nullptr) {
    // rMin, rMax
    ActsScalar rMin = unitLength * tubs->getRMin();
    ActsScalar rMax = unitLength * tubs->getRMax();

    // Get the shift
    const GeoShapeShift* shapeShift = dynamic_cast<const GeoShapeShift*>(opB);
    if (shapeShift != nullptr) {
      const Transform3& shift = shapeShift->getX();
      const GeoGenericTrap* trap =
          dynamic_cast<const GeoGenericTrap*>(shapeShift->getOp());
      if (trap != nullptr) {
        ActsScalar thickness = 2 * unitLength * trap->getZHalfLength();
        //    X half length at -z, -y.
        auto trapVertices = trap->getVertices();

        std::vector<Vector2> faceVertices(trapVertices.begin(),
                                          trapVertices.begin() + 4u);
        // to make sure they are in the right order
        std::ranges::sort(faceVertices, std::greater{}, [](const auto& f) {
          return (VectorHelpers::phi(f));
        });

        // Turn them into global
        std::vector<Vector3> faceVertices3D;
        for (const auto& v : faceVertices) {
          faceVertices3D.push_back(
              shift * Vector3{unitLength * v.x(), unitLength * v.y(), 0.});
        }

        faceVertices.clear();
        for (auto& v : faceVertices3D) {
          faceVertices.push_back(v.block<2, 1>(0, 0));
        }

        auto [annulusBounds, annulusTransform] =
            Acts::detail::AnnulusBoundsHelper::create(absTransform, rMin, rMax,
                                                      faceVertices);
        if (!sensitive) {
          auto surface =
              Surface::makeShared<DiscSurface>(annulusTransform, annulusBounds);
          return std::make_tuple(nullptr, surface);
        }

        // Create the detector element
        auto detectorElement =
            GeoModelDetectorElement::createDetectorElement<DiscSurface>(
                geoPV, annulusBounds, annulusTransform, thickness);
        auto surface = detectorElement->surface().getSharedPtr();
        return std::make_tuple(detectorElement, surface);
      }
      return std::make_tuple(nullptr, nullptr);
    }
    return std::make_tuple(nullptr, nullptr);
  }
  return std::make_tuple(nullptr, nullptr);
}
