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
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace Acts {
class Surface;
class PlanarBounds;
class DiscBounds;
class ISurfaceMaterial;
}  // namespace Acts

namespace ActsExamples::Telescope {

/// @class TelescopeDetectorElement
///
/// This is a lightweight type of detector element,
/// it simply implements the base class.
class TelescopeDetectorElement : public Acts::DetectorElementBase {
 public:
  /// @class ContextType
  /// convention: nested to the Detector element
  struct ContextType {
    /// The current interval of validity
    unsigned int iov = 0;
  };

  /// Constructor for single sided detector element
  /// - bound to a Plane Surface
  ///
  /// @param transform is the transform that element the layer in 3D frame
  /// @param pBounds is the planar bounds for the planar detector element
  /// @param thickness is the module thickness
  /// @param material is the (optional) Surface material associated to it
  TelescopeDetectorElement(
      std::shared_ptr<const Acts::Transform3> transform,
      std::shared_ptr<const Acts::PlanarBounds> pBounds, double thickness,
      std::shared_ptr<const Acts::ISurfaceMaterial> material = nullptr);

  /// Constructor for single sided detector element
  /// - bound to a Disc Surface
  ///
  /// @param transform is the transform that element the layer in 3D frame
  /// @param dBounds is the planar bounds for the disc like detector element
  /// @param thickness is the module thickness
  /// @param material is the (optional) Surface material associated to it
  TelescopeDetectorElement(
      std::shared_ptr<const Acts::Transform3> transform,
      std::shared_ptr<const Acts::DiscBounds> dBounds, double thickness,
      std::shared_ptr<const Acts::ISurfaceMaterial> material = nullptr);

  ///  Destructor
  ~TelescopeDetectorElement() override = default;

  /// Return surface associated with this detector element
  const Acts::Surface& surface() const final;

  /// Non-const access to the surface associated with this detector element
  Acts::Surface& surface() final;

  /// The maximal thickness of the detector element wrt normal axis
  double thickness() const final;

  /// Return local to global transform associated with this identifier
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  ///
  /// @note this is called from the surface().transform() in the PROXY mode
  const Acts::Transform3& transform(
      const Acts::GeometryContext& gctx) const final;

  /// Return the nominal local to global transform
  ///
  /// @note the geometry context will hereby be ignored
  const Acts::Transform3& nominalTransform(
      const Acts::GeometryContext& gctx) const;

  /// Return local to global transform associated with this identifier
  ///
  /// @param alignedTransform is a new transform
  /// @param iov is the batch for which it is meant
  void addAlignedTransform(std::unique_ptr<Acts::Transform3> alignedTransform,
                           unsigned int iov);

  /// Return the set of alignment transforms in flight
  const std::vector<std::unique_ptr<Acts::Transform3>>& alignedTransforms()
      const;

 private:
  /// the transform for positioning in 3D space
  std::shared_ptr<const Acts::Transform3> m_elementTransform = nullptr;
  // the aligned transforms
  std::vector<std::unique_ptr<Acts::Transform3>> m_alignedTransforms = {};
  /// the surface represented by it
  std::shared_ptr<Acts::Surface> m_elementSurface = nullptr;
  /// the element thickness
  double m_elementThickness = 0.;
  /// the planar bounds
  std::shared_ptr<const Acts::PlanarBounds> m_elementPlanarBounds = nullptr;
  /// the disc bounds
  std::shared_ptr<const Acts::DiscBounds> m_elementDiscBounds = nullptr;
};

inline const Acts::Surface& TelescopeDetectorElement::surface() const {
  return *m_elementSurface;
}

inline Acts::Surface& TelescopeDetectorElement::surface() {
  return *m_elementSurface;
}

inline double TelescopeDetectorElement::thickness() const {
  return m_elementThickness;
}

inline const Acts::Transform3& TelescopeDetectorElement::transform(
    const Acts::GeometryContext& gctx) const {
  // Check if a different transform than the nominal exists
  if (!m_alignedTransforms.empty()) {
    // cast into the right context object
    auto alignContext = gctx.get<ContextType>();
    return (*m_alignedTransforms[alignContext.iov].get());
  }
  // Return the standard transform if not found
  return nominalTransform(gctx);
}

inline const Acts::Transform3& TelescopeDetectorElement::nominalTransform(
    const Acts::GeometryContext& /*gctx*/) const {
  return *m_elementTransform;
}

inline void TelescopeDetectorElement::addAlignedTransform(
    std::unique_ptr<Acts::Transform3> alignedTransform, unsigned int iov) {
  // most standard case, it's just a new one:
  auto cios = m_alignedTransforms.size();
  for (unsigned int ic = cios; ic <= iov; ++ic) {
    m_alignedTransforms.push_back(nullptr);
  }
  m_alignedTransforms[iov] = std::move(alignedTransform);
}

inline const std::vector<std::unique_ptr<Acts::Transform3>>&
TelescopeDetectorElement::alignedTransforms() const {
  return m_alignedTransforms;
}

}  // namespace ActsExamples::Telescope
