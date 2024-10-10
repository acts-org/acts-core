// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/CurvilinearSurface.hpp"

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Utilities/JacobianHelpers.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <iomanip>
#include <ios>
#include <ostream>

namespace Acts {

bool CurvilinearSurface::isStandardRepresentation() const {
  Vector3 T = m_direction.normalized();
  return std::abs(T.dot(Vector3::UnitZ())) < s_curvilinearProjTolerance;
}

RotationMatrix3 CurvilinearSurface::referenceFrame() const {
  /// the right-handed coordinate system is defined as
  /// T = normal
  /// U = Z x T if T not parallel to Z otherwise U = X x T
  /// V = T x U
  Vector3 T = m_direction.normalized();
  Vector3 U = (isStandardRepresentation() ? Vector3::UnitZ() : Vector3::UnitX())
                  .cross(T)
                  .normalized();
  Vector3 V = T.cross(U);

  RotationMatrix3 rframe;
  rframe << U, V, T;
  return rframe;
}

Transform3 CurvilinearSurface::transform() const {
  Transform3 transform = Transform3(referenceFrame());
  transform.pretranslate(center());
  return transform;
}

BoundToFreeMatrix CurvilinearSurface::boundToFreeJacobian() const {
  // this is copied from the `Surface::boundToFreeJacobian`
  // implementation without bounds check and definite direction

  // Initialize the jacobian from local to global
  BoundToFreeMatrix jacobian = BoundToFreeMatrix::Zero();

  // retrieve the reference frame
  const auto rframe = referenceFrame();

  // the local error components - given by reference frame
  jacobian.topLeftCorner<3, 2>() = rframe.topLeftCorner<3, 2>();
  // the time component
  jacobian(eFreeTime, eBoundTime) = 1;
  // the momentum components
  jacobian.block<3, 2>(eFreeDir0, eBoundPhi) =
      sphericalToFreeDirectionJacobian(m_direction);
  jacobian(eFreeQOverP, eBoundQOverP) = 1;

  return jacobian;
}

FreeToBoundMatrix CurvilinearSurface::freeToBoundJacobian() const {
  // this is copied from the `Surface::freeToBoundJacobian`
  // implementation without bounds check and definite direction

  // Initialize the jacobian from global to local
  FreeToBoundMatrix jacobian = FreeToBoundMatrix::Zero();

  // The measurement frame of the surface
  RotationMatrix3 rframeT = referenceFrame().transpose();

  // Local position component given by the reference frame
  jacobian.block<2, 3>(eBoundLoc0, eFreePos0) = rframeT.block<2, 3>(0, 0);
  // Time component
  jacobian(eBoundTime, eFreeTime) = 1;
  // Directional and momentum elements for reference frame surface
  jacobian.block<2, 3>(eBoundPhi, eFreeDir0) =
      freeToSphericalDirectionJacobian(m_direction);
  jacobian(eBoundQOverP, eFreeQOverP) = 1;

  return jacobian;
}

FreeToPathMatrix CurvilinearSurface::freeToPathDerivative() const {
  FreeToPathMatrix freeToPath = FreeToPathMatrix::Zero();
  freeToPath.segment<3>(eFreePos0) = -1.0 * m_direction;
  return freeToPath;
}

std::ostream& CurvilinearSurface::toStream(std::ostream& sl) const {
  sl << "Curvilinear Surface" << std::endl;
  sl << "     Center position  (x, y, z) = (" << m_position.x() << ", "
     << m_position.y() << ", " << m_position.z() << ")" << std::endl;
  sl << "     Direction  (x, y, z) = (" << m_direction.x() << ", "
     << m_direction.y() << ", " << m_direction.z() << ")" << std::endl;
  return sl;
}

std::string CurvilinearSurface::toString() const {
  std::stringstream ss;
  ss << std::setiosflags(std::ios::fixed);
  ss << std::setprecision(4);
  ss << std::boolalpha;
  toStream(ss);
  return ss.str();
}

std::shared_ptr<PlaneSurface> CurvilinearSurface::planeSurface() const {
  return Surface::makeShared<PlaneSurface>(transform());
}

std::shared_ptr<Surface> CurvilinearSurface::surface() const {
  return planeSurface();
}

std::ostream& operator<<(std::ostream& os, const CurvilinearSurface& surface) {
  return surface.toStream(os);
}

}  // namespace Acts
