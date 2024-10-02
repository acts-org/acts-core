// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/FullBilloirVertexFitter.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Utilities/detail/periodic.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "Acts/Vertexing/VertexingError.hpp"

namespace {

/// @struct BilloirTrack
///
/// @brief Struct to cache track-specific matrix operations in Billoir fitter
struct BilloirTrack {
  BilloirTrack(const Acts::InputTrack& params) : originalTrack(params) {}

  BilloirTrack(const BilloirTrack& arg) = default;

  Acts::InputTrack originalTrack;
  double chi2 = 0;

  // We drop the summation index i from Ref. (1) for better readability
  Acts::ActsMatrix<Acts::eBoundSize, Acts::eBoundSize> W;  // Wi weight matrix
  Acts::ActsMatrix<Acts::eBoundSize, 4> D;  // Di (position Jacobian)
  Acts::ActsMatrix<Acts::eBoundSize, 3> E;  // Ei (momentum Jacobian)
  Acts::ActsSquareMatrix<3> C;              //  = sum{Ei^T Wi * Ei}
  Acts::ActsMatrix<4, 3> B;                 //  = Di^T * Wi * Ei
  Acts::ActsSquareMatrix<3> Cinv;           //  = (Ei^T * Wi * Ei)^-1
  Acts::Vector3 U;                          //  = Ei^T * Wi * dqi
  Acts::ActsMatrix<4, 3> BCinv;             //  = Bi * Ci^-1
  Acts::BoundVector deltaQ;
};

/// @struct BilloirVertex
///
/// @brief Struct to cache vertex-specific matrix operations in Billoir fitter
struct BilloirVertex {
  // A  = sum{Di^T * Wi * Di}
  Acts::SquareMatrix4 A = Acts::SquareMatrix4::Zero();
  // T  = sum{Di^T * Wi * dqi}
  Acts::Vector4 T = Acts::Vector4::Zero();
  // sumBCinvBt = sum{Bi * Ci^-1 * Bi^T}
  Acts::SquareMatrix4 sumBCinvBt = Acts::SquareMatrix4::Zero();
  // sumBCinvU = sum{B * Ci^-1 * Ui}
  Acts::Vector4 sumBCinvU = Acts::Vector4::Zero();
};

}  // namespace

