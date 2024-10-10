// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/AnnulusBounds.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/VerticesHelper.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

Acts::AnnulusBounds::AnnulusBounds(
    const std::array<double, eSize>& values) noexcept(false)
    : m_values(values), m_moduleOrigin({values[eOriginX], values[eOriginY]}) {
  checkConsistency();
  m_rotationStripPC = Translation2(Vector2(0, -get(eAveragePhi)));
  m_translation = Translation2(m_moduleOrigin);

  m_shiftXY = m_moduleOrigin * -1;
  m_shiftPC =
      Vector2(VectorHelpers::perp(m_shiftXY), VectorHelpers::phi(m_shiftXY));

  // we need the corner points of the module to do the inside
  // checking, calculate them here once, they don't change

  // find inner outer radius at edges in STRIP PC
  auto circIx = [](double O_x, double O_y, double r, double phi) -> Vector2 {
    //                      _____________________________________________
    //                     /      2  2                    2    2  2    2
    //     O_x + O_y*m - \/  - O_x *m  + 2*O_x*O_y*m - O_y  + m *r  + r
    // x = --------------------------------------------------------------
    //                                  2
    //                                 m  + 1
    //
    // y = m*x
    //
    double m = std::tan(phi);
    Vector2 dir(std::cos(phi), std::sin(phi));
    double x1 = (O_x + O_y * m -
                 std::sqrt(-std::pow(O_x, 2) * std::pow(m, 2) +
                           2 * O_x * O_y * m - std::pow(O_y, 2) +
                           std::pow(m, 2) * std::pow(r, 2) + std::pow(r, 2))) /
                (std::pow(m, 2) + 1);
    double x2 = (O_x + O_y * m +
                 std::sqrt(-std::pow(O_x, 2) * std::pow(m, 2) +
                           2 * O_x * O_y * m - std::pow(O_y, 2) +
                           std::pow(m, 2) * std::pow(r, 2) + std::pow(r, 2))) /
                (std::pow(m, 2) + 1);

    Vector2 v1(x1, m * x1);
    if (v1.dot(dir) > 0) {
      return v1;
    }
    return {x2, m * x2};
  };

  // calculate corners in STRIP XY, keep them we need them for minDistance()
  m_outLeftStripXY =
      circIx(m_moduleOrigin[eBoundLoc0], m_moduleOrigin[eBoundLoc1], get(eMaxR),
             get(eMaxPhiRel));
  m_inLeftStripXY =
      circIx(m_moduleOrigin[eBoundLoc0], m_moduleOrigin[eBoundLoc1], get(eMinR),
             get(eMaxPhiRel));
  m_outRightStripXY =
      circIx(m_moduleOrigin[eBoundLoc0], m_moduleOrigin[eBoundLoc1], get(eMaxR),
             get(eMinPhiRel));
  m_inRightStripXY =
      circIx(m_moduleOrigin[eBoundLoc0], m_moduleOrigin[eBoundLoc1], get(eMinR),
             get(eMinPhiRel));

  m_outLeftStripPC = {m_outLeftStripXY.norm(),
                      VectorHelpers::phi(m_outLeftStripXY)};
  m_inLeftStripPC = {m_inLeftStripXY.norm(),
                     VectorHelpers::phi(m_inLeftStripXY)};
  m_outRightStripPC = {m_outRightStripXY.norm(),
                       VectorHelpers::phi(m_outRightStripXY)};
  m_inRightStripPC = {m_inRightStripXY.norm(),
                      VectorHelpers::phi(m_inRightStripXY)};

  m_outLeftModulePC = stripXYToModulePC(m_outLeftStripXY);
  m_inLeftModulePC = stripXYToModulePC(m_inLeftStripXY);
  m_outRightModulePC = stripXYToModulePC(m_outRightStripXY);
  m_inRightModulePC = stripXYToModulePC(m_inRightStripXY);
}

std::vector<Acts::Vector2> Acts::AnnulusBounds::corners() const {
  auto rot = m_rotationStripPC.inverse();

  return {rot * m_outRightStripPC, rot * m_outLeftStripPC,
          rot * m_inLeftStripPC, rot * m_inRightStripPC};
}

