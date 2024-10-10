// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/detail/GeoUnionDoubleTrdConverter.hpp"

#include "Acts/Plugins/GeoModel/GeoModelConversionError.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoShiftConverter.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"

namespace {

auto distanceLinePoint(const Acts::Vector3 &lineA, const Acts::Vector3 &lineB,
                       const Acts::Vector3 &p) {
  auto dir = lineB - lineA;
  auto ap = p - lineA;
  return ap.cross(dir).norm() / dir.norm();
}

/// Checks with the following properties if the trapezoids are mergeable
bool trapezoidsAreMergeable(const std::vector<Acts::Vector3> &vtxsa,
                            const std::vector<Acts::Vector3> &vtxsb) {
  // Compute the distance of the lines connecting A3 and B0 and the midpoint of
  // the gap (resp. for other trapezoid side) These should be close to zero,
  // otherwise we cannot merge the trapezoids
  auto P1 = vtxsa[0] + 0.5 * (vtxsb[3] - vtxsa[0]);
  auto dist1 = distanceLinePoint(vtxsa[3], vtxsb[0], P1);

  auto P2 = vtxsa[1] + 0.5 * (vtxsb[2] - vtxsa[1]);
  auto dist2 = distanceLinePoint(vtxsa[2], vtxsb[1], P2);

  if (dist1 > 1.e-3 || dist2 > 1.e-3) {
    return false;
  }

  // We could verify other properties, but this seems sufficient for know
  return true;
}

}  // namespace

namespace Acts::detail {

Result<GeoModelSensitiveSurface> GeoUnionDoubleTrdConverter::operator()(
    const PVConstLink &geoPV, const GeoShapeUnion &geoUnion,
    const Transform3 &absTransform, bool sensitive) const {
  const auto shiftA = dynamic_cast<const GeoShapeShift *>(geoUnion.getOpA());
  const auto shiftB = dynamic_cast<const GeoShapeShift *>(geoUnion.getOpB());

  if (shiftA == nullptr || shiftB == nullptr) {
    return GeoModelConversionError::WrongShapeForConverter;
  }

  auto shiftARes =
      detail::GeoShiftConverter{}(geoPV, *shiftA, absTransform, sensitive);
  if (!shiftARes.ok()) {
    return shiftARes.error();
  }
  auto shiftBRes =
      detail::GeoShiftConverter{}(geoPV, *shiftB, absTransform, sensitive);
  if (!shiftBRes.ok()) {
    return shiftBRes.error();
  }

  const auto &[elA, surfaceA] = shiftARes.value();
  const auto &[elB, surfaceB] = shiftBRes.value();

  if (!(surfaceA && surfaceB)) {
    return GeoModelConversionError::WrongShapeForConverter;
  }

  if (surfaceA->bounds().type() != Acts::SurfaceBounds::eTrapezoid ||
      surfaceB->bounds().type() != Acts::SurfaceBounds::eTrapezoid) {
    return GeoModelConversionError::WrongShapeForConverter;
  }

  // At this point we know, that we have two trapezoids
  // We assume the following Layout for this:
  //
  //  0 ______________________ 1
  //    \                    /
  //     \        B         /
  //      \________________/
  //     3                 2
  //      0 _____________ 1
  //        \           /
  //         \    A    /
  //          \_______/
  //          3       2

  // First check now, if this actually is correct
  const auto vtxsa = surfaceA->polyhedronRepresentation({}, 0).vertices;
  const auto vtxsb = surfaceB->polyhedronRepresentation({}, 0).vertices;

  if (!trapezoidsAreMergeable(vtxsa, vtxsb)) {
    return GeoModelConversionError::WrongShapeForConverter;
  }

  // Compute half length y as the distance between the middle points of the
  // outer lines of the trapezoids
  Acts::Vector3 mpA = vtxsa[3] + 0.5 * (vtxsa[2] - vtxsa[3]);
  Acts::Vector3 mpB = vtxsb[0] + 0.5 * (vtxsb[1] - vtxsb[0]);

  auto halfLengthY = 0.5 * (mpB - mpA).norm();

  // Compute the gap between trapezoids
  const auto &boundsA =
      static_cast<const TrapezoidBounds &>(surfaceA->bounds());
  const auto &boundsB =
      static_cast<const TrapezoidBounds &>(surfaceB->bounds());

  const auto gap =
      halfLengthY - (boundsA.values()[TrapezoidBounds::eHalfLengthY] +
                     boundsB.values()[TrapezoidBounds::eHalfLengthY]);

  if (gap > gapTolerance) {
    return GeoModelConversionError::WrongShapeForConverter;
  }

  // For the x half lengths, we can take the values from the 2 trapezoids
  auto hlxny = boundsA.values()[TrapezoidBounds::eHalfLengthXposY];
  auto hlxpy = boundsB.values()[TrapezoidBounds::eHalfLengthXnegY];

  auto trapezoidBounds =
      std::make_shared<TrapezoidBounds>(hlxpy, hlxny, halfLengthY);

  // Create transform from the transform of surfaceA and translate it in y
  // direction using the half length
  auto transform = surfaceA->transform({});
  transform.translate(Vector3{
      0.f, boundsA.values()[TrapezoidBounds::eHalfLengthY] - halfLengthY, 0.f});

  // TODO extract this code snipped since it is copied quite a lot
  if (!sensitive) {
    auto surface =
        Surface::makeShared<PlaneSurface>(transform, trapezoidBounds);
    return std::make_tuple(nullptr, surface);
  }

  // Create the element and the surface (we assume both have equal thickness)
  auto detectorElement =
      GeoModelDetectorElement::createDetectorElement<PlaneSurface>(
          geoPV, trapezoidBounds, transform, elA->thickness());
  auto surface = detectorElement->surface().getSharedPtr();

  return std::make_tuple(detectorElement, surface);
}

}  // namespace Acts::detail
