// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Geant4/Geant4DetectorElement.hpp"

#include "Acts/Surfaces/Surface.hpp"

#include <utility>

Acts::Geant4DetectorElement::Geant4DetectorElement(
    std::shared_ptr<Acts::Surface> surface, const G4VPhysicalVolume& g4physVol,
    const Acts::Transform3& toGlobal, Acts::ActsScalar thickness)
    : m_surface(std::move(surface)),
      m_g4physVol(&g4physVol),
      m_toGlobal(toGlobal),
      m_thickness(thickness) {}

const Acts::Transform3& Acts::Geant4DetectorElement::transform(
    const GeometryContext& /*gctx*/) const {
  return m_toGlobal;
}

const Acts::Surface& Acts::Geant4DetectorElement::surface() const {
  return *m_surface;
}

Acts::Surface& Acts::Geant4DetectorElement::surface() {
  return *m_surface;
}

Acts::ActsScalar Acts::Geant4DetectorElement::thickness() const {
  return m_thickness;
}

const G4VPhysicalVolume& Acts::Geant4DetectorElement::g4PhysicalVolume() const {
  return *m_g4physVol;
}
