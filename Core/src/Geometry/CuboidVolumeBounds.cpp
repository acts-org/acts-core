// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/CuboidVolumeBounds.hpp"

#include "Acts/Definitions/Direction.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BoundingBox.hpp"

#include <utility>

namespace Acts {

CuboidVolumeBounds::CuboidVolumeBounds(ActsScalar halex, ActsScalar haley,
                                       ActsScalar halez)
    : VolumeBounds(), m_values({halex, haley, halez}) {
  checkConsistency();
  buildSurfaceBounds();
}

CuboidVolumeBounds::CuboidVolumeBounds(
    const std::array<ActsScalar, eSize>& values)
    : m_values(values) {
  checkConsistency();
  buildSurfaceBounds();
}

std::vector<Acts::OrientedSurface> Acts::CuboidVolumeBounds::orientedSurfaces(
    const Transform3& transform) const {
  std::vector<OrientedSurface> oSurfaces;
  oSurfaces.reserve(6);
  // Face surfaces xy -------------------------------------
  //   (1) - at negative local z
  auto sf = Surface::makeShared<PlaneSurface>(
      transform * Translation3(0., 0., -get(eHalfLengthZ)), m_xyBounds);
  oSurfaces.push_back(OrientedSurface{std::move(sf), Direction::AlongNormal});
  //   (2) - at positive local z
  sf = Surface::makeShared<PlaneSurface>(
      transform * Translation3(0., 0., get(eHalfLengthZ)), m_xyBounds);
  oSurfaces.push_back(
      OrientedSurface{std::move(sf), Direction::OppositeNormal});
  // Face surfaces yz -------------------------------------
  //   (3) - at negative local x
  sf = Surface::makeShared<PlaneSurface>(
      transform * Translation3(-get(eHalfLengthX), 0., 0.) * s_planeYZ,
      m_yzBounds);
  oSurfaces.push_back(OrientedSurface{std::move(sf), Direction::AlongNormal});
  //   (4) - at positive local x
  sf = Surface::makeShared<PlaneSurface>(
      transform * Translation3(get(eHalfLengthX), 0., 0.) * s_planeYZ,
      m_yzBounds);
  oSurfaces.push_back(
      OrientedSurface{std::move(sf), Direction::OppositeNormal});
  // Face surfaces zx -------------------------------------
  //   (5) - at negative local y
  sf = Surface::makeShared<PlaneSurface>(
      transform * Translation3(0., -get(eHalfLengthY), 0.) * s_planeZX,
      m_zxBounds);
  oSurfaces.push_back(OrientedSurface{std::move(sf), Direction::AlongNormal});
  //   (6) - at positive local y
  sf = Surface::makeShared<PlaneSurface>(
      transform * Translation3(0., get(eHalfLengthY), 0.) * s_planeZX,
      m_zxBounds);
  oSurfaces.push_back(
      OrientedSurface{std::move(sf), Direction::OppositeNormal});

  return oSurfaces;
}

std::ostream& CuboidVolumeBounds::toStream(std::ostream& os) const {
  os << std::setiosflags(std::ios::fixed);
  os << std::setprecision(5);
  os << "Acts::CuboidVolumeBounds: (halfLengthX, halfLengthY, halfLengthZ) = ";
  os << "(" << get(eHalfLengthX) << ", " << get(eHalfLengthY) << ", "
     << get(eHalfLengthZ) << ")";
  return os;
}

Volume::BoundingBox CuboidVolumeBounds::boundingBox(
    const Transform3* trf, const Vector3& envelope,
    const Volume* entity) const {
  Vector3 vmin(-get(eHalfLengthX), -get(eHalfLengthY), -get(eHalfLengthZ));
  Vector3 vmax(get(eHalfLengthX), get(eHalfLengthY), get(eHalfLengthZ));

  Volume::BoundingBox box(entity, vmin - envelope, vmax + envelope);
  return trf == nullptr ? box : box.transformed(*trf);
}

void CuboidVolumeBounds::buildSurfaceBounds() {
  m_xyBounds = std::make_shared<const RectangleBounds>(get(eHalfLengthX),
                                                       get(eHalfLengthY));
  m_yzBounds = std::make_shared<const RectangleBounds>(get(eHalfLengthY),
                                                       get(eHalfLengthZ));
  m_zxBounds = std::make_shared<const RectangleBounds>(get(eHalfLengthZ),
                                                       get(eHalfLengthX));
}

ActsScalar CuboidVolumeBounds::binningBorder(BinningValue bValue) const {
  if (bValue <= BinningValue::binZ) {
    return m_values[toUnderlying(bValue)];
  }
  if (bValue == BinningValue::binR) {
    return std::sqrt(m_values[toUnderlying(BinningValue::binX)] *
                         m_values[toUnderlying(BinningValue::binX)] +
                     m_values[toUnderlying(BinningValue::binY)] *
                         m_values[toUnderlying(BinningValue::binY)]);
  }
  return 0.0;
}

bool CuboidVolumeBounds::inside(const Vector3& pos, ActsScalar tol) const {
  return (std::abs(pos.x()) <= get(eHalfLengthX) + tol &&
          std::abs(pos.y()) <= get(eHalfLengthY) + tol &&
          std::abs(pos.z()) <= get(eHalfLengthZ) + tol);
}

std::vector<ActsScalar> CuboidVolumeBounds::values() const {
  std::vector<ActsScalar> valvector;
  valvector.insert(valvector.begin(), m_values.begin(), m_values.end());
  return valvector;
}

void CuboidVolumeBounds::checkConsistency() noexcept(false) {
  if (get(eHalfLengthX) <= 0 || get(eHalfLengthY) <= 0 ||
      get(eHalfLengthZ) <= 0.) {
    throw std::invalid_argument(
        "CuboidVolumeBounds: invalid input, zero or negative.");
  }
}

void CuboidVolumeBounds::set(BoundValues bValue, ActsScalar value) {
  set({{bValue, value}});
}

void CuboidVolumeBounds::set(
    std::initializer_list<std::pair<BoundValues, ActsScalar>> keyValues) {
  std::array<ActsScalar, eSize> previous = m_values;
  for (const auto& [key, value] : keyValues) {
    m_values[key] = value;
  }
  try {
    checkConsistency();
    buildSurfaceBounds();
  } catch (std::invalid_argument& e) {
    m_values = previous;
    throw e;
  }
}

}  // namespace Acts
