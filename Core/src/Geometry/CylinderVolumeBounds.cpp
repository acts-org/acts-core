// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/CylinderVolumeBounds.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BoundingBox.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <cmath>
#include <numbers>
#include <utility>

namespace Acts {

CylinderVolumeBounds::CylinderVolumeBounds(double rmin, double rmax,
                                           double halfz, double halfphi,
                                           double avgphi, double bevelMinZ,
                                           double bevelMaxZ)
    : m_values() {
  m_values[eMinR] = rmin;
  m_values[eMaxR] = rmax;
  m_values[eHalfLengthZ] = halfz;
  m_values[eHalfPhiSector] = halfphi;
  m_values[eAveragePhi] = avgphi;
  m_values[eBevelMinZ] = bevelMinZ;
  m_values[eBevelMaxZ] = bevelMaxZ;
  checkConsistency();
  buildSurfaceBounds();
}

CylinderVolumeBounds::CylinderVolumeBounds(
    const std::array<double, eSize>& values)
    : m_values(values) {
  checkConsistency();
  buildSurfaceBounds();
}

CylinderVolumeBounds::CylinderVolumeBounds(const CylinderBounds& cBounds,
                                           double thickness)
    : VolumeBounds() {
  double cR = cBounds.get(CylinderBounds::eR);
  if (thickness <= 0. || (cR - 0.5 * thickness) < 0.) {
    throw(std::invalid_argument(
        "CylinderVolumeBounds: invalid extrusion thickness."));
  }
  m_values[eMinR] = cR - 0.5 * thickness;
  m_values[eMaxR] = cR + 0.5 * thickness;
  m_values[eHalfLengthZ] = cBounds.get(CylinderBounds::eHalfLengthZ);
  m_values[eHalfPhiSector] = cBounds.get(CylinderBounds::eHalfPhiSector);
  m_values[eAveragePhi] = cBounds.get(CylinderBounds::eAveragePhi);
  m_values[eBevelMinZ] = cBounds.get(CylinderBounds::eBevelMinZ);
  m_values[eBevelMaxZ] = cBounds.get(CylinderBounds::eBevelMaxZ);
  buildSurfaceBounds();
}

CylinderVolumeBounds::CylinderVolumeBounds(const RadialBounds& rBounds,
                                           double thickness)
    : VolumeBounds() {
  if (thickness <= 0.) {
    throw(std::invalid_argument(
        "CylinderVolumeBounds: invalid extrusion thickness."));
  }
  m_values[eMinR] = rBounds.get(RadialBounds::eMinR);
  m_values[eMaxR] = rBounds.get(RadialBounds::eMaxR);
  m_values[eHalfLengthZ] = 0.5 * thickness;
  m_values[eHalfPhiSector] = rBounds.get(RadialBounds::eHalfPhiSector);
  m_values[eAveragePhi] = rBounds.get(RadialBounds::eAveragePhi);
  m_values[eBevelMinZ] = 0.;
  m_values[eBevelMaxZ] = 0.;
  buildSurfaceBounds();
}

std::vector<OrientedSurface> CylinderVolumeBounds::orientedSurfaces(
    const Transform3& transform) const {
  std::vector<OrientedSurface> oSurfaces;
  oSurfaces.reserve(6);

  Translation3 vMinZ(0., 0., -get(eHalfLengthZ));
  Translation3 vMaxZ(0., 0., get(eHalfLengthZ));
  // Set up transform for beveled edges if they are defined
  double bevelMinZ = get(eBevelMinZ);
  double bevelMaxZ = get(eBevelMaxZ);
  Transform3 transMinZ, transMaxZ;
  if (bevelMinZ != 0.) {
    // double sy = 1 - 1 / std::cos(bevelMinZ);
    // transMinZ = transform * vMinZ *
    //             Eigen::AngleAxisd(-bevelMinZ, Eigen::Vector3d(1., 0., 0.)) *
    //             Eigen::Scaling(1., 1. + sy, 1.);
    throw std::invalid_argument("Bevel Min Z != 0 is not supported");
  } else {
    transMinZ = transform * vMinZ;
  }
  if (bevelMaxZ != 0.) {
    // double sy = 1 - 1 / std::cos(bevelMaxZ);
    // transMaxZ = transform * vMaxZ *
    //             Eigen::AngleAxisd(bevelMaxZ, Eigen::Vector3d(1., 0., 0.)) *
    //             Eigen::Scaling(1., 1. + sy, 1.);
    throw std::invalid_argument("Bevel Max Z != 0 is not supported");
  } else {
    transMaxZ = transform * vMaxZ;
  }
  // [0] Bottom Disc (negative z)
  auto dSurface = Surface::makeShared<DiscSurface>(transMinZ, m_discBounds);
  oSurfaces.push_back(
      OrientedSurface{std::move(dSurface), Direction::AlongNormal()});
  // [1] Top Disc (positive z)
  dSurface = Surface::makeShared<DiscSurface>(transMaxZ, m_discBounds);
  oSurfaces.push_back(
      OrientedSurface{std::move(dSurface), Direction::OppositeNormal()});

  // [2] Outer Cylinder
  auto cSurface =
      Surface::makeShared<CylinderSurface>(transform, m_outerCylinderBounds);
  oSurfaces.push_back(
      OrientedSurface{std::move(cSurface), Direction::OppositeNormal()});

  // [3] Inner Cylinder (optional)
  if (m_innerCylinderBounds != nullptr) {
    cSurface =
        Surface::makeShared<CylinderSurface>(transform, m_innerCylinderBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(cSurface), Direction::AlongNormal()});
  }