std::vector<Acts::Vector2> Acts::AnnulusBounds::vertices(
    unsigned int quarterSegments) const {
  if (quarterSegments > 0u) {
    using VectorHelpers::phi;

    ActsScalar phiMinInner = phi(m_inRightStripXY - m_moduleOrigin);
    ActsScalar phiMaxInner = phi(m_inLeftStripXY - m_moduleOrigin);

    ActsScalar phiMinOuter = phi(m_outRightStripXY - m_moduleOrigin);
    ActsScalar phiMaxOuter = phi(m_outLeftStripXY - m_moduleOrigin);

    // Inner bow from phi_min -> phi_max (needs to be reversed)
    std::vector<Acts::Vector2> rvertices =
        detail::VerticesHelper::segmentVertices<Vector2, Transform2>(
            {get(eMinR), get(eMinR)}, phiMinInner, phiMaxInner, {},
            quarterSegments);
    std::reverse(rvertices.begin(), rvertices.end());

    // Outer bow from phi_min -> phi_max
    auto overtices =
        detail::VerticesHelper::segmentVertices<Vector2, Transform2>(
            {get(eMaxR), get(eMaxR)}, phiMinOuter, phiMaxOuter, {},
            quarterSegments);
    rvertices.insert(rvertices.end(), overtices.begin(), overtices.end());

    std::for_each(rvertices.begin(), rvertices.end(),
                  [&](Acts::Vector2& rv) { rv += m_moduleOrigin; });
    return rvertices;
  }
  return {m_inLeftStripXY, m_inRightStripXY, m_outRightStripXY,
          m_outLeftStripXY};
}

bool Acts::AnnulusBounds::inside(const Vector2& lposition, double tolR,
                                 double tolPhi) const {
  // locpo is PC in STRIP SYSTEM
  // need to perform internal rotation induced by average phi
  Vector2 locpo_rotated = m_rotationStripPC * lposition;
  double phiLoc = locpo_rotated[eBoundLoc1];
  double rLoc = locpo_rotated[eBoundLoc0];

  if (phiLoc < (get(eMinPhiRel) - tolPhi) ||
      phiLoc > (get(eMaxPhiRel) + tolPhi)) {
    return false;
  }

  // calculate R in MODULE SYSTEM to evaluate R-bounds
  if (tolR == 0.) {
    // don't need R, can use R^2
    double r_mod2 =
        m_shiftPC[eBoundLoc0] * m_shiftPC[eBoundLoc0] + rLoc * rLoc +
        2 * m_shiftPC[eBoundLoc0] * rLoc * cos(phiLoc - m_shiftPC[eBoundLoc1]);

    if (r_mod2 < get(eMinR) * get(eMinR) || r_mod2 > get(eMaxR) * get(eMaxR)) {
      return false;
    }
  } else {
    // use R
    double r_mod = sqrt(
        m_shiftPC[eBoundLoc0] * m_shiftPC[eBoundLoc0] + rLoc * rLoc +
        2 * m_shiftPC[eBoundLoc0] * rLoc * cos(phiLoc - m_shiftPC[eBoundLoc1]));

    if (r_mod < (get(eMinR) - tolR) || r_mod > (get(eMaxR) + tolR)) {
      return false;
    }
  }
  return true;
}

