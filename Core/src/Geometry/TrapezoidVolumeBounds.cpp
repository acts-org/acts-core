// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"

#include "Acts/Definitions/Direction.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Utilities/BoundingBox.hpp"

#include <cmath>
#include <cstddef>
#include <utility>

namespace Acts {

TrapezoidVolumeBounds::TrapezoidVolumeBounds(ActsScalar minhalex,
                                             ActsScalar maxhalex,
                                             ActsScalar haley, ActsScalar halez)
    : VolumeBounds() {
  m_values[eHalfLengthXnegY] = minhalex;
  m_values[eHalfLengthXposY] = maxhalex;
  m_values[eHalfLengthY] = haley;
  m_values[eHalfLengthZ] = halez;
  m_values[eAlpha] = atan2(2 * haley, (maxhalex - minhalex));
  m_values[eBeta] = M_PI - get(eAlpha);
  checkConsistency();
  buildSurfaceBounds();
}

TrapezoidVolumeBounds::TrapezoidVolumeBounds(ActsScalar minhalex,
                                             ActsScalar haley, ActsScalar halez,
                                             ActsScalar alpha, ActsScalar beta)
    : VolumeBounds() {
  m_values[eHalfLengthXnegY] = minhalex;
  m_values[eHalfLengthY] = haley;
  m_values[eHalfLengthZ] = halez;
  m_values[eAlpha] = alpha;
  m_values[eBeta] = beta;
  // now calculate the remaining max half X
  ActsScalar gamma =
      (alpha > beta) ? (alpha - 0.5 * M_PI) : (beta - 0.5 * M_PI);
  m_values[eHalfLengthXposY] = minhalex + (2. * haley) * tan(gamma);

  checkConsistency();
  buildSurfaceBounds();
}

std::vector<OrientedSurface> TrapezoidVolumeBounds::orientedSurfaces(
    const Transform3& transform) const {
  std::vector<OrientedSurface> oSurfaces;
  oSurfaces.reserve(6);

  // Face surfaces xy
  RotationMatrix3 trapezoidRotation(transform.rotation());
  Vector3 trapezoidX(trapezoidRotation.col(0));
  Vector3 trapezoidY(trapezoidRotation.col(1));
  Vector3 trapezoidZ(trapezoidRotation.col(2));
  Vector3 trapezoidCenter(transform.translation());

  //   (1) - At negative local z
  auto nzTransform = transform * Translation3(0., 0., -get(eHalfLengthZ));
  auto sf =
      Surface::makeShared<PlaneSurface>(nzTransform, m_faceXYTrapezoidBounds);
  oSurfaces.push_back(OrientedSurface{std::move(sf), Direction::AlongNormal});
  //   (2) - At positive local z
  auto pzTransform = transform * Translation3(0., 0., get(eHalfLengthZ));
  sf = Surface::makeShared<PlaneSurface>(pzTransform, m_faceXYTrapezoidBounds);
  oSurfaces.push_back(
      OrientedSurface{std::move(sf), Direction::OppositeNormal});

  ActsScalar poshOffset = get(eHalfLengthY) / std::tan(get(eAlpha));
  ActsScalar neghOffset = get(eHalfLengthY) / std::tan(get(eBeta));
  ActsScalar topShift = poshOffset + neghOffset;

  // Face surfaces yz
  // (3) - At point B, attached to beta opening angle
  Vector3 fbPosition(-get(eHalfLengthXnegY) + neghOffset, 0., 0.);
  auto fbTransform = transform * Translation3(fbPosition) *
                     AngleAxis3(-0.5 * M_PI + get(eBeta), Vector3(0., 0., 1.)) *
                     s_planeYZ;
  sf =
      Surface::makeShared<PlaneSurface>(fbTransform, m_faceBetaRectangleBounds);
  oSurfaces.push_back(OrientedSurface{std::move(sf), Direction::AlongNormal});

  // (4) - At point A, attached to alpha opening angle
  Vector3 faPosition(get(eHalfLengthXnegY) + poshOffset, 0., 0.);
  auto faTransform =
      transform * Translation3(faPosition) *
      AngleAxis3(-0.5 * M_PI + get(eAlpha), Vector3(0., 0., 1.)) * s_planeYZ;
  sf = Surface::makeShared<PlaneSurface>(faTransform,
                                         m_faceAlphaRectangleBounds);
  oSurfaces.push_back(
      OrientedSurface{std::move(sf), Direction::OppositeNormal});

  // Face surfaces zx
  //   (5) - At negative local y
  auto nxTransform =
      transform * Translation3(0., -get(eHalfLengthY), 0.) * s_planeZX;
  sf = Surface::makeShared<PlaneSurface>(nxTransform,
                                         m_faceZXRectangleBoundsBottom);
  oSurfaces.push_back(OrientedSurface{std::move(sf), Direction::AlongNormal});
  //   (6) - At positive local y
  auto pxTransform =
      transform * Translation3(topShift, get(eHalfLengthY), 0.) * s_planeZX;
  sf = Surface::makeShared<PlaneSurface>(pxTransform,
                                         m_faceZXRectangleBoundsTop);
  oSurfaces.push_back(
      OrientedSurface{std::move(sf), Direction::OppositeNormal});

  return oSurfaces;
}

void TrapezoidVolumeBounds::buildSurfaceBounds() {
  m_faceXYTrapezoidBounds = std::make_shared<const TrapezoidBounds>(
      get(eHalfLengthXnegY), get(eHalfLengthXposY), get(eHalfLengthY));

  m_faceAlphaRectangleBounds = std::make_shared<const RectangleBounds>(
      get(eHalfLengthY) / cos(get(eAlpha) - 0.5 * M_PI), get(eHalfLengthZ));

  m_faceBetaRectangleBounds = std::make_shared<const RectangleBounds>(
      get(eHalfLengthY) / cos(get(eBeta) - 0.5 * M_PI), get(eHalfLengthZ));

  m_faceZXRectangleBoundsBottom = std::make_shared<const RectangleBounds>(
      get(eHalfLengthZ), get(eHalfLengthXnegY));

  m_faceZXRectangleBoundsTop = std::make_shared<const RectangleBounds>(
      get(eHalfLengthZ), get(eHalfLengthXposY));
}

bool TrapezoidVolumeBounds::inside(const Vector3& pos, ActsScalar tol) const {
  if (std::abs(pos.z()) > get(eHalfLengthZ) + tol) {
    return false;
  }
  if (std::abs(pos.y()) > get(eHalfLengthY) + tol) {
    return false;
  }
  Vector2 locp(pos.x(), pos.y());
  bool inside(m_faceXYTrapezoidBounds->inside(
      locp, BoundaryTolerance::AbsoluteBound{tol, tol}));
  return inside;
}

std::ostream& TrapezoidVolumeBounds::toStream(std::ostream& os) const {
  os << std::setiosflags(std::ios::fixed);
  os << std::setprecision(5);
  os << "TrapezoidVolumeBounds: (minhalfX, halfY, halfZ, alpha, beta) "
        "= ";
  os << "(" << get(eHalfLengthXnegY) << ", " << get(eHalfLengthXposY) << ", "
     << get(eHalfLengthY) << ", " << get(eHalfLengthZ);
  os << ", " << get(eAlpha) << ", " << get(eBeta) << ")";
  return os;
}

Volume::BoundingBox TrapezoidVolumeBounds::boundingBox(
    const Transform3* trf, const Vector3& envelope,
    const Volume* entity) const {
  ActsScalar minx = get(eHalfLengthXnegY);
  ActsScalar maxx = get(eHalfLengthXposY);
  ActsScalar haley = get(eHalfLengthY);
  ActsScalar halez = get(eHalfLengthZ);

  std::array<Vector3, 8> vertices = {{{-minx, -haley, -halez},
                                      {+minx, -haley, -halez},
                                      {-maxx, +haley, -halez},
                                      {+maxx, +haley, -halez},
                                      {-minx, -haley, +halez},
                                      {+minx, -haley, +halez},
                                      {-maxx, +haley, +halez},
                                      {+maxx, +haley, +halez}}};

  Transform3 transform = Transform3::Identity();
  if (trf != nullptr) {
    transform = *trf;
  }

  Vector3 vmin = transform * vertices[0];
  Vector3 vmax = transform * vertices[0];

  for (std::size_t i = 1; i < 8; i++) {
    const Vector3 vtx = transform * vertices[i];
    vmin = vmin.cwiseMin(vtx);
    vmax = vmax.cwiseMax(vtx);
  }

  return {entity, vmin - envelope, vmax + envelope};
}

std::vector<ActsScalar> TrapezoidVolumeBounds::values() const {
  std::vector<ActsScalar> valvector;
  valvector.insert(valvector.begin(), m_values.begin(), m_values.end());
  return valvector;
}

void TrapezoidVolumeBounds::checkConsistency() noexcept(false) {
  if (get(eHalfLengthXnegY) < 0. || get(eHalfLengthXposY) < 0.) {
    throw std::invalid_argument(
        "TrapezoidVolumeBounds: invalid trapezoid parameters in x.");
  }
  if (get(eHalfLengthY) <= 0.) {
    throw std::invalid_argument("TrapezoidVolumeBounds: invalid y extrusion.");
  }
  if (get(eHalfLengthZ) <= 0.) {
    throw std::invalid_argument("TrapezoidVolumeBounds: invalid z extrusion.");
  }
}

}  // namespace Acts
