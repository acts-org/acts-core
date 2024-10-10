// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/DetectorElementBase.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Tests/CommonHelpers/LineSurfaceStub.hpp"

namespace Acts {
class PlanarBounds;
class DiscBounds;
class ISurfaceMaterial;
class LineBounds;
}  // namespace Acts

namespace Acts::Test {

/// @class DetectorElementStub
///
/// This is a lightweight type of detector element,
/// it simply implements the base class.
class DetectorElementStub : public DetectorElementBase {
 public:
  DetectorElementStub() : DetectorElementBase() {}

  DetectorElementStub(const Transform3& transform)
      : DetectorElementBase(), m_elementTransform(transform) {}

  /// Constructor for single sided detector element
  /// - bound to a Line Surface
  ///
  /// @param transform places the element in global frame
  /// @param cBounds is the cylindrical bounds
  /// @param thickness is the module thickness
  /// @param material is the (optional) Surface material associated to it
  DetectorElementStub(
      const Transform3& transform,
      std::shared_ptr<const CylinderBounds> cBounds, double thickness,
      std::shared_ptr<const ISurfaceMaterial> material = nullptr)
      : DetectorElementBase(),
        m_elementTransform(transform),
        m_elementThickness(thickness) {
    m_elementSurface =
        Surface::makeShared<CylinderSurface>(std::move(cBounds), *this);
    m_elementSurface->assignSurfaceMaterial(std::move(material));
  }

  /// Constructor for single sided detector element
  /// - bound to a Plane Surface
  ///
  /// @param transform places the element in global frame
  /// @param pBounds is the planar bounds for the planar detector element
  /// @param thickness is the module thickness
  /// @param material is the (optional) Surface material associated to it
  DetectorElementStub(
      const Transform3& transform, std::shared_ptr<const PlanarBounds> pBounds,
      double thickness,
      std::shared_ptr<const ISurfaceMaterial> material = nullptr)
      : DetectorElementBase(),
        m_elementTransform(transform),
        m_elementThickness(thickness) {
    m_elementSurface =
        Surface::makeShared<PlaneSurface>(std::move(pBounds), *this);
    m_elementSurface->assignSurfaceMaterial(std::move(material));
  }

  /// Constructor for single sided detector element
  /// - bound to a Line Surface
  ///
  /// @param transform places the element in global frame
  /// @param dBounds is the line bounds for the line like detector element
  /// @param thickness is the module thickness
  /// @param material is the (optional) Surface material associated to it
  DetectorElementStub(
      const Transform3& transform, std::shared_ptr<const LineBounds> lBounds,
      double thickness,
      std::shared_ptr<const ISurfaceMaterial> material = nullptr)
      : DetectorElementBase(),
        m_elementTransform(transform),
        m_elementThickness(thickness) {
    m_elementSurface =
        Surface::makeShared<LineSurfaceStub>(std::move(lBounds), *this);
    m_elementSurface->assignSurfaceMaterial(std::move(material));
  }

  ///  Destructor
  ~DetectorElementStub() override = default;

  /// Return local to global transform associated with this identifier
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  ///
  /// @note this is called from the surface().transform() in the PROXY mode
  const Transform3& transform(const GeometryContext& gctx) const override;

  /// Return surface associated with this detector element
  const Surface& surface() const override;

  /// Non-const access to surface associated with this detector element
  Surface& surface() override;

  /// The maximal thickness of the detector element wrt normal axis
  double thickness() const override;

 private:
  /// the transform for positioning in 3D space
  Transform3 m_elementTransform;
  /// the surface represented by it
  std::shared_ptr<Surface> m_elementSurface{nullptr};
  /// the element thickness
  double m_elementThickness{0.};
};

inline const Transform3& DetectorElementStub::transform(
    const GeometryContext& /*gctx*/) const {
  return m_elementTransform;
}

inline const Surface& DetectorElementStub::surface() const {
  return *m_elementSurface;
}

inline Surface& DetectorElementStub::surface() {
  return *m_elementSurface;
}

inline double DetectorElementStub::thickness() const {
  return m_elementThickness;
}

}  // namespace Acts::Test
