// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Result.hpp"

#include <array>
#include <memory>
#include <utility>

namespace Acts {
class PlanarBounds;
}  // namespace Acts

using namespace Acts::UnitLiterals;

namespace Acts::Test {

/// @class AlignmentContext
struct AlignmentContext {
  /// We have 2 different transforms
  std::shared_ptr<const std::array<Transform3, 2>> alignmentStore = nullptr;

  /// Context index
  unsigned int alignmentIndex{0};

  /// Default constructor
  AlignmentContext() = default;

  /// Constructor with Store and context index
  AlignmentContext(std::shared_ptr<const std::array<Transform3, 2>> aStore,
                   unsigned int aIndex = 0)
      : alignmentStore(std::move(aStore)), alignmentIndex(aIndex) {}
};

/// @class AlignableDetectorElement
///
/// This is a lightweight type of detector element,
/// it simply implements the base class.
class AlignableDetectorElement : public DetectorElementBase {
 public:
  // Deleted default constructor
  AlignableDetectorElement() = delete;

  /// Constructor for single sided detector element
  /// - bound to a Plane Surface
  ///
  /// @param transform is the transform that element the layer in 3D frame
  /// @param pBounds is the planar bounds for the planar detector element
  /// @param thickness is the module thickness
  AlignableDetectorElement(std::shared_ptr<const Transform3> transform,
                           const std::shared_ptr<const PlanarBounds>& pBounds,
                           double thickness)
      : DetectorElementBase(),
        m_elementTransform(std::move(transform)),
        m_elementThickness(thickness) {
    m_elementSurface = Surface::makeShared<PlaneSurface>(pBounds, *this);
  }

  ///  Destructor
  ~AlignableDetectorElement() override = default;

  /// Return local to global transform associated with this identifier
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  ///
  /// @note this is called from the surface().transform() in the PROXY mode
  const Transform3& transform(const GeometryContext& gctx) const override;

  /// Return surface associated with this detector element
  const Surface& surface() const override;

  /// Non-const access to the surface associated with this detector element
  Surface& surface() override;

  /// The maximal thickness of the detector element wrt normal axis
  double thickness() const override;

 private:
  /// the transform for positioning in 3D space
  std::shared_ptr<const Transform3> m_elementTransform;
  /// the surface represented by it
  std::shared_ptr<Surface> m_elementSurface{nullptr};
  /// the element thickness
  double m_elementThickness{0.};
};

inline const Transform3& AlignableDetectorElement::transform(
    const GeometryContext& gctx) const {
  auto alignContext = gctx.get<AlignmentContext>();
  if (alignContext.alignmentStore != nullptr &&
      alignContext.alignmentIndex < 2) {
    return (*(alignContext.alignmentStore))[alignContext.alignmentIndex];
  }
  return (*m_elementTransform);
}

inline const Surface& AlignableDetectorElement::surface() const {
  return *m_elementSurface;
}

inline Surface& AlignableDetectorElement::surface() {
  return *m_elementSurface;
}

inline double AlignableDetectorElement::thickness() const {
  return m_elementThickness;
}

/// Unit test for creating compliant/non-compliant Surface object
BOOST_AUTO_TEST_CASE(AlignmentContextTests) {
  // The nominal and alignments
  Vector3 nominalCenter(0., 0., 0.);
  Vector3 negativeCenter(0., 0., -1.);
  Vector3 positiveCenter(0., 0., 1.);

  // Checkpoints
  Vector3 onNominal(3., 3., 0.);
  Vector3 onNegative(3., 3., -1.);
  Vector3 onPositive(3., 3., 1.);

  // Local position
  Vector2 localPosition(3., 3.);

  // A position placeholder and dummy momentum
  Vector3 globalPosition(100_cm, 100_cm, 100_cm);
  Vector3 dummyMomentum(4., 4., 4.);

  Transform3 negativeTransform = Transform3::Identity();
  negativeTransform.translation() = negativeCenter;

  Transform3 positiveTransform = Transform3::Identity();
  positiveTransform.translation() = positiveCenter;

  std::array<Transform3, 2> alignmentArray = {negativeTransform,
                                              positiveTransform};

  std::shared_ptr<const std::array<Transform3, 2>> alignmentStore =
      std::make_shared<const std::array<Transform3, 2>>(alignmentArray);

  // The detector element at nominal position
  AlignableDetectorElement alignedElement(
      std::make_shared<const Transform3>(Transform3::Identity()),
      std::make_shared<const RectangleBounds>(100_cm, 100_cm), 1_mm);

  const auto& alignedSurface = alignedElement.surface();

  // The alignment contexts
  GeometryContext defaultContext{AlignmentContext{}};
  GeometryContext negativeContext{AlignmentContext{alignmentStore, 0}};
  GeometryContext positiveContext{AlignmentContext{alignmentStore, 1}};

  // Test the transforms
  BOOST_CHECK(alignedSurface.transform(defaultContext)
                  .isApprox(Transform3::Identity()));
  BOOST_CHECK(
      alignedSurface.transform(negativeContext).isApprox(negativeTransform));
  BOOST_CHECK(
      alignedSurface.transform(positiveContext).isApprox(positiveTransform));

  // Test the centers
  BOOST_CHECK_EQUAL(alignedSurface.center(defaultContext), nominalCenter);
  BOOST_CHECK_EQUAL(alignedSurface.center(negativeContext), negativeCenter);
  BOOST_CHECK_EQUAL(alignedSurface.center(positiveContext), positiveCenter);

  // Test OnSurface
  BOOST_CHECK(
      alignedSurface.isOnSurface(defaultContext, onNominal, dummyMomentum));
  BOOST_CHECK(
      alignedSurface.isOnSurface(negativeContext, onNegative, dummyMomentum));
  BOOST_CHECK(
      alignedSurface.isOnSurface(positiveContext, onPositive, dummyMomentum));

  // Test local to Global and vice versa
  globalPosition = alignedSurface.localToGlobal(defaultContext, localPosition,
                                                dummyMomentum);
  BOOST_CHECK_EQUAL(globalPosition, onNominal);
  localPosition =
      alignedSurface.globalToLocal(defaultContext, onNominal, dummyMomentum)
          .value();
  BOOST_CHECK_EQUAL(localPosition, Vector2(3., 3.));

  globalPosition = alignedSurface.localToGlobal(negativeContext, localPosition,
                                                dummyMomentum);
  BOOST_CHECK_EQUAL(globalPosition, onNegative);
  localPosition =
      alignedSurface.globalToLocal(negativeContext, onNegative, dummyMomentum)
          .value();
  BOOST_CHECK_EQUAL(localPosition, Vector2(3., 3.));

  globalPosition = alignedSurface.localToGlobal(positiveContext, localPosition,
                                                dummyMomentum);
  BOOST_CHECK_EQUAL(globalPosition, onPositive);
  localPosition =
      alignedSurface.globalToLocal(positiveContext, onPositive, dummyMomentum)
          .value();
  BOOST_CHECK_EQUAL(localPosition, Vector2(3., 3.));
}

}  // namespace Acts::Test
