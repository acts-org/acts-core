// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"

namespace Acts {

/// @class LinearizedTrack
///
/// Class for linear expansion of track parameters in vicinity of vertex
///
/// The measurement equation is linearized in the following way:
///
/// F_k= D_k (x_k - x_0k) + E_k (p_k - p_0k) + F^0_k
///
/// where F_k are the parameters at perigee nearest to the linearization point,
/// x_k is the position of the vertex, p_k the track momentum at the vertex,
/// and F^0_k is the constant term of expansion. D_k and E_k are matrices
/// of derivatives, denoted hereafter as "positionJacobian" and
/// "momentumJacobian" respectively.
///

struct LinearizedTrack {
  LinearizedTrack() = default;

  /// @brief Constructor taking perigee parameters and covariance matrix
  /// of track propagated to closest approach (PCA) of linearization point,
  /// position and momentum Jacobian and const term.
  ///
  /// @param paramsAtPCA Parameters at point of closest approach
  /// @param parCovarianceAtPCA Parameter covariance matrix at point of closest
  ///                           approach
  /// @param parWeightAtPCA The weight at the point of closest approach
  /// @param linPoint Linearization point
  /// @param posJacobian Position jacobian
  /// @param momJacobian Momentum jacobian
  /// @param position Position at point of closest approach
  /// @param momentum Momentum at point of closest approach
  /// @param constTerm Constant term in taylor expansion
  LinearizedTrack(const BoundVector& paramsAtPCA,
                  const BoundSquareMatrix& parCovarianceAtPCA,
                  const BoundSquareMatrix& parWeightAtPCA,
                  const Vector4& linPoint,
                  const ActsMatrix<eBoundSize, 4>& posJacobian,
                  const ActsMatrix<eBoundSize, 3>& momJacobian,
                  const Vector4& position, const Vector3& momentum,
                  const BoundVector& constTerm)
      : parametersAtPCA(paramsAtPCA),
        covarianceAtPCA(parCovarianceAtPCA),
        weightAtPCA(parWeightAtPCA),
        linearizationPoint(linPoint),
        positionJacobian(posJacobian),
        momentumJacobian(momJacobian),
        positionAtPCA(position),
        momentumAtPCA(momentum),
        constantTerm(constTerm) {}

  BoundVector parametersAtPCA{BoundVector::Zero()};
  BoundSquareMatrix covarianceAtPCA{BoundSquareMatrix::Zero()};
  BoundSquareMatrix weightAtPCA{BoundSquareMatrix::Zero()};
  Vector4 linearizationPoint{Vector4::Zero()};
  ActsMatrix<eBoundSize, 4> positionJacobian{ActsMatrix<eBoundSize, 4>::Zero()};
  ActsMatrix<eBoundSize, 3> momentumJacobian{ActsMatrix<eBoundSize, 3>::Zero()};
  Vector4 positionAtPCA{Vector4::Zero()};
  Vector3 momentumAtPCA{Vector3::Zero()};
  BoundVector constantTerm{BoundVector::Zero()};
};

}  // namespace Acts
