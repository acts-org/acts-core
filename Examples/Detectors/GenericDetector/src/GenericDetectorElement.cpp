// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/GenericDetector/GenericDetectorElement.hpp"

#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"

#include <utility>

namespace ActsExamples {

GenericDetectorElement::GenericDetectorElement(
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

GenericDetectorElement::GenericDetectorElement(
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

const Acts::Transform3& GenericDetectorElement::transform(
    const Acts::GeometryContext& /*gctx*/) const {
  return *m_elementTransform;
}

const Acts::Surface& GenericDetectorElement::surface() const {
  return *m_elementSurface;
}

Acts::Surface& GenericDetectorElement::surface() {
  return *m_elementSurface;
}

double GenericDetectorElement::thickness() const {
  return m_elementThickness;
}

GenericDetectorElement::Identifier GenericDetectorElement::identifier() const {
  return m_elementIdentifier;
}

}  // namespace ActsExamples
