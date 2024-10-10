// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/ConeVolumeBounds.hpp"

#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Surfaces/ConeBounds.hpp"
#include "Acts/Surfaces/ConeSurface.hpp"
#include "Acts/Surfaces/ConvexPolygonBounds.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BoundingBox.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace Acts {
ConeVolumeBounds::ConeVolumeBounds(
    ActsScalar innerAlpha, ActsScalar innerOffsetZ, ActsScalar outerAlpha,
    ActsScalar outerOffsetZ, ActsScalar halflengthZ, ActsScalar averagePhi,
    ActsScalar halfPhiSector) noexcept(false)
    : VolumeBounds(), m_values() {
  m_values[eInnerAlpha] = innerAlpha;
  m_values[eInnerOffsetZ] = innerOffsetZ;
  m_values[eOuterAlpha] = outerAlpha;
  m_values[eOuterOffsetZ] = outerOffsetZ;
  m_values[eHalfLengthZ] = halflengthZ;
  m_values[eAveragePhi] = averagePhi;
  m_values[eHalfPhiSector] = halfPhiSector;
  buildSurfaceBounds();
  checkConsistency();
}

ConeVolumeBounds::ConeVolumeBounds(ActsScalar cylinderR, ActsScalar alpha,
                                   ActsScalar offsetZ, ActsScalar halflengthZ,
                                   ActsScalar averagePhi,
                                   ActsScalar halfPhiSector) noexcept(false)
    : VolumeBounds(), m_values() {
  m_values[eInnerAlpha] = 0.;
  m_values[eInnerOffsetZ] = 0.;
  m_values[eOuterAlpha] = 0.;
  m_values[eOuterOffsetZ] = 0.;
  m_values[eHalfLengthZ] = halflengthZ;
  m_values[eAveragePhi] = averagePhi;
  m_values[eHalfPhiSector] = halfPhiSector;

  // Cone parameters
  ActsScalar tanAlpha = std::tan(alpha);
  ActsScalar zmin = offsetZ - halflengthZ;
  ActsScalar zmax = offsetZ + halflengthZ;
  ActsScalar rmin = std::abs(zmin) * tanAlpha;
  ActsScalar rmax = std::abs(zmax) * tanAlpha;

  if (rmin >= cylinderR) {
    // Cylindrical cut-out of a cone
    m_innerRmin = cylinderR;
    m_innerRmax = cylinderR;
    m_outerTanAlpha = tanAlpha;
    m_outerRmin = rmin;
    m_outerRmax = rmax;
    m_values[eOuterAlpha] = alpha;
    m_values[eOuterOffsetZ] = offsetZ;
  } else if (rmax <= cylinderR) {
    // Conical cut-out of a cylinder
    m_outerRmin = cylinderR;
    m_outerRmax = cylinderR;
    m_innerTanAlpha = tanAlpha;
    m_innerRmin = rmin;
    m_innerRmax = rmax;
    m_values[eInnerAlpha] = alpha;
    m_values[eInnerOffsetZ] = offsetZ;
  } else {
    throw std::domain_error(
        "Cylinder and Cone are intersecting, not possible.");
  }
  buildSurfaceBounds();
  checkConsistency();
}