bool Acts::AnnulusBounds::inside(
    const Vector2& lposition,
    const BoundaryTolerance& boundaryTolerance) const {
  if (boundaryTolerance.isInfinite()) {
    return true;
  }

  if (boundaryTolerance.isNone()) {
    return inside(lposition, 0., 0.);
  }

  if (auto absoluteBound = boundaryTolerance.asAbsoluteBoundOpt();
      absoluteBound.has_value()) {
    return inside(lposition, absoluteBound->tolerance0,
                  absoluteBound->tolerance1);
  }

  // first check if inside. We don't need to look into the covariance if inside
  if (inside(lposition, 0., 0.)) {
    return true;
  }

  if (!boundaryTolerance.hasChi2Bound()) {
    throw std::logic_error("not implemented");
  }

  const auto& boundaryToleranceChi2 = boundaryTolerance.asChi2Bound();

  // locpo is PC in STRIP SYSTEM
  // we need to rotate the locpo
  Vector2 locpo_rotated = m_rotationStripPC * lposition;

  // covariance is given in STRIP SYSTEM in PC we need to convert the covariance
  // to the MODULE SYSTEM in PC via jacobian. The following transforms into
  // STRIP XY, does the shift into MODULE XY, and then transforms into MODULE PC
  double dphi = get(eAveragePhi);
  double phi_strip = locpo_rotated[eBoundLoc1];
  double r_strip = locpo_rotated[eBoundLoc0];
  double O_x = m_shiftXY[eBoundLoc0];
  double O_y = m_shiftXY[eBoundLoc1];

  // For a transformation from cartesian into polar coordinates
  //
  //              [         _________      ]
  //              [        /  2    2       ]
  //              [      \/  x  + y        ]
  //     [ r' ]   [                        ]
  // v = [    ] = [      /       y        \]
  //     [phi']   [2*atan|----------------|]
  //              [      |       _________|]
  //              [      |      /  2    2 |]
  //              [      \x + \/  x  + y  /]
  //
  // Where x, y are polar coordinates that can be rotated by dPhi
  //
  // [x]   [O_x + r*cos(dPhi - phi)]
  // [ ] = [                       ]
  // [y]   [O_y - r*sin(dPhi - phi)]
  //
  // The general jacobian is:
  //
  //        [d        d      ]
  //        [--(f_x)  --(f_x)]
  //        [dx       dy     ]
  // Jgen = [                ]
  //        [d        d      ]
  //        [--(f_y)  --(f_y)]
  //        [dx       dy     ]
  //
  // which means in this case:
  //
  //     [     d                   d           ]
  //     [ ----------(rMod)    ---------(rMod) ]
  //     [ dr_{strip}          dphiStrip       ]
  // J = [                                     ]
  //     [    d                   d            ]
  //     [----------(phiMod)  ---------(phiMod)]
  //     [dr_{strip}          dphiStrip        ]
  //
  // Performing the derivative one gets:
  //
  //     [B*O_x + C*O_y + rStrip  rStrip*(B*O_y + O_x*sin(dPhi - phiStrip))]
  //     [----------------------  -----------------------------------------]
  //     [          ___                               ___                  ]
  //     [        \/ A                              \/ A                   ]
  // J = [                                                                 ]
  //     [  -(B*O_y - C*O_x)           rStrip*(B*O_x + C*O_y + rStrip)     ]
  //     [  -----------------          -------------------------------     ]
  //     [          A                                 A                    ]
  //
  // where
  //        2                                          2
  // A = O_x  + 2*O_x*rStrip*cos(dPhi - phiStrip) + O_y
  //                                                 2
  //     - 2*O_y*rStrip*sin(dPhi - phiStrip) + rStrip
  // B = cos(dPhi - phiStrip)
  // C = -sin(dPhi - phiStrip)

  double cosDPhiPhiStrip = std::cos(dphi - phi_strip);
  double sinDPhiPhiStrip = std::sin(dphi - phi_strip);

  double A = O_x * O_x + 2 * O_x * r_strip * cosDPhiPhiStrip + O_y * O_y -
             2 * O_y * r_strip * sinDPhiPhiStrip + r_strip * r_strip;
  double sqrtA = std::sqrt(A);

  double B = cosDPhiPhiStrip;
  double C = -sinDPhiPhiStrip;
  ActsMatrix<2, 2> jacobianStripPCToModulePC;
  jacobianStripPCToModulePC(0, 0) = (B * O_x + C * O_y + r_strip) / sqrtA;
  jacobianStripPCToModulePC(0, 1) =
      r_strip * (B * O_y + O_x * sinDPhiPhiStrip) / sqrtA;
  jacobianStripPCToModulePC(1, 0) = -(B * O_y - C * O_x) / A;
  jacobianStripPCToModulePC(1, 1) = r_strip * (B * O_x + C * O_y + r_strip) / A;

  // Mahalanobis distance uses inverse covariance as weights
  const auto& weightStripPC = boundaryToleranceChi2.weight;
  auto weightModulePC = jacobianStripPCToModulePC.transpose() * weightStripPC *
                        jacobianStripPCToModulePC;

  double minDist = std::numeric_limits<double>::max();

  Vector2 currentClosest;
  double currentDist = 0;

  // do projection in STRIP PC

  // first: STRIP system. locpo is in STRIP PC already
  currentClosest = closestOnSegment(m_inLeftStripPC, m_outLeftStripPC,
                                    locpo_rotated, weightStripPC);
  currentDist = squaredNorm(locpo_rotated - currentClosest, weightStripPC);
  minDist = currentDist;

  currentClosest = closestOnSegment(m_inRightStripPC, m_outRightStripPC,
                                    locpo_rotated, weightStripPC);
  currentDist = squaredNorm(locpo_rotated - currentClosest, weightStripPC);
  if (currentDist < minDist) {
    minDist = currentDist;
  }

  // now: MODULE system. Need to transform locpo to MODULE PC
  //  transform is STRIP PC -> STRIP XY -> MODULE XY -> MODULE PC
  Vector2 locpoStripXY(
      locpo_rotated[eBoundLoc0] * std::cos(locpo_rotated[eBoundLoc1]),
      locpo_rotated[eBoundLoc0] * std::sin(locpo_rotated[eBoundLoc1]));
  Vector2 locpoModulePC = stripXYToModulePC(locpoStripXY);

  // now check edges in MODULE PC (inner and outer circle) assuming Mahalanobis
  // distances are of same unit if covariance is correctly transformed
  currentClosest = closestOnSegment(m_inLeftModulePC, m_inRightModulePC,
                                    locpoModulePC, weightModulePC);
  currentDist = squaredNorm(locpoModulePC - currentClosest, weightModulePC);
  if (currentDist < minDist) {
    minDist = currentDist;
  }

  currentClosest = closestOnSegment(m_outLeftModulePC, m_outRightModulePC,
                                    locpoModulePC, weightModulePC);
  currentDist = squaredNorm(locpoModulePC - currentClosest, weightModulePC);
  if (currentDist < minDist) {
    minDist = currentDist;
  }

  // compare resulting Mahalanobis distance to configured "number of sigmas" we
  // square it b/c we never took the square root of the distance
  return minDist <
         boundaryToleranceChi2.maxChi2 * boundaryToleranceChi2.maxChi2;
}

