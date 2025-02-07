// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/ConeLayer.hpp"

#include "Acts/Definitions/Algebra.hpp"

namespace Acts {
class ConeBounds;
}  // namespace Acts

Acts::ConeLayer::ConeLayer(const Transform3& transform,
                           std::shared_ptr<const ConeBounds> cbounds,
                           std::unique_ptr<SurfaceArray> surfaceArray,
                           double thickness,
                           std::unique_ptr<ApproachDescriptor> ade,
                           LayerType laytyp)
    : ConeSurface(transform, std::move(cbounds)),
      Layer(std::move(surfaceArray), thickness, std::move(ade), laytyp) {}

const Acts::ConeSurface& Acts::ConeLayer::surfaceRepresentation() const {
  return (*this);
}

Acts::ConeSurface& Acts::ConeLayer::surfaceRepresentation() {
  return (*this);
}