std::vector<Acts::OrientedSurface> Acts::ConeVolumeBounds::orientedSurfaces(
    const Transform3& transform) const {
  std::vector<OrientedSurface> oSurfaces;
  oSurfaces.reserve(6);

  // Create an inner Cone
  if (m_innerConeBounds != nullptr) {
    auto innerConeTrans = transform * Translation3(0., 0., -get(eInnerOffsetZ));
    auto innerCone =
        Surface::makeShared<ConeSurface>(innerConeTrans, m_innerConeBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(innerCone), Direction::AlongNormal});
  } else if (m_innerCylinderBounds != nullptr) {
    // Or alternatively the inner Cylinder
    auto innerCylinder =
        Surface::makeShared<CylinderSurface>(transform, m_innerCylinderBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(innerCylinder), Direction::AlongNormal});
  }

  // Create an outer Cone
  if (m_outerConeBounds != nullptr) {
    auto outerConeTrans = transform * Translation3(0., 0., -get(eOuterOffsetZ));
    auto outerCone =
        Surface::makeShared<ConeSurface>(outerConeTrans, m_outerConeBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(outerCone), Direction::OppositeNormal});
  } else if (m_outerCylinderBounds != nullptr) {
    // or alternatively an outer Cylinder
    auto outerCylinder =
        Surface::makeShared<CylinderSurface>(transform, m_outerCylinderBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(outerCylinder), Direction::OppositeNormal});
  }

  // Set a disc at Zmin
  if (m_negativeDiscBounds != nullptr) {
    auto negativeDiscTrans =
        transform * Translation3(0., 0., -get(eHalfLengthZ));
    auto negativeDisc = Surface::makeShared<DiscSurface>(negativeDiscTrans,
                                                         m_negativeDiscBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(negativeDisc), Direction::AlongNormal});
  }

  // Set a disc at Zmax
  auto positiveDiscTrans = transform * Translation3(0., 0., get(eHalfLengthZ));
  auto positiveDisc =
      Surface::makeShared<DiscSurface>(positiveDiscTrans, m_positiveDiscBounds);
  oSurfaces.push_back(
      OrientedSurface{std::move(positiveDisc), Direction::OppositeNormal});

  if (m_sectorBounds) {
    RotationMatrix3 sectorRotation;
    sectorRotation.col(0) = Vector3::UnitZ();
    sectorRotation.col(1) = Vector3::UnitX();
    sectorRotation.col(2) = Vector3::UnitY();

    Transform3 negSectorRelTrans{sectorRotation};
    negSectorRelTrans.prerotate(
        AngleAxis3(get(eAveragePhi) - get(eHalfPhiSector), Vector3::UnitZ()));
    auto negSectorAbsTrans = transform * negSectorRelTrans;
    auto negSectorPlane =
        Surface::makeShared<PlaneSurface>(negSectorAbsTrans, m_sectorBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(negSectorPlane), Direction::AlongNormal});

    Transform3 posSectorRelTrans{sectorRotation};
    posSectorRelTrans.prerotate(
        AngleAxis3(get(eAveragePhi) + get(eHalfPhiSector), Vector3::UnitZ()));
    auto posSectorAbsTrans = transform * posSectorRelTrans;
    auto posSectorPlane =
        Surface::makeShared<PlaneSurface>(posSectorAbsTrans, m_sectorBounds);

    oSurfaces.push_back(
        OrientedSurface{std::move(posSectorPlane), Direction::OppositeNormal});
  }
  return oSurfaces;
}

void ConeVolumeBounds::checkConsistency() noexcept(false) {
  if (innerRmin() > outerRmin() || innerRmax() > outerRmax()) {
    throw std::invalid_argument("ConeVolumeBounds: invalid radial input.");
  }
  if (get(eHalfLengthZ) <= 0) {
    throw std::invalid_argument(
        "ConeVolumeBounds: invalid longitudinal input.");
  }
  if (get(eHalfPhiSector) < 0. || get(eHalfPhiSector) > M_PI) {
    throw std::invalid_argument("ConeVolumeBounds: invalid phi sector setup.");
  }
  if (get(eAveragePhi) != detail::radian_sym(get(eAveragePhi))) {
    throw std::invalid_argument("ConeVolumeBounds: invalid phi positioning.");
  }
  if (get(eInnerAlpha) == 0. && get(eOuterAlpha) == 0.) {
    throw std::invalid_argument(
        "ConeVolumeBounds: neither inner nor outer cone.");
  }
}

bool ConeVolumeBounds::inside(const Vector3& pos, ActsScalar tol) const {
  ActsScalar z = pos.z();
  ActsScalar zmin = z + tol;
  ActsScalar zmax = z - tol;
  // Quick check outside z
  if (zmin < -get(eHalfLengthZ) || zmax > get(eHalfLengthZ)) {
    return false;
  }
  ActsScalar r = VectorHelpers::perp(pos);
  if (std::abs(get(eHalfPhiSector) - M_PI) > s_onSurfaceTolerance) {
    // need to check the phi sector - approximate phi tolerance
    ActsScalar phitol = tol / r;
    ActsScalar phi = VectorHelpers::phi(pos);
    ActsScalar phimin = phi - phitol;
    ActsScalar phimax = phi + phitol;
    if (phimin < get(eAveragePhi) - get(eHalfPhiSector) ||
        phimax > get(eAveragePhi) + get(eHalfPhiSector)) {
      return false;
    }
  }
  // We are within phi sector check box r quickly
  ActsScalar rmin = r + tol;
  ActsScalar rmax = r - tol;
  if (rmin > innerRmax() && rmax < outerRmin()) {
    return true;
  }
  // Finally we need to check the cone
  if (m_innerConeBounds != nullptr) {
    ActsScalar innerConeR =
        m_innerConeBounds->r(std::abs(z + get(eInnerOffsetZ)));
    if (innerConeR > rmin) {
      return false;
    }
  } else if (innerRmax() > rmin) {
    return false;
  }
  // And the outer cone
  if (m_outerConeBounds != nullptr) {
    ActsScalar outerConeR =
        m_outerConeBounds->r(std::abs(z + get(eOuterOffsetZ)));
    if (outerConeR < rmax) {
      return false;
    }
  } else if (outerRmax() < rmax) {
    return false;
  }
  return true;
}

