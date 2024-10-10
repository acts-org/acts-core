// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "ActsExamples/GenericDetector/GenericDetectorElement.hpp"

#include <map>

namespace ActsExamples::Contextual {

/// @class ExternallyAlignedDetectorElement extends GenericDetectorElement
///
/// This is a lightweight type of detector element,
/// it simply implements the base class.
///
/// The ExternallyAlignedDetectorElement demonstrates how a GeometryContext
/// can be used if it carries the entire set of Transforms through
/// the program flow.
///
/// The nominal transform is only used to once create the alignment
/// store and then in a contextual call the actual detector element
/// position is taken from the alignment Store.
///
/// In this simple implementation, it does rely on the Identifier
/// to be orderded from 0 to N-1, as the identifier is simply taken
/// as a vector index for the alignment store
class ExternallyAlignedDetectorElement
    : public Generic::GenericDetectorElement {
 public:
  struct AlignmentStore {
    // GenericDetector identifiers are sequential
    std::vector<Acts::Transform3> transforms;
    std::size_t lastAccessed = 0;
  };

  /// @class ContextType
  /// convention: nested to the Detector element
  struct ContextType {
    // GenericDetector identifiers are an integer sequence, so vector is fine!
    std::shared_ptr<const AlignmentStore> alignmentStore{nullptr};
  };

  using Generic::GenericDetectorElement::GenericDetectorElement;

  /// Return local to global transform associated with this identifier
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  ///
  /// @note this is called from the surface().transform(gctx)
  const Acts::Transform3& transform(
      const Acts::GeometryContext& gctx) const override;
};

inline const Acts::Transform3& ExternallyAlignedDetectorElement::transform(
    const Acts::GeometryContext& gctx) const {
  if (!gctx.hasValue()) {  // Treating empty context => nominal alignment
    return GenericDetectorElement::transform(gctx);
  }
  // cast into the right context object
  const auto& alignContext = gctx.get<ContextType>();
  identifier_type idValue = static_cast<identifier_type>(identifier());

  if (alignContext.alignmentStore == nullptr) {
    // geometry construction => nominal alignment
    return GenericDetectorElement::transform(gctx);
  }

  // At this point, the alignment store should be populated
  assert(idValue < alignContext.alignmentStore->transforms.size());
  return alignContext.alignmentStore->transforms[idValue];
}

}  // namespace ActsExamples::Contextual
