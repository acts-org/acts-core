// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"

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
  DetectorElementBase() = default;
  virtual ~DetectorElementBase() = default;

  /// Return the transform for the Element proxy mechanism
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  virtual const Transform3& transform(const GeometryContext& gctx) const = 0;

  /// Return surface representation - const return pattern
  virtual const Surface& surface() const = 0;

  /// Non-const return pattern
  virtual Surface& surface() = 0;

  /// Returns the thickness of the module
  /// @return double that indicates the thickness of the module
  virtual double thickness() const = 0;
};

}  // namespace Acts

#endif
