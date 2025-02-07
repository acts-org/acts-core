// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "ActsExamples/GenericDetector/GenericDetectorElement.hpp"

#include <mutex>
#include <unordered_map>

namespace ActsExamples {

/// @class InternallyAlignedDetectorElement extends GenericDetectorElement
///
/// This is a lightweight type of detector element,
/// it simply implements the base class.
///
/// The AlignedDetectorElement demonstrates how a GeometryContext
/// can be used if it carries an interval of validity concept
///
/// The nominal transform is only used to once create the alignment
/// store and then in a contextual call the actual detector element
/// position is taken internal multi component store - the latter
/// has to be filled though from an external source
class InternallyAlignedDetectorElement : public GenericDetectorElement {
 public:
  struct ContextType {
    /// The current interval of validity
    unsigned int iov = 0;
    bool nominal = false;
  };

  // Inherit constructor
  using GenericDetectorElement::GenericDetectorElement;

  /// Return local to global transform associated with this identifier
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  ///
  /// @note this is called from the surface().transform(gctx)
  const Acts::Transform3& transform(
      const Acts::GeometryContext& gctx) const override;

  /// Return the nominal local to global transform
  ///
  /// @note the geometry context will hereby be ignored
  const Acts::Transform3& nominalTransform(
      const Acts::GeometryContext& gctx) const;

  /// Return local to global transform associated with this identifier
  ///
  /// @param alignedTransform is a new transform
  /// @param iov is the batch for which it is meant
  void addAlignedTransform(const Acts::Transform3& alignedTransform,
                           unsigned int iov);

  void clearAlignedTransform(unsigned int iov);

 private:
  std::unordered_map<unsigned int, Acts::Transform3> m_alignedTransforms;
  mutable std::mutex m_alignmentMutex;
};

inline const Acts::Transform3& InternallyAlignedDetectorElement::transform(
    const Acts::GeometryContext& gctx) const {
  if (!gctx.hasValue()) {
    // Return the standard transform if geo context is empty
    return nominalTransform(gctx);
  }
  const auto& alignContext = gctx.get<ContextType&>();

  std::lock_guard lock{m_alignmentMutex};
  if (alignContext.nominal) {
    // nominal alignment
    return nominalTransform(gctx);
  }
  auto aTransform = m_alignedTransforms.find(alignContext.iov);
  if (aTransform == m_alignedTransforms.end()) {
    throw std::runtime_error{
        "Aligned transform for IOV " + std::to_string(alignContext.iov) +
        " not found. This can happen if the garbage collection runs too "
        "early (--align-flushsize too low)"};
  }
  return aTransform->second;
}

inline const Acts::Transform3&
InternallyAlignedDetectorElement::nominalTransform(
    const Acts::GeometryContext& gctx) const {
  return GenericDetectorElement::transform(gctx);
}

inline void InternallyAlignedDetectorElement::addAlignedTransform(
    const Acts::Transform3& alignedTransform, unsigned int iov) {
  std::lock_guard lock{m_alignmentMutex};
  m_alignedTransforms[iov] = alignedTransform;
}

inline void InternallyAlignedDetectorElement::clearAlignedTransform(
    unsigned int iov) {
  std::lock_guard lock{m_alignmentMutex};
  if (auto it = m_alignedTransforms.find(iov);
      it != m_alignedTransforms.end()) {
    m_alignedTransforms.erase(it);
  }
}

}  // namespace ActsExamples
