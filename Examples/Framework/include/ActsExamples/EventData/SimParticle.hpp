// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Utilities/GroupBy.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <boost/container/flat_set.hpp>

namespace ActsExamples {
namespace detail {
struct CompareParticleId {
  using is_transparent = void;
  constexpr bool operator()(const ActsFatras::Particle& lhs,
                            const ActsFatras::Particle& rhs) const {
    return lhs.particleId() < rhs.particleId();
  }
  constexpr bool operator()(ActsFatras::Barcode lhs,
                            const ActsFatras::Particle& rhs) const {
    return lhs < rhs.particleId();
  }
  constexpr bool operator()(const ActsFatras::Particle& lhs,
                            ActsFatras::Barcode rhs) const {
    return lhs.particleId() < rhs;
  }
};
struct PrimaryVertexIdGetter {
  constexpr ActsFatras::Barcode operator()(
      const ActsFatras::Particle& particle) const {
    return ActsFatras::Barcode(0u).setVertexPrimary(
        particle.particleId().vertexPrimary());
  }
};
struct SecondaryVertexIdGetter {
  constexpr ActsFatras::Barcode operator()(
      const ActsFatras::Particle& particle) const {
    return ActsFatras::Barcode(0u)
        .setVertexPrimary(particle.particleId().vertexPrimary())
        .setVertexSecondary(particle.particleId().vertexSecondary());
  }
};
}  // namespace detail

using SimBarcode = ::ActsFatras::Barcode;
using SimParticle = ::ActsFatras::Particle;
/// Store particles ordered by particle identifier.
using SimBarcodeContainer = ::boost::container::flat_set<SimBarcode>;
using SimParticleContainer =
    ::boost::container::flat_set<SimParticle, detail::CompareParticleId>;

/// Iterate over groups of particles belonging to the same primary vertex.
inline GroupBy<SimParticleContainer::const_iterator,
               detail::PrimaryVertexIdGetter>
groupByPrimaryVertex(const SimParticleContainer& container) {
  return makeGroupBy(container, detail::PrimaryVertexIdGetter());
}

/// Iterate over groups of particles belonging to the same secondary vertex.
///
/// For each primary vertex, this yields one group of particles belonging
/// directly to the primary vertex (secondary vertex id 0) and a group for
/// each secondary vertex.
inline GroupBy<SimParticleContainer::const_iterator,
               detail::SecondaryVertexIdGetter>
groupBySecondaryVertex(const SimParticleContainer& container) {
  return makeGroupBy(container, detail::SecondaryVertexIdGetter());
}

}  // namespace ActsExamples