void ConeVolumeBounds::buildSurfaceBounds() {
  // Build inner cone or inner cylinder
  if (get(eInnerAlpha) > s_epsilon) {
    m_innerTanAlpha = std::tan(get(eInnerAlpha));
    ActsScalar innerZmin = get(eInnerOffsetZ) - get(eHalfLengthZ);
    ActsScalar innerZmax = get(eInnerOffsetZ) + get(eHalfLengthZ);
    m_innerRmin = std::abs(innerZmin) * m_innerTanAlpha;
    m_innerRmax = std::abs(innerZmax) * m_innerTanAlpha;
    m_innerConeBounds =
        std::make_shared<ConeBounds>(get(eInnerAlpha), innerZmin, innerZmax,
                                     get(eHalfPhiSector), get(eAveragePhi));
  } else if (m_innerRmin == m_innerRmax && m_innerRmin > s_epsilon) {
    m_innerCylinderBounds = std::make_shared<CylinderBounds>(
        m_innerRmin, get(eHalfLengthZ), get(eHalfPhiSector), get(eAveragePhi));
  }

  if (get(eOuterAlpha) > s_epsilon) {
    m_outerTanAlpha = std::tan(get(eOuterAlpha));
    ActsScalar outerZmin = get(eOuterOffsetZ) - get(eHalfLengthZ);
    ActsScalar outerZmax = get(eOuterOffsetZ) + get(eHalfLengthZ);
    m_outerRmin = std::abs(outerZmin) * m_outerTanAlpha;
    m_outerRmax = std::abs(outerZmax) * m_outerTanAlpha;
    m_outerConeBounds =
        std::make_shared<ConeBounds>(get(eOuterAlpha), outerZmin, outerZmax,
                                     get(eHalfPhiSector), get(eAveragePhi));

  } else if (m_outerRmin == m_outerRmax) {
    m_outerCylinderBounds = std::make_shared<CylinderBounds>(
        m_outerRmax, get(eHalfLengthZ), get(eHalfPhiSector), get(eAveragePhi));
  }

  if (get(eHalfLengthZ) < std::max(get(eInnerOffsetZ), get(eOuterOffsetZ))) {
    m_negativeDiscBounds = std::make_shared<RadialBounds>(
        m_innerRmin, m_outerRmin, get(eHalfPhiSector), get(eAveragePhi));
  }

  m_positiveDiscBounds = std::make_shared<RadialBounds>(
      m_innerRmax, m_outerRmax, get(eHalfPhiSector), get(eAveragePhi));

  // Create the sector bounds
  if (std::abs(get(eHalfPhiSector) - M_PI) > s_epsilon) {
    // The 4 points building the sector
    std::vector<Vector2> polyVertices = {{-get(eHalfLengthZ), m_innerRmin},
                                         {get(eHalfLengthZ), m_innerRmax},
                                         {get(eHalfLengthZ), m_outerRmax},
                                         {-get(eHalfLengthZ), m_outerRmin}};
    m_sectorBounds =
        std::make_shared<ConvexPolygonBounds<4>>(std::move(polyVertices));
  }
}

std::ostream& ConeVolumeBounds::toStream(std::ostream& os) const {
  os << std::setiosflags(std::ios::fixed);
  os << std::setprecision(5);
  os << "Acts::ConeVolumeBounds : (innerAlpha, innerOffsetZ, outerAlpha,";
  os << "  outerOffsetZ, halflenghZ, averagePhi, halfPhiSector) = ";
  os << get(eInnerAlpha) << ", " << get(eInnerOffsetZ) << ", ";
  os << get(eOuterAlpha) << ", " << get(eOuterOffsetZ) << ", ";
  os << get(eHalfLengthZ) << ", " << get(eAveragePhi) << std::endl;
  return os;
}

Volume::BoundingBox ConeVolumeBounds::boundingBox(const Transform3* trf,
                                                  const Vector3& envelope,
                                                  const Volume* entity) const {
  Vector3 vmin(-outerRmax(), -outerRmax(), -0.5 * get(eHalfLengthZ));
  Vector3 vmax(outerRmax(), outerRmax(), 0.5 * get(eHalfLengthZ));
  Volume::BoundingBox box(entity, vmin - envelope, vmax + envelope);
  return trf == nullptr ? box : box.transformed(*trf);
}

ActsScalar ConeVolumeBounds::innerRmin() const {
  return m_innerRmin;
}

ActsScalar ConeVolumeBounds::innerRmax() const {
  return m_innerRmax;
}

ActsScalar ConeVolumeBounds::innerTanAlpha() const {
  return m_innerTanAlpha;
}

ActsScalar ConeVolumeBounds::outerRmin() const {
  return m_outerRmin;
}

ActsScalar ConeVolumeBounds::outerRmax() const {
  return m_outerRmax;
}

ActsScalar ConeVolumeBounds::outerTanAlpha() const {
  return m_outerTanAlpha;
}

std::vector<ActsScalar> ConeVolumeBounds::values() const {
  std::vector<ActsScalar> valvector;
  valvector.insert(valvector.begin(), m_values.begin(), m_values.end());
  return valvector;
}

}  // namespace Acts