  // [4] & [5] - Sectoral planes (optional)
  if (m_sectorPlaneBounds != nullptr) {
    // sectorPlane 1 (negative phi)
    const Transform3 sp1Transform =
        Transform3(transform *
                   AngleAxis3(get(eAveragePhi) - get(eHalfPhiSector),
                              Vector3(0., 0., 1.)) *
                   Translation3(0.5 * (get(eMinR) + get(eMaxR)), 0., 0.) *
                   AngleAxis3(std::numbers::pi / 2, Vector3(1., 0., 0.)));
    auto pSurface =
        Surface::makeShared<PlaneSurface>(sp1Transform, m_sectorPlaneBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(pSurface), Direction::AlongNormal()});
    // sectorPlane 2 (positive phi)
    const Transform3 sp2Transform =
        Transform3(transform *
                   AngleAxis3(get(eAveragePhi) + get(eHalfPhiSector),
                              Vector3(0., 0., 1.)) *
                   Translation3(0.5 * (get(eMinR) + get(eMaxR)), 0., 0.) *
                   AngleAxis3(-std::numbers::pi / 2, Vector3(1., 0., 0.)));
    pSurface =
        Surface::makeShared<PlaneSurface>(sp2Transform, m_sectorPlaneBounds);
    oSurfaces.push_back(
        OrientedSurface{std::move(pSurface), Direction::OppositeNormal()});
  }
  return oSurfaces;
}

void CylinderVolumeBounds::buildSurfaceBounds() {
  if (get(eMinR) > s_epsilon) {
    m_innerCylinderBounds = std::make_shared<const CylinderBounds>(
        get(eMinR), get(eHalfLengthZ), get(eHalfPhiSector), get(eAveragePhi),
        get(eBevelMinZ), get(eBevelMaxZ));
  }
  m_outerCylinderBounds = std::make_shared<const CylinderBounds>(
      get(eMaxR), get(eHalfLengthZ), get(eHalfPhiSector), get(eAveragePhi),
      get(eBevelMinZ), get(eBevelMaxZ));
  m_discBounds = std::make_shared<const RadialBounds>(
      get(eMinR), get(eMaxR), get(eHalfPhiSector), get(eAveragePhi));

  if (std::abs(get(eHalfPhiSector) - std::numbers::pi) > s_epsilon) {
    m_sectorPlaneBounds = std::make_shared<const RectangleBounds>(
        0.5 * (get(eMaxR) - get(eMinR)), get(eHalfLengthZ));
  }
}

std::ostream& CylinderVolumeBounds::toStream(std::ostream& os) const {
  os << std::setiosflags(std::ios::fixed);
  os << std::setprecision(5);
  os << "CylinderVolumeBounds: (rMin, rMax, halfZ, halfPhi, "
        "averagePhi, minBevelZ, maxBevelZ) = ";
  os << get(eMinR) << ", " << get(eMaxR) << ", " << get(eHalfLengthZ) << ", "
     << get(eHalfPhiSector) << ", " << get(eAveragePhi) << ", "
     << get(eBevelMinZ) << ", " << get(eBevelMaxZ);
  return os;
}

