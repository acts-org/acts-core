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
  struct ContextType {
    /// The current interval of validity
    unsigned int iov = 0;
  };

  // Inherit constructor
  using Generic::GenericDetectorElement::GenericDetectorElement;

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
  void addAlignedTransform(const Acts::Transform3& alignedTransform,
                           unsigned int iov);

  void clearAlignedTransform(unsigned int iov);

 private:
  std::map<unsigned int, Acts::Transform3> m_alignedTransforms;
};

inline const Acts::Transform3& InternallyAlignedDetectorElement::transform(
    const Acts::GeometryContext& gctx) const {
  if (!gctx.hasValue()) {
    // Return the standard transform if geo context is empty
    return nominalTransform(gctx);
  }
  const auto& alignContext = gctx.get<ContextType&>();

  if (m_alignedTransforms.empty()) {
    // no alignments -> nominal alignment
    return nominalTransform(gctx);
  }
  auto aTransform = m_alignedTransforms.find(alignContext.iov);
  assert(aTransform != m_alignedTransforms.end());
  return aTransform->second;
}

inline const Acts::Transform3&
InternallyAlignedDetectorElement::nominalTransform(
    const Acts::GeometryContext& gctx) const {
  return GenericDetectorElement::transform(gctx);
}

inline void InternallyAlignedDetectorElement::addAlignedTransform(
    const Acts::Transform3& alignedTransform, unsigned int iov) {
  m_alignedTransforms[iov] = std::move(alignedTransform);
}

inline void InternallyAlignedDetectorElement::clearAlignedTransform(
    unsigned int iov) {
  m_alignedTransforms.erase(m_alignedTransforms.find(iov));
}

}  // namespace Contextual
}  // end of namespace ActsExamples
