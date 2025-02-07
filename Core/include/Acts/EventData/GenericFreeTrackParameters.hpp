// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/TrackParameterHelpers.hpp"
#include "Acts/EventData/TrackParametersConcept.hpp"
#include "Acts/EventData/TransformationHelpers.hpp"
#include "Acts/EventData/detail/PrintParameters.hpp"
#include "Acts/Utilities/MathHelpers.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <cmath>
#include <optional>

namespace Acts {

/// Track parameters not bound to a surface for a single track.
///
/// @tparam particle_hypothesis_t Helper type to interpret the particle charge/momentum
///
/// Parameters and covariance matrix are stored using the free parametrization
/// defined in `enum FreeIndices`.
template <class particle_hypothesis_t>
class GenericFreeTrackParameters {
 public:
  using ParametersVector = FreeVector;
  using CovarianceMatrix = FreeSquareMatrix;
  using ParticleHypothesis = particle_hypothesis_t;

  /// Construct from a parameters vector and particle charge.
  ///
  /// @param params Free parameters vector
  /// @param cov Free parameters covariance matrix
  /// @param particleHypothesis Particle hypothesis
  ///
  /// In principle, only the charge magnitude is needed her to allow unambiguous
  /// extraction of the absolute momentum. The particle charge is required as
  /// an input here to be consistent with the other constructors below that
  /// that also take the charge as an input. The charge sign is only used in
  /// debug builds to check for consistency with the q/p parameter.
  GenericFreeTrackParameters(const ParametersVector& params,
                             std::optional<CovarianceMatrix> cov,
                             ParticleHypothesis particleHypothesis)
      : m_params(params),
        m_cov(std::move(cov)),
        m_particleHypothesis(std::move(particleHypothesis)) {
    assert(isFreeVectorValid(m_params) && "Invalid free parameters vector");
  }

  /// Construct from four-position, direction, absolute momentum, and charge.
  ///
  /// @param pos4 Track position/time four-vector
  /// @param dir Track direction three-vector; normalization is ignored.
  /// @param qOverP Charge over momentum
  /// @param cov Free parameters covariance matrix
  /// @param particleHypothesis Particle hypothesis
  GenericFreeTrackParameters(const Vector4& pos4, const Vector3& dir,
                             double qOverP, std::optional<CovarianceMatrix> cov,
                             ParticleHypothesis particleHypothesis)
      : m_params(FreeVector::Zero()),
        m_cov(std::move(cov)),
        m_particleHypothesis(std::move(particleHypothesis)) {
    m_params[eFreePos0] = pos4[ePos0];
    m_params[eFreePos1] = pos4[ePos1];
    m_params[eFreePos2] = pos4[ePos2];
    m_params[eFreeTime] = pos4[eTime];
    m_params[eFreeDir0] = dir[eMom0];
    m_params[eFreeDir1] = dir[eMom1];
    m_params[eFreeDir2] = dir[eMom2];
    m_params[eFreeQOverP] = qOverP;

    assert(isFreeVectorValid(m_params) && "Invalid free parameters vector");
  }

  /// Construct from four-position, angles, absolute momentum, and charge.
  ///
  /// @param pos4 Track position/time four-vector
  /// @param phi Transverse track direction angle
  /// @param theta Longitudinal track direction angle
  /// @param qOverP Charge over momentum
  /// @param cov Free parameters covariance matrix
  /// @param particleHypothesis Particle hypothesis
  GenericFreeTrackParameters(const Vector4& pos4, double phi, double theta,
                             double qOverP, std::optional<CovarianceMatrix> cov,
                             ParticleHypothesis particleHypothesis)
      : m_params(FreeVector::Zero()),
        m_cov(std::move(cov)),
        m_particleHypothesis(std::move(particleHypothesis)) {
    auto dir = makeDirectionFromPhiTheta(phi, theta);
    m_params[eFreePos0] = pos4[ePos0];
    m_params[eFreePos1] = pos4[ePos1];
    m_params[eFreePos2] = pos4[ePos2];
    m_params[eFreeTime] = pos4[eTime];
    m_params[eFreeDir0] = dir[eMom0];
    m_params[eFreeDir1] = dir[eMom1];
    m_params[eFreeDir2] = dir[eMom2];
    m_params[eFreeQOverP] = qOverP;

    assert(isFreeVectorValid(m_params) && "Invalid free parameters vector");
  }

  /// Converts a free track parameter with a different hypothesis.
  template <typename other_particle_hypothesis_t>
  GenericFreeTrackParameters(
      const GenericFreeTrackParameters<other_particle_hypothesis_t>& other)
      : GenericFreeTrackParameters(other.parameters(),
                                   other.particleHypothesis(),
                                   other.covariance()) {}

