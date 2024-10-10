// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/GenericDetector/GenericDetectorElement.hpp"

#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"

#include <utility>

ActsExamples::Generic::GenericDetectorElement::GenericDetectorElement(
    const Identifier identifier,
    std::shared_ptr<const Acts::Transform3> transform,
    std::shared_ptr<const Acts::PlanarBounds> pBounds, double thickness,
    std::shared_ptr<const Acts::ISurfaceMaterial> material)
    : Acts::DetectorElementBase(),
      m_elementIdentifier(identifier),
      m_elementTransform(std::move(transform)),
      m_elementSurface(
          Acts::Surface::makeShared<Acts::PlaneSurface>(pBounds, *this)),
      m_elementThickness(thickness),
      m_elementPlanarBounds(std::move(pBounds)),
      m_elementDiscBounds(nullptr) {
  m_elementSurface->assignSurfaceMaterial(std::move(material));
}

ActsExamples::Generic::GenericDetectorElement::GenericDetectorElement(
    const Identifier identifier,
    std::shared_ptr<const Acts::Transform3> transform,
    std::shared_ptr<const Acts::DiscBounds> dBounds, double thickness,
    std::shared_ptr<const Acts::ISurfaceMaterial> material)
    : Acts::DetectorElementBase(),
      m_elementIdentifier(identifier),
      m_elementTransform(std::move(transform)),
      m_elementSurface(
          Acts::Surface::makeShared<Acts::DiscSurface>(dBounds, *this)),
      m_elementThickness(thickness),
      m_elementPlanarBounds(nullptr),
      m_elementDiscBounds(std::move(dBounds)) {
  m_elementSurface->assignSurfaceMaterial(std::move(material));
}
