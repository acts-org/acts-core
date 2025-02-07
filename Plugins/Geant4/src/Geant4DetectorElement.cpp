// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Geant4/Geant4DetectorElement.hpp"

#include "Acts/Surfaces/Surface.hpp"

#include <utility>

namespace Acts {

Geant4DetectorElement::Geant4DetectorElement(std::shared_ptr<Surface> surface,
                                             const G4VPhysicalVolume& g4physVol,
                                             const Transform3& toGlobal,
                                             double thickness)
    : m_surface(std::move(surface)),
      m_g4physVol(&g4physVol),
      m_toGlobal(toGlobal),
      m_thickness(thickness) {}

const Transform3& Geant4DetectorElement::transform(
    const GeometryContext& /*gctx*/) const {
  return m_toGlobal;
}

const Surface& Geant4DetectorElement::surface() const {
  return *m_surface;
}

Surface& Geant4DetectorElement::surface() {
  return *m_surface;
}

double Geant4DetectorElement::thickness() const {
  return m_thickness;
}

const G4VPhysicalVolume& Geant4DetectorElement::g4PhysicalVolume() const {
  return *m_g4physVol;
}

}  // namespace Acts