  /// Converts an unknown bound track parameter.
  template <typename other_track_parameter_t>
  static GenericFreeTrackParameters create(
      const other_track_parameter_t& other) {
    static_assert(FreeTrackParametersConcept<other_track_parameter_t>);

    return GenericFreeTrackParameters(
        other.parameters(), other.particleHypothesis(), other.covariance());
  }

  /// Parameters are not default constructible due to the charge type.
  GenericFreeTrackParameters() = delete;
  GenericFreeTrackParameters(const GenericFreeTrackParameters&) = default;
  GenericFreeTrackParameters(GenericFreeTrackParameters&&) = default;
  ~GenericFreeTrackParameters() = default;
  GenericFreeTrackParameters& operator=(const GenericFreeTrackParameters&) =
      default;
  GenericFreeTrackParameters& operator=(GenericFreeTrackParameters&&) = default;

  /// Parameters vector.
  const ParametersVector& parameters() const { return m_params; }
  /// Optional covariance matrix.
  const std::optional<CovarianceMatrix>& covariance() const { return m_cov; }

  /// Access a single parameter value identified by its index.
  ///
  /// @tparam kIndex Track parameter index
  template <FreeIndices kIndex>
  double get() const {
    return m_params[kIndex];
  }

  /// Space-time position four-vector.
  Vector4 fourPosition() const {
    Vector4 pos4;
    pos4[ePos0] = m_params[eFreePos0];
    pos4[ePos1] = m_params[eFreePos1];
    pos4[ePos2] = m_params[eFreePos2];
    pos4[eTime] = m_params[eFreeTime];
    return pos4;
  }
  /// Spatial position three-vector.
  Vector3 position() const { return m_params.segment<3>(eFreePos0); }
  /// Time coordinate.
  double time() const { return m_params[eFreeTime]; }

  /// Phi direction.
  double phi() const { return VectorHelpers::phi(direction()); }
  /// Theta direction.
  double theta() const { return VectorHelpers::theta(direction()); }
  /// Charge over momentum.
  double qOverP() const { return m_params[eFreeQOverP]; }

  /// Unit direction three-vector, i.e. the normalized momentum three-vector.
  Vector3 direction() const {
    return m_params.segment<3>(eFreeDir0).normalized();
  }
  /// Absolute momentum.
  double absoluteMomentum() const {
    return m_particleHypothesis.extractMomentum(m_params[eFreeQOverP]);
  }
  /// Transverse momentum.
  double transverseMomentum() const {
    // direction vector w/ arbitrary normalization can be parametrized as
    //   [f*sin(theta)*cos(phi), f*sin(theta)*sin(phi), f*cos(theta)]
    // w/ f,sin(theta) positive, the transverse magnitude is then
    //   sqrt(f^2*sin^2(theta)) = f*sin(theta)
    double transverseMagnitude2 =
        square(m_params[eFreeDir0]) + square(m_params[eFreeDir1]);
    // absolute magnitude is f by construction
    double magnitude2 = transverseMagnitude2 + square(m_params[eFreeDir2]);
    // such that we can extract sin(theta) = f*sin(theta) / f
    return std::sqrt(transverseMagnitude2 / magnitude2) * absoluteMomentum();
  }
  /// Momentum three-vector.
  Vector3 momentum() const { return absoluteMomentum() * direction(); }

  /// Particle electric charge.
  double charge() const {
    return m_particleHypothesis.extractCharge(get<eFreeQOverP>());
  }

  /// Particle hypothesis.
  const ParticleHypothesis& particleHypothesis() const {
    return m_particleHypothesis;
  }

  /// Reflect the parameters in place.
  void reflectInPlace() { m_params = reflectFreeParameters(m_params); }

  /// Reflect the parameters.
  /// @return Reflected parameters.
  GenericFreeTrackParameters<ParticleHypothesis> reflect() const {
    GenericFreeTrackParameters<ParticleHypothesis> reflected = *this;
    reflected.reflectInPlace();
    return reflected;
  }

 private:
  FreeVector m_params;
  std::optional<FreeSquareMatrix> m_cov;
  // TODO use [[no_unique_address]] once we switch to C++20
  ParticleHypothesis m_particleHypothesis;

  /// Print information to the output stream.
  friend std::ostream& operator<<(std::ostream& os,
                                  const GenericFreeTrackParameters& tp) {
    detail::printFreeParameters(
        os, tp.parameters(),
        tp.covariance().has_value() ? &tp.covariance().value() : nullptr);
    return os;
  }
};

}  // namespace Acts
