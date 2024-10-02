// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/TGeo/TGeoDetectorElement.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/TGeo/TGeoSurfaceConverter.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <tuple>
#include <utility>

#include <boost/algorithm/string.hpp>

#include "RtypesCore.h"
#include "TGeoArb8.h"
#include "TGeoBBox.h"
#include "TGeoBoolNode.h"
#include "TGeoCompositeShape.h"
#include "TGeoTrd2.h"
#include "TGeoTube.h"

using Line2D = Eigen::Hyperplane<double, 2>;

Acts::TGeoDetectorElement::TGeoDetectorElement(
    const Identifier& identifier, const TGeoNode& tGeoNode,
    const TGeoMatrix& tGeoMatrix, const std::string& axes, double scalor,
    std::shared_ptr<const Acts::ISurfaceMaterial> material)
    : Acts::DetectorElementBase(),
      m_detElement(&tGeoNode),
      m_identifier(identifier) {
  // Create temporary local non const surface (to allow setting the
  // material)
  const Double_t* translation = tGeoMatrix.GetTranslation();
  const Double_t* rotation = tGeoMatrix.GetRotationMatrix();

  auto sensor = m_detElement->GetVolume();
  auto tgShape = sensor->GetShape();

  auto [cBounds, cTransform, cThickness] =
      TGeoSurfaceConverter::cylinderComponents(*tgShape, rotation, translation,
                                               axes, scalor);
  if (cBounds != nullptr) {
    m_transform = cTransform;
    m_bounds = cBounds;
    m_thickness = cThickness;
    m_surface = Surface::makeShared<CylinderSurface>(cBounds, *this);
  }

  // Check next if you do not have a surface
  if (m_surface == nullptr) {
    auto [dBounds, dTransform, dThickness] =
        TGeoSurfaceConverter::discComponents(*tgShape, rotation, translation,
                                             axes, scalor);
    if (dBounds != nullptr) {
      m_bounds = dBounds;
      m_transform = dTransform;
      m_thickness = dThickness;
      m_surface = Surface::makeShared<DiscSurface>(dBounds, *this);
    }
  }

  // Check next if you do not have a surface
  if (m_surface == nullptr) {
    auto [pBounds, pTransform, pThickness] =
        TGeoSurfaceConverter::planeComponents(*tgShape, rotation, translation,
                                              axes, scalor);
    if (pBounds != nullptr) {
      m_bounds = pBounds;
      m_transform = pTransform;
      m_thickness = pThickness;
      m_surface = Surface::makeShared<PlaneSurface>(pBounds, *this);
    }
  }

  // set the asscoiated material (non const method)
  if (m_surface != nullptr) {
    m_surface->assignSurfaceMaterial(std::move(material));
  }
}

Acts::TGeoDetectorElement::TGeoDetectorElement(
    const Identifier& identifier, const TGeoNode& tGeoNode,
    const Transform3& tgTransform,
    const std::shared_ptr<const PlanarBounds>& tgBounds, double tgThickness)
    : Acts::DetectorElementBase(),
      m_detElement(&tGeoNode),
      m_transform(tgTransform),
      m_identifier(identifier),
      m_bounds(tgBounds),
      m_thickness(tgThickness) {
  m_surface = Surface::makeShared<PlaneSurface>(tgBounds, *this);
}

Acts::TGeoDetectorElement::TGeoDetectorElement(
    const Identifier& identifier, const TGeoNode& tGeoNode,
    const Transform3& tgTransform,
    const std::shared_ptr<const DiscBounds>& tgBounds, double tgThickness)
    : Acts::DetectorElementBase(),
      m_detElement(&tGeoNode),
      m_transform(tgTransform),
      m_identifier(identifier),
      m_bounds(tgBounds),
      m_thickness(tgThickness) {
  m_surface = Surface::makeShared<DiscSurface>(tgBounds, *this);
}

Acts::TGeoDetectorElement::~TGeoDetectorElement() = default;
