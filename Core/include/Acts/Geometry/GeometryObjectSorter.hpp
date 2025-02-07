// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

// clang-format off
// Workaround for building on clang+libstdc++. Must always be first
#include "Acts/Utilities/detail/ReferenceWrapperAnyCompat.hpp"
// clang-format on

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <functional>
#include <memory>

namespace Acts {

template <class T>
class ObjectSorterT {
 public:
  /// Constructor from a binning value
  ///
  /// @param aDir is the direction in which the sorting is done
  explicit ObjectSorterT(AxisDirection aDir) : m_sortingDirection(aDir) {}

  /// Comparison operator
  ///
  /// @param one first object
  /// @param two second object
  ///
  /// @return boolean indicator
  bool operator()(T one, T two) const {
    using VectorHelpers::eta;
    using VectorHelpers::perp;
    using VectorHelpers::phi;
    using enum AxisDirection;
    switch (m_sortingDirection) {
      // compare on x
      case AxisX: {
        return one.x() < two.x();
      }
      // compare on y
      case AxisY: {
        return one.y() < two.y();
      }
      // compare on z
      case AxisZ: {
        return one.z() < two.z();
      }
      // compare on r
      case AxisR: {
        return perp(one) < perp(two);
      }
      // compare on phi
      case AxisPhi: {
        return phi(one) < phi(two);
      }
      // compare on eta
      case AxisEta: {
        return eta(one) < eta(two);
      }
      // default for the moment
      default: {
        return one.norm() < two.norm();
      }
    }
  }

  AxisDirection sortingDirection() const { return m_sortingDirection; }

 private:
  AxisDirection m_sortingDirection;  ///< the binning value
};

/// This will check on absolute distance
template <class T>
class DistanceSorterT {
 public:
  /// Constructor from a binning value
  ///
  /// @param aDir is the value in which the sorting is done
  /// @param reference is the reference point
  DistanceSorterT(AxisDirection aDir, Vector3 reference)
      : m_sortingDirection(aDir),
        m_reference(reference),
        m_refR(VectorHelpers::perp(reference)),
        m_refPhi(VectorHelpers::phi(reference)),
        m_refEta(VectorHelpers::eta(reference)) {}

  /// Comparison operator
  ///
  /// @tparam one first object
  /// @tparam two second object
  ///
  /// @return boolean indicator
  bool operator()(T one, T two) const {
    using Acts::VectorHelpers::eta;
    using Acts::VectorHelpers::perp;
    using Acts::VectorHelpers::phi;
    // switch the sorting value
    // - AxisX, AxisY, AxisZ, AxisR, AxisPhi, AxisRPhi, AxisTheta, AxisEta
    switch (m_sortingDirection) {
      // compare on diff x
      case AxisDirection::AxisX: {
        double diffOneX = one.x() - m_reference.x();
        double diffTwoX = two.x() - m_reference.x();
        return std::abs(diffOneX) < std::abs(diffTwoX);
      }
      // compare on diff y
      case AxisDirection::AxisY: {
        double diffOneY = one.y() - m_reference.y();
        double diffTwoY = two.y() - m_reference.y();
        return std::abs(diffOneY) < std::abs(diffTwoY);
      }
      // compare on diff z
      case AxisDirection::AxisZ: {
        double diffOneZ = one.z() - m_reference.z();
        double diffTwoZ = two.z() - m_reference.z();
        return std::abs(diffOneZ) < std::abs(diffTwoZ);
      }
      // compare on r
      case AxisDirection::AxisR: {
        double diffOneR = perp(one) - m_refR;
        double diffTwoR = perp(two) - m_refR;
        return std::abs(diffOneR) < std::abs(diffTwoR);
      }
      // compare on phi /// @todo add cyclic value
      case AxisDirection::AxisPhi: {
        double diffOnePhi = phi(one) - m_refPhi;
        double diffTwoPhi = phi(two) - m_refPhi;
        return std::abs(diffOnePhi) < std::abs(diffTwoPhi);
      }
      // compare on eta
      case AxisDirection::AxisEta: {
        double diffOneEta = eta(one) - m_refEta;
        double diffTwoEta = eta(two) - m_refEta;
        return std::abs(diffOneEta) < std::abs(diffTwoEta);
      }
      // default for the moment
      default: {
        T diffOne(one - m_reference);
        T diffTwo(two - m_reference);
        return diffOne.mag2() < diffTwo.mag2();
      }
    }
  }

 private:
  AxisDirection m_sortingDirection;  ///< the sorting direction
  T m_reference;
  double m_refR;
  double m_refPhi;
  double m_refEta;
};

template <class T>
class GeometryObjectSorterT {
 public:
  /// Constructor from a sorting direction
  ///
  /// @param gctx The geometry context to use
  /// @param aDir is the direction in which the sorting is done
  /// @param transform is an optional transform to be performed
  GeometryObjectSorterT(const GeometryContext& gctx, AxisDirection aDir,
                        std::shared_ptr<const Transform3> transform = nullptr)
      : m_context(gctx),
        m_objectSorter(aDir),
        m_transform(std::move(transform)) {}

  /// Comparison operator
  ///
  /// @tparam one is the first object
  /// @tparam two is the second object
  ///
  /// @return boolean indicator
  bool operator()(T one, T two) const {
    // get the pos one / pos two
    Vector3 posOne = m_transform
                         ? m_transform->inverse() *
                               one->referencePosition(
                                   m_context, m_objectSorter.sortingDirection())
                         : one->referencePosition(
                               m_context, m_objectSorter.sortingDirection());
    Vector3 posTwo = m_transform
                         ? m_transform->inverse() *
                               two->referencePosition(
                                   m_context, m_objectSorter.sortingDirection())
                         : two->referencePosition(
                               m_context, m_objectSorter.sortingDirection());
    // now call the distance sorter
    return m_objectSorter.operator()(posOne, posTwo);
  }

 protected:
  std::reference_wrapper<const GeometryContext> m_context;
  ObjectSorterT<Vector3> m_objectSorter;
  std::shared_ptr<const Transform3> m_transform;
};
}  // namespace Acts
