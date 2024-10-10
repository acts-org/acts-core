// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Alignment.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/DetectorElementBase.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/Result.hpp"

namespace Acts {

template <typename S>
concept SurfaceConcept = requires(S s, const S cs, S s2, const S cs2,
                                  GeometryContext gctx,
                                  BoundaryTolerance tolerance) {
  { cs == s2 } -> std::same_as<bool>;

  { cs.type() } -> std::same_as<Surface::SurfaceType>;
  { cs.transform(gctx) } -> std::same_as<const Transform3&>;
  { cs.center(gctx) } -> std::same_as<Vector3>;
  { cs.normal(gctx, Vector3{}, Vector3{}) } -> std::same_as<Vector3>;
  { cs.bounds() } -> std::convertible_to<const SurfaceBounds&>;
  {
    cs.associatedDetectorElement()
  } -> std::same_as<const DetectorElementBase*>;

  { cs.associatedLayer() } -> std::same_as<const Layer*>;
  { s.associateLayer(std::declval<const Layer&>()) } -> std::same_as<void>;

  { cs.surfaceMaterial() } -> std::same_as<const ISurfaceMaterial*>;
  {
    cs.surfaceMaterialSharedPtr()
  } -> std::same_as<const std::shared_ptr<const ISurfaceMaterial>&>;
  {
    s.assignSurfaceMaterial(
        std::declval<std::shared_ptr<const ISurfaceMaterial>>())
  } -> std::same_as<void>;
  {
    cs.isOnSurface(gctx, Vector3{}, Vector3{}, tolerance)
  } -> std::same_as<bool>;
  { cs.insideBounds(Vector2{}, tolerance) } -> std::same_as<bool>;

  { cs.localToGlobal(gctx, Vector2{}, Vector3{}) } -> std::same_as<Vector3>;

  {
    cs.globalToLocal(gctx, Vector3{}, Vector3{}, double{5})
  } -> std::same_as<Result<Vector2>>;

  {
    cs.referenceFrame(gctx, Vector3{}, Vector3{})
  } -> std::same_as<RotationMatrix3>;

  {
    cs.boundToFreeJacobian(gctx, Vector3{}, Vector3{})
  } -> std::same_as<BoundToFreeMatrix>;

  {
    cs.freeToBoundJacobian(gctx, Vector3{}, Vector3{})
  } -> std::same_as<FreeToBoundMatrix>;

  {
    cs.freeToPathDerivative(gctx, Vector3{}, Vector3{})
  } -> std::same_as<FreeToPathMatrix>;

  { cs.pathCorrection(gctx, Vector3{}, Vector3{}) } -> std::same_as<double>;

  {
    cs.intersect(gctx, Vector3{}, Vector3{}, tolerance, std::declval<double>())
  } -> std::same_as<SurfaceMultiIntersection>;

  { cs.toStream(gctx) } -> std::same_as<GeometryContextOstreamWrapper<Surface>>;

  { cs.toString(gctx) } -> std::same_as<std::string>;

  { cs.name() } -> std::same_as<std::string>;

  {
    cs.polyhedronRepresentation(gctx, std::declval<unsigned int>())
  } -> std::same_as<Polyhedron>;

  {
    cs.alignmentToBoundDerivative(gctx, Vector3{}, Vector3{}, FreeVector{})
  } -> std::same_as<AlignmentToBoundMatrix>;

  {
    cs.alignmentToPathDerivative(gctx, Vector3{}, Vector3{})
  } -> std::same_as<AlignmentToPathMatrix>;

  {
    cs.localCartesianToBoundLocalDerivative(gctx, Vector3{})
  } -> std::same_as<ActsMatrix<2, 3>>;
};

template <typename S>
concept RegularSurfaceConcept =
    SurfaceConcept<S> && requires(S s, const S cs, GeometryContext gctx,
                                  BoundaryTolerance tolerance) {
      { cs.normal(gctx, Vector2{}) } -> std::same_as<Vector3>;

      { cs.normal(gctx, Vector3{}) } -> std::same_as<Vector3>;

      {
        cs.globalToLocal(gctx, Vector3{}, Vector3{}, std::declval<double>())
      } -> std::same_as<Result<Vector2>>;

      { cs.localToGlobal(gctx, Vector2{}) } -> std::same_as<Vector3>;

      { cs.isOnSurface(gctx, Vector3{}, tolerance) } -> std::same_as<bool>;
    };
}  // namespace Acts
