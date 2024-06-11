// This file is part of the Acts project.
//
// Copyright (C) 2016-2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <cmath>
#include <iterator>
#include <optional>
#include <variant>
#include <vector>

namespace Acts {

/// Captures different types of boundary tolerances
/// - Infinite: infinite tolerance i.e. no boundary check
/// - None: no tolerance i.e. exact boundary check
/// - AbsoluteBound: absolute tolerance in bound coordinates
/// - AbsoluteCartesian: absolute tolerance in Cartesian coordinates
/// - AbsoluteEuclidean: absolute tolerance in Euclidean distance
/// - Chi2Bound: chi2 tolerance in bound coordinates
class BoundaryTolerance {
 public:
  /// Infinite tolerance i.e. no boundary check
  struct Infinite {};

  /// No tolerance i.e. exact boundary check
  struct None {};

  /// Absolute tolerance in bound coordinates
  struct AbsoluteBound {
    double tolerance0{};
    double tolerance1{};

    AbsoluteBound() = default;
    AbsoluteBound(double tolerance0_, double tolerance1_)
        : tolerance0(tolerance0_), tolerance1(tolerance1_) {}
  };

  /// Absolute tolerance in Cartesian coordinates
  struct AbsoluteCartesian {
    double tolerance0{};
    double tolerance1{};

    AbsoluteCartesian() = default;
    AbsoluteCartesian(double tolerance0_, double tolerance1_)
        : tolerance0(tolerance0_), tolerance1(tolerance1_) {}
  };

  /// Absolute tolerance in Euclidean distance
  struct AbsoluteEuclidean {
    double tolerance{};

    AbsoluteEuclidean() = default;
    AbsoluteEuclidean(double tolerance_) : tolerance(tolerance_) {}
  };

  /// Chi2 tolerance in bound coordinates
  struct Chi2Bound {
    SquareMatrix2 weight = SquareMatrix2::Identity();
    double maxChi2{};

    Chi2Bound() = default;
    Chi2Bound(const SquareMatrix2& weight_, double maxChi2_)
        : weight(weight_), maxChi2(maxChi2_) {}
  };

  /// Underlying variant type
  using Variant = std::variant<Infinite, None, AbsoluteBound, AbsoluteCartesian,
                               AbsoluteEuclidean, Chi2Bound>;

  /// Construct with infinite tolerance.
  BoundaryTolerance(const Infinite& infinite);
  /// Construct with no tolerance.
  BoundaryTolerance(const None& none);
  /// Construct with absolute tolerance in bound coordinates.
  BoundaryTolerance(const AbsoluteBound& AbsoluteBound);
  /// Construct with absolute tolerance in Cartesian coordinates.
  BoundaryTolerance(const AbsoluteCartesian& absoluteCartesian);
  /// Construct with absolute tolerance in Euclidean distance.
  BoundaryTolerance(const AbsoluteEuclidean& absoluteEuclidean);
  /// Construct with chi2 tolerance in bound coordinates.
  BoundaryTolerance(const Chi2Bound& Chi2Bound);

  /// Construct from variant
  BoundaryTolerance(Variant variant);

  /// Check if the tolerance is infinite.
  bool isInfinite() const;
  /// Check if the is no tolerance.
  bool isNone() const;
  /// Check if the tolerance is absolute with bound coordinates.
  bool hasAbsoluteBound(bool isCartesian = false) const;
  /// Check if the tolerance is absolute with Cartesian coordinates.
  bool hasAbsoluteCartesian() const;
  /// Check if the tolerance is absolute with Euclidean distance.
  bool hasAbsoluteEuclidean() const;
  /// Check if the tolerance is chi2 with bound coordinates.
  bool hasChi2Bound() const;

  /// Check if any tolerance is set.
  bool hasTolerance() const;

  /// Get the tolerance as absolute bound.
  AbsoluteBound asAbsoluteBound(bool isCartesian = false) const;
  /// Get the tolerance as absolute Cartesian.
  const AbsoluteCartesian& asAbsoluteCartesian() const;
  /// Get the tolerance as absolute Euclidean.
  const AbsoluteEuclidean& asAbsoluteEuclidean() const;
  /// Get the tolerance as chi2 bound.
  const Chi2Bound& asChi2Bound() const;

  /// Get the tolerance as absolute bound if possible.
  std::optional<AbsoluteBound> asAbsoluteBoundOpt(
      bool isCartesian = false) const;

  /// Check if the distance is tolerated.
  bool isTolerated(const Vector2& distance,
                   const std::optional<SquareMatrix2>& jacobianOpt) const;

  /// Check if there is a metric assigned with this tolerance.
  bool hasMetric(bool hasJacobian) const;

  /// Get the metric for the tolerance.
  SquareMatrix2 getMetric(const std::optional<SquareMatrix2>& jacobian) const;

 private:
  Variant m_variant;

  /// Check if the boundary check is of a specific type.
  template <typename T>
  bool holdsVariant() const;

  /// Get the specific underlying type.
  template <typename T>
  const T& getVariant() const;

  template <typename T>
  const T* getVariantPtr() const;
};

/// Utility to check if a point is inside a box.
class AlignedBoxBoundaryCheck {
 public:
  /// Construct with the lower left and upper right corners of the box.
  AlignedBoxBoundaryCheck(const Vector2& lowerLeft, const Vector2& upperRight,
                          BoundaryTolerance tolerance);

  /// Get the lower left corner of the box.
  const Vector2& lowerLeft() const;

  /// Get the upper right corner of the box.
  const Vector2& upperRight() const;

  std::array<Vector2, 4> vertices() const;

  /// Get the tolerance.
  const BoundaryTolerance& tolerance() const;

  /// Check if the point is inside the box.
  bool inside(const Vector2& point,
              const std::optional<SquareMatrix2>& jacobian) const;

 private:
  Vector2 m_lowerLeft;
  Vector2 m_upperRight;
  BoundaryTolerance m_tolerance;
};

/// Utility to check if a point is inside a polygon.
template <typename Vector2Container>
class PolygonBoundaryCheck {
 public:
  /// Construct with the vertices of the polygon.
  PolygonBoundaryCheck(const Vector2Container& vertices,
                       BoundaryTolerance tolerance);

  /// Get the vertices of the polygon.
  const Vector2Container& vertices() const;

  /// Get the tolerance.
  const BoundaryTolerance& tolerance() const;

  /// Check if the point is inside the polygon.
  bool inside(const Vector2& point,
              const std::optional<SquareMatrix2>& jacobian) const;

 private:
  const Vector2Container& m_vertices;
  BoundaryTolerance m_tolerance;
};

}  // namespace Acts

#include "Acts/Surfaces/BoundaryCheck.ipp"
