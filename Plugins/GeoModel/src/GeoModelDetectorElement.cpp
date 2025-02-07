// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"

#include "Acts/Surfaces/Surface.hpp"

#include <utility>

#include <GeoModelKernel/GeoFullPhysVol.h>

Acts::GeoModelDetectorElement::GeoModelDetectorElement(
    PVConstLink geoPhysVol, std::shared_ptr<Surface> surface,
    const Transform3& sfTransform, double thickness)
    : m_geoPhysVol(std::move(geoPhysVol)),
      m_surface(std::move(surface)),
      m_surfaceTransform(sfTransform),
      m_thickness(thickness) {}

const Acts::Transform3& Acts::GeoModelDetectorElement::transform(
    const GeometryContext& /*gctx*/) const {
  return m_surfaceTransform;
}

const Acts::Surface& Acts::GeoModelDetectorElement::surface() const {
  return *m_surface;
}

Acts::Surface& Acts::GeoModelDetectorElement::surface() {
  return *m_surface;
}

double Acts::GeoModelDetectorElement::thickness() const {
  return m_thickness;
}

PVConstLink Acts::GeoModelDetectorElement::physicalVolume() const {
  return m_geoPhysVol;
}

const std::string& Acts::GeoModelDetectorElement::logVolName() const {
  return m_geoPhysVol->getLogVol()->getName();
}
