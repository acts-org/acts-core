// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/DetectorElementBase.hpp"

#include <nlohmann/json.hpp>

namespace Acts {

/// A implementation of a detector element, that is constructed from a
/// JSON description of a surface. The idea behind this is that it helps
/// importing whole tracking geometries from JSON files. In some parts of
/// the codebase, the existence of a detector element associated to a surface
/// has a specific meaning (e.g., flags surfaces as sensitive).
class JsonDetectorElement : public DetectorElementBase {
 public:
  JsonDetectorElement(const nlohmann::json &jSurface, double thickness);

  Surface &surface() override;
  const Surface &surface() const override;

  double thickness() const override;

  const Transform3 &transform(const GeometryContext &gctx) const override;

 private:
  std::shared_ptr<Surface> m_surface;
  Transform3 m_transform{};
  double m_thickness{};
};

}  // namespace Acts