Volume::BoundingBox CylinderVolumeBounds::boundingBox(
    const Transform3* trf, const Vector3& envelope,
    const Volume* entity) const {
  double xmax = 0, xmin = 0, ymax = 0, ymin = 0;
  xmax = get(eMaxR);

  if (get(eHalfPhiSector) > std::numbers::pi / 2.) {
    // more than half
    ymax = xmax;
    ymin = -xmax;
    xmin = xmax * std::cos(get(eHalfPhiSector));
  } else {
    // less than half
    ymax = get(eMaxR) * std::sin(get(eHalfPhiSector));
    ymin = -ymax;
    // in this case, xmin is given by the inner radius
    xmin = get(eMinR) * std::cos(get(eHalfPhiSector));
  }

  Vector3 vmin(xmin, ymin, -get(eHalfLengthZ));
  Vector3 vmax(xmax, ymax, get(eHalfLengthZ));

  // this is probably not perfect, but at least conservative
  Volume::BoundingBox box{entity, vmin - envelope, vmax + envelope};
  return trf == nullptr ? box : box.transformed(*trf);
}

bool CylinderVolumeBounds::inside(const Vector3& pos, double tol) const {
  using VectorHelpers::perp;
  using VectorHelpers::phi;
  double ros = perp(pos);
  bool insidePhi = cos(phi(pos)) >= cos(get(eHalfPhiSector)) - tol;
  bool insideR = insidePhi
                     ? ((ros >= get(eMinR) - tol) && (ros <= get(eMaxR) + tol))
                     : false;
  bool insideZ =
      insideR ? (std::abs(pos.z()) <= get(eHalfLengthZ) + tol) : false;
  return (insideZ && insideR && insidePhi);
}

Vector3 CylinderVolumeBounds::referenceOffset(AxisDirection aDir)
    const {  // the medium radius is taken for r-type binning
  if (aDir == Acts::AxisDirection::AxisR ||
      aDir == Acts::AxisDirection::AxisRPhi) {
    return Vector3(0.5 * (get(eMinR) + get(eMaxR)), 0., 0.);
  }
  return VolumeBounds::referenceOffset(aDir);
}

double CylinderVolumeBounds::referenceBorder(AxisDirection aDir) const {
  if (aDir == Acts::AxisDirection::AxisR) {
    return 0.5 * (get(eMaxR) - get(eMinR));
  }
  if (aDir == Acts::AxisDirection::AxisZ) {
    return get(eHalfLengthZ);
  }
  return VolumeBounds::referenceBorder(aDir);
}

std::vector<double> CylinderVolumeBounds::values() const {
  return {m_values.begin(), m_values.end()};
}

void CylinderVolumeBounds::checkConsistency() {
  if (get(eMinR) < 0. || get(eMaxR) <= 0.) {
    throw std::invalid_argument(
        "CylinderVolumeBounds: invalid radial input: minR (" +
        std::to_string(get(eMinR)) + ") < 0 or maxR (" +
        std::to_string(get(eMaxR)) + ") <= 0");
  }
  if (get(eMinR) >= get(eMaxR)) {
    throw std::invalid_argument(
        "CylinderVolumeBounds: invalid radial input: minR (" +
        std::to_string(get(eMinR)) + ") >= (" + std::to_string(get(eMaxR)) +
        ")");
  }
  if (get(eHalfLengthZ) <= 0) {
    throw std::invalid_argument(
        "CylinderVolumeBounds: invalid longitudinal input: hlZ (" +
        std::to_string(get(eHalfLengthZ)) + ") <= 0");
  }
  if (get(eHalfPhiSector) < 0. || get(eHalfPhiSector) > std::numbers::pi) {
    throw std::invalid_argument(
        "CylinderVolumeBounds: invalid phi sector setup.");
  }
  if (get(eAveragePhi) != detail::radian_sym(get(eAveragePhi))) {
    throw std::invalid_argument(
        "CylinderVolumeBounds: invalid phi positioning.");
  }
  if (get(eBevelMinZ) != detail::radian_sym(get(eBevelMinZ))) {
    throw std::invalid_argument("CylinderBounds: invalid bevel at min Z.");
  }
  if (get(eBevelMaxZ) != detail::radian_sym(get(eBevelMaxZ))) {
    throw std::invalid_argument("CylinderBounds: invalid bevel at max Z.");
  }
}

void CylinderVolumeBounds::set(BoundValues bValue, double value) {
  set({{bValue, value}});
}

void CylinderVolumeBounds::set(
    std::initializer_list<std::pair<BoundValues, double>> keyValues) {
  std::array<double, eSize> previous = m_values;
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

CylinderVolumeBounds::CylinderVolumeBounds(const CylinderVolumeBounds& cylbo) =
    default;

}  // namespace Acts
