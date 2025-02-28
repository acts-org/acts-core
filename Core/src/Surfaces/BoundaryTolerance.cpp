// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/BoundaryTolerance.hpp"

#include "Acts/Definitions/Algebra.hpp"

#include <stdexcept>
#include <utility>

namespace Acts {

BoundaryTolerance::BoundaryTolerance(Variant variant)
    : m_variant{std::move(variant)} {}

bool BoundaryTolerance::isInfinite() const {
  return holdsVariant<InfiniteParams>();
}

bool BoundaryTolerance::isNone() const {
  return holdsVariant<NoneParams>();
}

bool BoundaryTolerance::hasAbsoluteBound(bool isCartesian) const {
  return holdsVariant<NoneParams>() || holdsVariant<AbsoluteBoundParams>() ||
         (isCartesian && holdsVariant<AbsoluteCartesianParams>());
}

bool BoundaryTolerance::hasAbsoluteCartesian() const {
  return holdsVariant<AbsoluteCartesianParams>();
}

bool BoundaryTolerance::hasAbsoluteEuclidean() const {
  return holdsVariant<AbsoluteEuclideanParams>();
}

bool BoundaryTolerance::hasChi2Bound() const {
  return holdsVariant<Chi2BoundParams>();
}

BoundaryTolerance::Mode BoundaryTolerance::mode() const {
  using enum Mode;

  if (isInfinite()) {
    return Extend;
  }

  if (isNone()) {
    return None;
  }

  if (const auto* absoluteBound = getVariantPtr<AbsoluteBoundParams>();
      absoluteBound != nullptr) {
    if (absoluteBound->tolerance0 == 0. && absoluteBound->tolerance1 == 0.) {
      return None;
    }

    return Extend;
  }

  if (const auto* absoluteCartesian = getVariantPtr<AbsoluteCartesianParams>();
      absoluteCartesian != nullptr) {
    if (absoluteCartesian->tolerance0 == 0. &&
        absoluteCartesian->tolerance1 == 0.) {
      return None;
    }

    return Extend;
  }

  if (const auto* absoluteEuclidean = getVariantPtr<AbsoluteEuclideanParams>();
      absoluteEuclidean != nullptr) {
    if (absoluteEuclidean->tolerance == 0.) {
      return None;
    } else if (absoluteEuclidean->tolerance > 0.) {
      return Extend;
    } else {
      return Shrink;
    }
  }

  if (const auto* chi2Bound = getVariantPtr<Chi2BoundParams>();
      chi2Bound != nullptr) {
    if (chi2Bound->maxChi2 == 0.) {
      return None;
    } else if (chi2Bound->maxChi2 >= 0.) {
      return Extend;
    } else {
      return Shrink;
    }
  }

  assert(false && "Unsupported tolerance type");
  return None;
}

BoundaryTolerance::AbsoluteBoundParams BoundaryTolerance::asAbsoluteBound(
    bool isCartesian) const {
  if (isNone()) {
    return AbsoluteBoundParams{0., 0.};
  }

  if (isCartesian && hasAbsoluteCartesian()) {
    const auto& cartesian = getVariant<AbsoluteCartesianParams>();
    return AbsoluteBoundParams{cartesian.tolerance0, cartesian.tolerance1};
  }

  return getVariant<AbsoluteBoundParams>();
}

const BoundaryTolerance::AbsoluteCartesianParams&
BoundaryTolerance::asAbsoluteCartesian() const {
  return getVariant<AbsoluteCartesianParams>();
}

const BoundaryTolerance::AbsoluteEuclideanParams&
BoundaryTolerance::asAbsoluteEuclidean() const {
  return getVariant<AbsoluteEuclideanParams>();
}

const BoundaryTolerance::Chi2BoundParams& BoundaryTolerance::asChi2Bound()
    const {
  return getVariant<Chi2BoundParams>();
}

std::optional<BoundaryTolerance::AbsoluteBoundParams>
BoundaryTolerance::asAbsoluteBoundOpt(bool isCartesian) const {
  return hasAbsoluteBound(isCartesian)
             ? std::optional(asAbsoluteBound(isCartesian))
             : std::nullopt;
}

bool BoundaryTolerance::isTolerated(
    const Vector2& distance,
    const std::optional<SquareMatrix2>& jacobianOpt) const {
  if (isInfinite()) {
    return true;
  }

  if (isNone()) {
    return distance == Vector2::Zero();
  }

  if (const auto* absoluteBound = getVariantPtr<AbsoluteBoundParams>();
      absoluteBound != nullptr) {
    return std::abs(distance[0]) <= absoluteBound->tolerance0 &&
           std::abs(distance[1]) <= absoluteBound->tolerance1;
  }

  if (const auto* chi2Bound = getVariantPtr<Chi2BoundParams>();
      chi2Bound != nullptr) {
    // Mahalanobis distances mean is 2 in 2-dim. cut is 1-d sigma.
    double chi2 = distance.transpose() * chi2Bound->weight * distance;
    if (chi2Bound->maxChi2 < 0) {
      return chi2 > 2 * std::abs(chi2Bound->maxChi2);
    } else {
      return chi2 <= 2 * chi2Bound->maxChi2;
    }
  }

  bool isCartesian = !jacobianOpt.has_value();
  Vector2 cartesianDistance;
  if (isCartesian) {
    cartesianDistance = distance;
  } else {
    const auto& jacobian = *jacobianOpt;
    cartesianDistance = jacobian * distance;
  }

  if (const auto* absoluteCartesian = getVariantPtr<AbsoluteCartesianParams>();
      absoluteCartesian != nullptr) {
    return std::abs(cartesianDistance[0]) <= absoluteCartesian->tolerance0 &&
           std::abs(cartesianDistance[1]) <= absoluteCartesian->tolerance1;
  }

  if (const auto* absoluteEuclidean = getVariantPtr<AbsoluteEuclideanParams>();
      absoluteEuclidean != nullptr) {
    if (absoluteEuclidean->tolerance < 0) {
      return cartesianDistance.norm() > std::abs(absoluteEuclidean->tolerance);
    } else {
      return cartesianDistance.norm() <= absoluteEuclidean->tolerance;
    }
  }

  throw std::logic_error("Unsupported tolerance type");
}

}  // namespace Acts
