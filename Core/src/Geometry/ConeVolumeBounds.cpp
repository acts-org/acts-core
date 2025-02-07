// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
#include <numbers>
#include <stdexcept>
#include <utility>

namespace Acts {

ConeVolumeBounds::ConeVolumeBounds(double innerAlpha, double innerOffsetZ,
                                   double outerAlpha, double outerOffsetZ,
                                   double halflengthZ, double averagePhi,
                                   double halfPhiSector) noexcept(false)
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

ConeVolumeBounds::ConeVolumeBounds(double cylinderR, double alpha,
                                   double offsetZ, double halflengthZ,
                                   double averagePhi,
                                   double halfPhiSector) noexcept(false)
    : VolumeBounds(), m_values() {
  m_values[eInnerAlpha] = 0.;
  m_values[eInnerOffsetZ] = 0.;
  m_values[eOuterAlpha] = 0.;
  m_values[eOuterOffsetZ] = 0.;
  m_values[eHalfLengthZ] = halflengthZ;
  m_values[eAveragePhi] = averagePhi;
  m_values[eHalfPhiSector] = halfPhiSector;

  // Cone parameters
  double tanAlpha = std::tan(alpha);
  double zmin = offsetZ - halflengthZ;
  double zmax = offsetZ + halflengthZ;
  double rmin = std::abs(zmin) * tanAlpha;
  double rmax = std::abs(zmax) * tanAlpha;

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

std::vector<double> ConeVolumeBounds::values() const {
  return {m_values.begin(), m_values.end()};
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
        OrientedSurface{std::move(innerCone), Direction::AlongNormal()});
  } else if (m_innerCylinderBounds != nullptr) {
    // Or alternatively the inner Cylinder
    auto innerCylinder =
        Surface::makeShared<CylinderSurface>(transform, m_innerCylinderBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(innerCylinder), Direction::AlongNormal()});
  }

  // Create an outer Cone
  if (m_outerConeBounds != nullptr) {
    auto outerConeTrans = transform * Translation3(0., 0., -get(eOuterOffsetZ));
    auto outerCone =
        Surface::makeShared<ConeSurface>(outerConeTrans, m_outerConeBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(outerCone), Direction::OppositeNormal()});
  } else if (m_outerCylinderBounds != nullptr) {
    // or alternatively an outer Cylinder
    auto outerCylinder =
        Surface::makeShared<CylinderSurface>(transform, m_outerCylinderBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(outerCylinder), Direction::OppositeNormal()});
  }

  // Set a disc at Zmin
  if (m_negativeDiscBounds != nullptr) {
    auto negativeDiscTrans =
        transform * Translation3(0., 0., -get(eHalfLengthZ));
    auto negativeDisc = Surface::makeShared<DiscSurface>(negativeDiscTrans,
                                                         m_negativeDiscBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(negativeDisc), Direction::AlongNormal()});
  }

  // Set a disc at Zmax
  auto positiveDiscTrans = transform * Translation3(0., 0., get(eHalfLengthZ));
  auto positiveDisc =
      Surface::makeShared<DiscSurface>(positiveDiscTrans, m_positiveDiscBounds);
  oSurfaces.push_back(
      OrientedSurface{std::move(positiveDisc), Direction::OppositeNormal()});

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
        OrientedSurface{std::move(negSectorPlane), Direction::AlongNormal()});

    Transform3 posSectorRelTrans{sectorRotation};
    posSectorRelTrans.prerotate(
        AngleAxis3(get(eAveragePhi) + get(eHalfPhiSector), Vector3::UnitZ()));
    auto posSectorAbsTrans = transform * posSectorRelTrans;
    auto posSectorPlane =
        Surface::makeShared<PlaneSurface>(posSectorAbsTrans, m_sectorBounds);

    oSurfaces.push_back(OrientedSurface{std::move(posSectorPlane),
                                        Direction::OppositeNormal()});
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
  if (get(eHalfPhiSector) < 0. || get(eHalfPhiSector) > std::numbers::pi) {
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

bool ConeVolumeBounds::inside(const Vector3& pos, double tol) const {
  double z = pos.z();
  double zmin = z + tol;
  double zmax = z - tol;
  // Quick check outside z
  if (zmin < -get(eHalfLengthZ) || zmax > get(eHalfLengthZ)) {
    return false;
  }
  double r = VectorHelpers::perp(pos);
  if (std::abs(get(eHalfPhiSector) - std::numbers::pi) > s_onSurfaceTolerance) {
    // need to check the phi sector - approximate phi tolerance
    double phitol = tol / r;
    double phi = VectorHelpers::phi(pos);
    double phimin = phi - phitol;
    double phimax = phi + phitol;
    if (phimin < get(eAveragePhi) - get(eHalfPhiSector) ||
        phimax > get(eAveragePhi) + get(eHalfPhiSector)) {
      return false;
    }
  }
  // We are within phi sector check box r quickly
  double rmin = r + tol;
  double rmax = r - tol;
  if (rmin > innerRmax() && rmax < outerRmin()) {
    return true;
  }
  // Finally we need to check the cone
  if (m_innerConeBounds != nullptr) {
    double innerConeR = m_innerConeBounds->r(std::abs(z + get(eInnerOffsetZ)));
    if (innerConeR > rmin) {
      return false;
    }
  } else if (innerRmax() > rmin) {
    return false;
  }
  // And the outer cone
  if (m_outerConeBounds != nullptr) {
    double outerConeR = m_outerConeBounds->r(std::abs(z + get(eOuterOffsetZ)));
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
    double innerZmin = get(eInnerOffsetZ) - get(eHalfLengthZ);
    double innerZmax = get(eInnerOffsetZ) + get(eHalfLengthZ);
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
    double outerZmin = get(eOuterOffsetZ) - get(eHalfLengthZ);
    double outerZmax = get(eOuterOffsetZ) + get(eHalfLengthZ);
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
  if (std::abs(get(eHalfPhiSector) - std::numbers::pi) > s_epsilon) {
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

double ConeVolumeBounds::innerRmin() const {
  return m_innerRmin;
}

double ConeVolumeBounds::innerRmax() const {
  return m_innerRmax;
}

double ConeVolumeBounds::innerTanAlpha() const {
  return m_innerTanAlpha;
}

double ConeVolumeBounds::outerRmin() const {
  return m_outerRmin;
}

double ConeVolumeBounds::outerRmax() const {
  return m_outerRmax;
}

double ConeVolumeBounds::outerTanAlpha() const {
  return m_outerTanAlpha;
}

}  // namespace Acts
