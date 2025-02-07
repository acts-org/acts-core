// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Json/JsonDetectorElement.hpp"

#include "Acts/Plugins/Json/AlgebraJsonConverter.hpp"
#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"

namespace Acts {

JsonDetectorElement::JsonDetectorElement(const nlohmann::json &jSurface,
                                         double thickness)
    : m_thickness(thickness) {
  m_surface = Acts::SurfaceJsonConverter::fromJson(jSurface);
  m_transform = Transform3JsonConverter::fromJson(jSurface["transform"]);
  m_surface->assignDetectorElement(*this);
}

const Surface &JsonDetectorElement::surface() const {
  return *m_surface;
}

Surface &JsonDetectorElement::surface() {
  return *m_surface;
}

const Transform3 &JsonDetectorElement::transform(
    const GeometryContext & /*gctx*/) const {
  return m_transform;
}

double JsonDetectorElement::thickness() const {
  return m_thickness;
}

}  // namespace Acts
