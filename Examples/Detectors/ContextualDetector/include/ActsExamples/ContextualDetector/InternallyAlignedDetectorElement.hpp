// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Plugins/Identification/IdentifiedDetectorElement.hpp"
#include "Acts/Plugins/Identification/Identifier.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "ActsExamples/GenericDetector/GenericDetectorElement.hpp"

#include <iostream>
#include <map>
#include <memory>

namespace ActsExamples {

namespace Contextual {

/// @class InternallyAlignedDetectorElement extends GenericDetectorElement
///
/// This is a lightweight type of detector element,
/// it simply implements the base class.
///
/// The AlignedDetectorElement demonstrates how a GeometryContext
/// can be used if it carries an intervall of validity concept
///
/// The nominal transform is only used to once create the alignment
/// store and then in a contextual call the actual detector element
/// position is taken internal multi component store - the latter
/// has to be filled though from an external source
class InternallyAlignedDetectorElement
    : public Generic::GenericDetectorElement {
 public:
  /// @class ContextType
  /// convention: nested to the Detector element
  struct ContextType {
    /// The current intervall of validity
    unsigned int iov = 0;
  };

  /// Constructor for an alignable surface
  ///
  /// @note see Generic::GenericDetectorElement for documentation
  template <typename... Args>
  InternallyAlignedDetectorElement(Args&&... args)
      : Generic::GenericDetectorElement(std::forward<Args>(args)...) {}

  /// Return local to global transform associated with this identifier
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  ///
  /// @note this is called from the surface().transform(gctx)
  const Acts::Transform3& transform(
      const Acts::GeometryContext& gctx) const final override;

  /// Return the nominal local to global transform
  ///
  /// @note the geometry context will hereby be ignored
  const Acts::Transform3& nominalTransform(
      const Acts::GeometryContext& gctx) const;

  /// Return local to global transform associated with this identifier
  ///
  /// @param alignedTransform is a new transform
  /// @oaram iov is the batch for which it is meant
  void addAlignedTransform(std::unique_ptr<Acts::Transform3> alignedTransform,
                           unsigned int iov);

  /// Return the set of alignment transforms in flight
  const std::map<unsigned int, std::unique_ptr<Acts::Transform3>>&
  alignedTransforms() const;

 private:
  std::map<unsigned int, std::unique_ptr<Acts::Transform3>> m_alignedTransforms;
};

inline const Acts::Transform3& InternallyAlignedDetectorElement::transform(
    const Acts::GeometryContext& gctx) const {
  // Check if a different transform than the nominal exists
  if (not m_alignedTransforms.empty()) {
    // cast into the right context object
    auto alignContext = gctx.get<ContextType>();
    auto aTransform = m_alignedTransforms.find(alignContext.iov);
    if (aTransform != m_alignedTransforms.end()) {
      return (*aTransform->second.get());
    }
  }
  // Return the standard transform if not found
  return nominalTransform(gctx);
}

inline const Acts::Transform3&
InternallyAlignedDetectorElement::nominalTransform(
    const Acts::GeometryContext& gctx) const {
  return GenericDetectorElement::transform(gctx);
}

inline void InternallyAlignedDetectorElement::addAlignedTransform(
    std::unique_ptr<Acts::Transform3> alignedTransform, unsigned int iov) {
  m_alignedTransforms[iov] = std::move(alignedTransform);
}

inline const std::map<unsigned int, std::unique_ptr<Acts::Transform3>>&
InternallyAlignedDetectorElement::alignedTransforms() const {
  return m_alignedTransforms;
}

}  // end of namespace Contextual
}  // end of namespace ActsExamples
