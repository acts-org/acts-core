// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsFatras/EventData/Barcode.hpp"

#include <cstdint>

namespace ActsFatras {

/// A simulation hit on a surface.
///
/// This is the undigitized, truth hit, i.e. just a recording of the particle
/// state at the surface intersection. Since Fatras is surface-based, the hit
/// position is always constrained to a surface. Depending on the simulated
/// interactions the momentum state before and after might differ and is
/// thus stored as two separate four-vectors.
class Hit {
 public:
  using Scalar = Acts::ActsScalar;
  using Vector3 = Acts::ActsVector<3>;
  using Vector4 = Acts::ActsVector<4>;

  /// Construct default hit with (mostly) invalid information.
  Hit() = default;
  /// Construct from four-position and four-momenta.
  ///
  /// @param geometryId      Geometry identifier of the surface
  /// @param particleId Particle identifier of the particle that created the hit
  /// @param pos4       Particle space-time four-vector on the surface
  /// @param before4    Particle four-momentum before the interaction
  /// @param after4     Particle four-momentum after the interaction
  /// @param index_     Hit index along the particle trajectory
  ///
  /// All quantities are given in the global coordinate system. It is the
  /// users responsibility to ensure that the position correspond to a
  /// position on the given surface.
  Hit(Acts::GeometryIdentifier geometryId, Barcode particleId,
      const Vector4& pos4, const Vector4& before4, const Vector4& after4,
      std::int32_t index_ = -1)
      : m_geometryId(geometryId),
        m_particleId(particleId),
        m_index(index_),
        m_pos4(pos4),
        m_before4(before4),
        m_after4(after4) {}
  Hit(const Hit&) = default;
  Hit(Hit&&) = default;
  Hit& operator=(const Hit&) = default;
  Hit& operator=(Hit&&) = default;

  /// Geometry identifier of the hit surface.
  constexpr Acts::GeometryIdentifier geometryId() const { return m_geometryId; }
  /// Particle identifier of the particle that generated the hit.
  constexpr Barcode particleId() const { return m_particleId; }
  /// Hit index along the particle trajectory.
  ///
  /// @retval negative if the hit index is undefined.
  constexpr std::int32_t index() const { return m_index; }

  /// Space-time position four-vector.
  const Vector4& fourPosition() const { return m_pos4; }
  /// Three-position, i.e. spatial coordinates without the time.
  auto position() const { return m_pos4.segment<3>(Acts::ePos0); }
  /// Time coordinate.
  Scalar time() const { return m_pos4[Acts::eTime]; }

  /// Particle four-momentum before the hit.
  const Vector4& momentum4Before() const { return m_before4; }
  /// Particle four-momentum after the hit.
  const Vector4& momentum4After() const { return m_after4; }
  /// Normalized particle direction vector before the hit.
  Vector3 directionBefore() const {
    return m_before4.segment<3>(Acts::eMom0).normalized();
  }
  /// Normalized particle direction vector the hit.
  Vector3 directionAfter() const {
    return m_after4.segment<3>(Acts::eMom0).normalized();
  }
  /// Average normalized particle direction vector through the surface.
  Vector3 direction() const {
    auto dir0 = m_before4.segment<3>(Acts::eMom0).normalized();
    auto dir1 = m_after4.segment<3>(Acts::eMom0).normalized();
    return ((dir0 + dir1) / 2.).segment<3>(Acts::eMom0).normalized();
  }
  /// Energy deposited by the hit.
  ///
  /// @retval positive if the particle lost energy when it passed the surface
  /// @retval negative if magic was involved
  Scalar depositedEnergy() const {
    return m_before4[Acts::eEnergy] - m_after4[Acts::eEnergy];
  }

 private:
  /// Identifier of the surface.
  Acts::GeometryIdentifier m_geometryId;
  /// Identifier of the generating particle.
  Barcode m_particleId;
  /// Index of the hit along the particle trajectory.
  std::int32_t m_index = -1;
  /// Global space-time position four-vector.
  Vector4 m_pos4 = Vector4::Zero();
  /// Global particle energy-momentum four-vector before the hit.
  Vector4 m_before4 = Vector4::Zero();
  /// Global particle energy-momentum four-vector after the hit.
  Vector4 m_after4 = Vector4::Zero();
};

}  // namespace ActsFatras
