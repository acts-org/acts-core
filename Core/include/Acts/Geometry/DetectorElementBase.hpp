// This file is part of the Acts project.
//
// Copyright (C) 2016-2018 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <memory>
#include <vector>

/// This is the plugin mechanism to exchange the entire DetectorElementBase
///
/// By defining ACTS_DETECTOR_ELEMENT_BASE_REPLACEMENT pre-compile time the
/// detector element entire detector element can be exchanged with a file
/// provided by the client.
///
/// The API has to be present though
#ifdef ACTS_DETECTOR_ELEMENT_BASE_REPLACEMENT
#include ACTS_DETECTOR_ELEMENT_BASE_REPLACEMENT
#else

namespace Acts {

class Surface;

class DetectorElementBase {
 public:
  /// Construct a detector element. It will have ownership of the surface
  explicit DetectorElementBase(std::shared_ptr<Surface>&& surface) : m_surface(surface) {
    if (m_surface->m_associatedDetElement != nullptr) {
      throw std::runtime_error("Detector Element already associated");
    }

    m_surface->m_associatedDetElement = this;
  }
  virtual ~DetectorElementBase() {
    m_surface->m_associatedDetElement = nullptr;
  }

  /// Return the transform for the Element proxy mechanism
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  virtual const Transform3& transform(const GeometryContext& gctx) const = 0;

  /// Return surface representation - const return pattern
  virtual const Surface& surface() const { return *m_surface; };

  /// Non-const return pattern
  virtual Surface& surface() { return *m_surface; };

  /// Returns the thickness of the module
  /// @return double that indicates the thickness of the module
  virtual double thickness() const = 0;

 private:
  std::shared_ptr<Surface> m_surface;
};

}  // namespace Acts

#endif
