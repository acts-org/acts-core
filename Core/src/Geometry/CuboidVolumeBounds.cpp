// This file is part of the Acts project.
//
// Copyright (C) 2016-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BoundingBox.hpp"

#include <cmath>
#include <iostream>

Acts::CuboidVolumeBounds::CuboidVolumeBounds(double halex, double haley,
                                             double halez)
    : VolumeBounds(),
      m_values({halex, haley, halez}),
      m_xyBounds(std::make_shared<const RectangleBounds>(halex, haley)),
      m_yzBounds(std::make_shared<const RectangleBounds>(haley, halez)),
      m_zxBounds(std::make_shared<const RectangleBounds>(halez, halex)) {
  checkConsistency();
}

Acts::CuboidVolumeBounds::CuboidVolumeBounds(const CuboidVolumeBounds& bobo)
    : VolumeBounds(),
      m_values(bobo.m_values),
      m_xyBounds(bobo.m_xyBounds),
      m_yzBounds(bobo.m_yzBounds),
      m_zxBounds(bobo.m_zxBounds) {}

Acts::CuboidVolumeBounds& Acts::CuboidVolumeBounds::operator=(
    const CuboidVolumeBounds& bobo) {
  if (this != &bobo) {
    m_values = bobo.m_values;
    m_xyBounds = bobo.m_xyBounds;
    m_yzBounds = bobo.m_yzBounds;
    m_zxBounds = bobo.m_zxBounds;
  }
  return *this;
}

Acts::SurfacePtrVector Acts::CuboidVolumeBounds::decomposeToSurfaces(
    const Transform3D* transformPtr) const {
  // The transform - apply when given
  Transform3D transform =
      (transformPtr == nullptr) ? Transform3D::Identity() : (*transformPtr);

  SurfacePtrVector rSurfaces;
  rSurfaces.reserve(6);
  // Face surfaces xy -------------------------------------
  //   (1) - at negative local z
  rSurfaces.push_back(Surface::makeShared<PlaneSurface>(
      std::make_shared<const Transform3D>(
          transform * Translation3D(0., 0., -get(eHalfLengthZ))),
      m_xyBounds));

  //   (2) - at positive local z
  rSurfaces.push_back(Surface::makeShared<PlaneSurface>(
      std::make_shared<const Transform3D>(
          transform * Translation3D(0., 0., get(eHalfLengthZ))),
      m_xyBounds));
  // Face surfaces yz -------------------------------------
  //   (3) - at negative local x
  rSurfaces.push_back(Surface::makeShared<PlaneSurface>(
      std::make_shared<const Transform3D>(
          transform * Translation3D(-get(eHalfLengthX), 0., 0.) * s_planeYZ),
      m_yzBounds));
  //   (4) - at positive local x
  rSurfaces.push_back(Surface::makeShared<PlaneSurface>(
      std::make_shared<const Transform3D>(
          transform * Translation3D(get(eHalfLengthX), 0., 0.) * s_planeYZ),
      m_yzBounds));
  // Face surfaces zx -------------------------------------
  //   (5) - at negative local y
  rSurfaces.push_back(Surface::makeShared<PlaneSurface>(
      std::make_shared<const Transform3D>(
          transform * Translation3D(0., -get(eHalfLengthY), 0.) * s_planeZX),
      m_zxBounds));
  //   (6) - at positive local y
  rSurfaces.push_back(Surface::makeShared<PlaneSurface>(
      std::make_shared<const Transform3D>(
          transform * Translation3D(0., get(eHalfLengthY), 0.) * s_planeZX),
      m_zxBounds));

  return rSurfaces;
}

std::ostream& Acts::CuboidVolumeBounds::toStream(std::ostream& sl) const {
  return dumpT(sl);
}

Acts::Volume::BoundingBox Acts::CuboidVolumeBounds::boundingBox(
    const Acts::Transform3D* trf, const Vector3D& envelope,
    const Volume* entity) const {
  Vector3D vmin(-get(eHalfLengthX), -get(eHalfLengthY), -get(eHalfLengthZ));
  Vector3D vmax(get(eHalfLengthX), get(eHalfLengthY), get(eHalfLengthZ));

  Volume::BoundingBox box(entity, vmin - envelope, vmax + envelope);
  return trf == nullptr ? box : box.transformed(*trf);
}