Acts::Vector2 Acts::AnnulusBounds::stripXYToModulePC(
    const Vector2& vStripXY) const {
  Vector2 vecModuleXY = vStripXY + m_shiftXY;
  return {vecModuleXY.norm(), VectorHelpers::phi(vecModuleXY)};
}

Acts::Vector2 Acts::AnnulusBounds::closestOnSegment(
    const Vector2& a, const Vector2& b, const Vector2& p,
    const SquareMatrix2& weight) const {
  // connecting vector
  auto n = b - a;
  // squared norm of line
  auto f = (n.transpose() * weight * n).value();
  // weighted scalar product of line to point and segment line
  auto u = ((p - a).transpose() * weight * n).value() / f;
  // clamp to [0, 1], convert to point
  return std::min(std::max(u, static_cast<ActsScalar>(0)),
                  static_cast<ActsScalar>(1)) *
             n +
         a;
}

double Acts::AnnulusBounds::squaredNorm(const Vector2& v,
                                        const SquareMatrix2& weight) const {
  return (v.transpose() * weight * v).value();
}

Acts::Vector2 Acts::AnnulusBounds::moduleOrigin() const {
  return Eigen::Rotation2D<ActsScalar>(get(eAveragePhi)) * m_moduleOrigin;
}

// Ostream operator overload
std::ostream& Acts::AnnulusBounds::toStream(std::ostream& sl) const {
  sl << std::setiosflags(std::ios::fixed);
  sl << std::setprecision(7);
  sl << "Acts::AnnulusBounds:  (innerRadius, outerRadius, minPhi, maxPhi) = ";
  sl << "(" << get(eMinR) << ", " << get(eMaxR) << ", " << phiMin() << ", "
     << phiMax() << ")" << '\n';
  sl << " - shift xy = " << m_shiftXY.x() << ", " << m_shiftXY.y() << '\n';
  sl << " - shift pc = " << m_shiftPC.x() << ", " << m_shiftPC.y() << '\n';
  sl << std::setprecision(-1);
  return sl;
}