Acts::Result<Acts::Vertex> Acts::FullBilloirVertexFitter::fit(
    const std::vector<InputTrack>& paramVector,
    const VertexingOptions& vertexingOptions,
    MagneticFieldProvider::Cache& fieldCache) const {
  unsigned int nTracks = paramVector.size();
  double chi2 = std::numeric_limits<double>::max();

  if (nTracks == 0) {
    return Vertex(Vector3(0., 0., 0.));
  }

  // Set number of degrees of freedom following Eq. 8.28 from Ref. (2):
  //
  // ndf = sum_i=1^nTracks rank(Wi^-1) - 3 * (nTracks + 1),
  //
  // where W_i denotes the weight matrix of the i-th track.
  // Assuming rank(W_i) = #(Perigee params) = 6 for all tracks, we have
  //
  // ndf = 3 * nTracks - 3.
  int ndf = 3 * nTracks - 3;
  // TODO: this seems strange - can we even do a vertex fit if we only have one
  // track?
  if (nTracks < 2) {
    ndf = 1;
  }

  // Since we add a term to the chi2 when adding a vertex constraint (see Ref.
  // (1)), the number of degrees of freedom increases
  if (vertexingOptions.useConstraintInFit) {
    ndf += 3;
  }

  std::vector<BilloirTrack> billoirTracks;
  std::vector<Vector3> trackMomenta;
  // Initial guess of the 4D vertex position
  Vector4 linPoint = vertexingOptions.constraint.fullPosition();
  Vertex fittedVertex;

  for (int nIter = 0; nIter < m_cfg.maxIterations; ++nIter) {
    billoirTracks.clear();
    double newChi2 = 0;
    BilloirVertex billoirVertex;

    Vector3 linPointPos = VectorHelpers::position(linPoint);
    // Make Perigee surface at linPointPos, transverse plane of Perigee
    // corresponds the global x-y plane
    const std::shared_ptr<PerigeeSurface> perigeeSurface =
        Surface::makeShared<PerigeeSurface>(linPointPos);

    // iterate over all tracks
    for (std::size_t iTrack = 0; iTrack < nTracks; ++iTrack) {
      const InputTrack& trackContainer = paramVector[iTrack];

      const auto& trackParams = m_cfg.extractParameters(trackContainer);

      auto result =
          m_cfg.trackLinearizer(trackParams, linPoint[3], *perigeeSurface,
                                vertexingOptions.geoContext,
                                vertexingOptions.magFieldContext, fieldCache);
      if (!result.ok()) {
        return result.error();
      }

      const auto& linTrack = *result;
      const auto& parametersAtPCA = linTrack.parametersAtPCA;
      double d0 = parametersAtPCA[BoundIndices::eBoundLoc0];
      double z0 = parametersAtPCA[BoundIndices::eBoundLoc1];
      double phi = parametersAtPCA[BoundIndices::eBoundPhi];
      double theta = parametersAtPCA[BoundIndices::eBoundTheta];
      double qOverP = parametersAtPCA[BoundIndices::eBoundQOverP];
      double t0 = parametersAtPCA[BoundIndices::eBoundTime];

      // Take the track momenta at the PCA as an initial estimate of the track
      // momenta at the vertex
      if (nIter == 0) {
        trackMomenta.push_back(Vector3(phi, theta, qOverP));
      }

      // Calculate F(V_0,p_0), i.e., the track parameters estimated from the
      // vertex position and the track momenta. fD0 = fZ0 = 0 because the track
      // originates at the vertex in the Billoir model.
      double fPhi = trackMomenta[iTrack][0];
      double fTheta = trackMomenta[iTrack][1];
      double fQOvP = trackMomenta[iTrack][2];
      double fTime = linPoint[FreeIndices::eFreeTime];
      BilloirTrack billoirTrack(trackContainer);

      billoirTrack.deltaQ << d0, z0, phi - fPhi, theta - fTheta, qOverP - fQOvP,
          t0 - fTime;

      // position jacobian (D matrix)
      ActsMatrix<eBoundSize, 4> D = linTrack.positionJacobian;

      // momentum jacobian (E matrix)
      ActsMatrix<eBoundSize, 3> E = linTrack.momentumJacobian;

      // cache some matrix multiplications
      BoundSquareMatrix W = linTrack.weightAtPCA;
      ActsMatrix<4, eBoundSize> DtW = D.transpose() * W;
      ActsMatrix<3, eBoundSize> EtW = E.transpose() * W;

      // compute track quantities for Billoir fit
      billoirTrack.D = D;
      billoirTrack.E = E;
      billoirTrack.W = W;
      billoirTrack.C = EtW * E;
      billoirTrack.B = DtW * E;                        // Di^T * Wi * Ei
      billoirTrack.U = EtW * billoirTrack.deltaQ;      // Ei^T * Wi * dqi
      billoirTrack.Cinv = (billoirTrack.C).inverse();  // (Ei^T * Wi * Ei)^-1
      billoirTrack.BCinv =
          billoirTrack.B * billoirTrack.Cinv;  // BCinv = Bi * Ci^-1

      // compute vertex quantities for Billoir fit
      billoirVertex.T += DtW * billoirTrack.deltaQ;  // sum{Di^T * Wi * dqi}
      billoirVertex.A += DtW * D;                    // sum{Di^T * Wi * Di}
      billoirVertex.sumBCinvU +=
          billoirTrack.BCinv * billoirTrack.U;  // sum{Bi * Ci^-1 * Ui}
      billoirVertex.sumBCinvBt +=
          billoirTrack.BCinv *
          billoirTrack.B.transpose();  // sum{Bi * Ci^-1 * Bi^T}

      billoirTracks.push_back(billoirTrack);
    }  // end loop tracks

    // (Matrix-valued) factor contributing to the vertex estimate update (i.e.,
    // to deltaV).
    // deltaVFac = T-sum{Bi*Ci^-1*Ui}
    Vector4 deltaVFac = billoirVertex.T - billoirVertex.sumBCinvU;

    // Inverse of the covariance matrix of the 4D vertex position (note that
    // Cov(V) = Cov(deltaV)), see Ref. (1).
    // invCovV = A-sum{Bi*Ci^-1*Bi^T}
    SquareMatrix4 invCovV = billoirVertex.A - billoirVertex.sumBCinvBt;
    if (vertexingOptions.useConstraintInFit) {
      // Position of vertex constraint in Billoir frame (i.e., in coordinate
      // system with origin at linPoint). This will be 0 for first iteration but
      // != 0 from second on since our first guess for the vertex position is
      // the vertex constraint position.
      Vector4 posInBilloirFrame =
          vertexingOptions.constraint.fullPosition() - linPoint;

      // For vertex constraint: T -> T + Cb^-1 (b - V0) where Cb is the
      // covariance matrix of the constraint, b is the constraint position, and
      // V0 is the vertex estimate (see Ref. (1)).
      deltaVFac += vertexingOptions.constraint.fullCovariance().inverse() *
                   posInBilloirFrame;
      // For vertex constraint: A -> A + Cb^-1
      invCovV += vertexingOptions.constraint.fullCovariance().inverse();
    }

    // Covariance matrix of the 4D vertex position, see Ref. (3)
    SquareMatrix4 covV = invCovV.inverse();
    // Update of the vertex position
    Vector4 deltaV = covV * deltaVFac;
    //--------------------------------------------------------------------------------------
    // start momentum related calculations

    std::vector<std::optional<BoundSquareMatrix>> covDeltaP(nTracks);

    // Update track momenta and calculate the covariance of the track parameters
    // after the fit (TODO: parameters -> momenta).
    for (std::size_t iTrack = 0; iTrack < billoirTracks.size(); ++iTrack) {
      auto& billoirTrack = billoirTracks[iTrack];

      Vector3 deltaP = (billoirTrack.Cinv) *
                       (billoirTrack.U - billoirTrack.B.transpose() * deltaV);

      // update track momenta
      trackMomenta[iTrack] += deltaP;

      // correct for 2PI / PI periodicity
      const auto correctedPhiTheta = detail::normalizePhiTheta(
          trackMomenta[iTrack][0], trackMomenta[iTrack][1]);
      trackMomenta[iTrack][0] = correctedPhiTheta.first;
      trackMomenta[iTrack][1] = correctedPhiTheta.second;

      // Calculate chi2 of the track...
      Acts::BoundVector diffQ = billoirTrack.deltaQ - billoirTrack.D * deltaV -
                                billoirTrack.E * deltaP;
      billoirTrack.chi2 = diffQ.transpose().dot(billoirTrack.W * diffQ);
      // ... and add it to the total chi2 value
      newChi2 += billoirTrack.chi2;

      // calculate the 6x6 cross-covariance matrix
      // TODO: replace fittedParams with fittedMomentum since d0 and z0 are
      // ill-defined. At the moment, only the submatrix of momentum covariances
      // is correct.

      // coordinate transformation matrix, i.e.,
      // d(d0,z0,phi,theta,qOverP,t)/d(x,y,z,t,phi,theta,qOverP)
      // TODO: This is not correct since the (x, y, z, t) in the derivatives in
      // the D matrix correspond to the global track position at the PCA rather
      // than the 4D vertex position.
      ActsMatrix<eBoundSize, 7> transMat;
      transMat.setZero();
      transMat.block<2, 2>(0, 0) = billoirTrack.D.template block<2, 2>(0, 0);
      transMat(1, 2) = 1.;
      transMat(2, 4) = 1.;
      transMat(3, 5) = 1.;
      transMat(4, 6) = 1.;
      transMat(5, 3) = 1.;

      // cov(V,P)
      ActsMatrix<4, 3> covVP = -covV * billoirTrack.BCinv;

      // cov(P,P), 3x3 matrix, see Ref. (3)
      ActsSquareMatrix<3> covP =
          billoirTrack.Cinv +
          billoirTrack.BCinv.transpose() * covV * billoirTrack.BCinv;

      ActsSquareMatrix<7> cov;
      cov.setZero();
      cov.block<4, 4>(0, 0) = covV;
      cov.block<4, 3>(0, 4) = covVP;
      cov.block<3, 4>(4, 0) = covVP.transpose();
      cov.block<3, 3>(4, 4) = covP;

      covDeltaP[iTrack] = transMat * cov * transMat.transpose();
    }

    // assign new linearization point (= new vertex position in global frame)
    linPoint += deltaV;

    // Add an additional term to chi2 if there is a vertex constraint (see Ref.
    // (1))
    if (vertexingOptions.useConstraintInFit) {
      // Position of vertex constraint in Billoir frame (i.e., in coordinate
      // system with origin at linPoint).
      Vector4 posInBilloirFrame =
          vertexingOptions.constraint.fullPosition() - linPoint;

      newChi2 +=
          (posInBilloirFrame.transpose())
              .dot(vertexingOptions.constraint.fullCovariance().inverse() *
                   posInBilloirFrame);
    }

    if (!std::isnormal(newChi2)) {
      ACTS_ERROR("Encountered non-normal chi2 value during the fit.");
      return VertexingError::NumericFailure;
    }

    if (newChi2 < chi2) {
      chi2 = newChi2;

      fittedVertex.setFullPosition(linPoint);
      fittedVertex.setFullCovariance(covV);
      fittedVertex.setFitQuality(chi2, ndf);

      std::vector<TrackAtVertex> tracksAtVertex;

      std::shared_ptr<PerigeeSurface> perigee =
          Surface::makeShared<PerigeeSurface>(
              VectorHelpers::position(linPoint));

      for (std::size_t iTrack = 0; iTrack < billoirTracks.size(); ++iTrack) {
        const auto& billoirTrack = billoirTracks[iTrack];

        // TODO: replace refittedParams with refittedMomenta since d0 and z0
        // after the vertex fit are ill-defined (FullBilloir only outputs the
        // updated track momenta)

        // new refitted trackparameters
        BoundVector paramVec = BoundVector::Zero();
        paramVec[eBoundPhi] = trackMomenta[iTrack](0);
        paramVec[eBoundTheta] = trackMomenta[iTrack](1);
        paramVec[eBoundQOverP] = trackMomenta[iTrack](2);
        paramVec[eBoundTime] = linPoint[FreeIndices::eFreeTime];
        BoundTrackParameters refittedParams(
            perigee, paramVec, covDeltaP[iTrack],
            m_cfg.extractParameters(billoirTrack.originalTrack)
                .particleHypothesis());
        TrackAtVertex trackAtVertex(billoirTrack.chi2, refittedParams,
                                    billoirTrack.originalTrack);
        tracksAtVertex.push_back(std::move(trackAtVertex));
      }

      fittedVertex.setTracksAtVertex(std::move(tracksAtVertex));
    }
  }  // end loop iterations

  return fittedVertex;
}
