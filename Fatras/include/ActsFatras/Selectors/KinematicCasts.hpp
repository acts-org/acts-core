// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Utilities/VectorHelpers.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <cmath>

namespace ActsFatras::Casts {

/// Retrieve the transverse absolute distance of the position to the origin.
struct Vrho {
  double operator()(const Particle& particle) const {
    return std::hypot(particle.position().x(), particle.position().y());
  }
};

/// Retrieve the longitudinal distance of the position to the origin.
struct Vz {
  double operator()(const Particle& particle) const {
    return particle.position().z();
  }
};

/// Retrieve the longitudinal absolute distance of the position to the origin.
struct AbsVz {
  double operator()(const Particle& particle) const {
    return std::abs(particle.position().z());
  }
};

/// Retrieve the direction pseudo-rapidity.
struct Eta {
  double operator()(const Particle& particle) const {
    // particle direction is always normalized, i.e. dz = pz / p
    return std::atanh(particle.direction().z());
  }
};

/// Retrieve the direction absolute pseudo-rapidity.
struct AbsEta {
  double operator()(const Particle& particle) const {
    // particle direction is always normalized, i.e. dz = pz / p
    return std::atanh(std::abs(particle.direction().z()));
  }
};

/// Retrieve the transverse momentum.
struct Pt {
  double operator()(const Particle& particle) const {
    // particle direction is always normalized, i.e. dt²+dz²=1 w/ dt²=dx²+dy²
    return particle.absoluteMomentum() *
           Acts::VectorHelpers::perp(particle.direction());
  }
};

/// Retrieve the absolute momentum.
struct P {
  double operator()(const Particle& particle) const {
    return particle.absoluteMomentum();
  }
};

/// Retrieve the total energy.
struct E {
  double operator()(const Particle& particle) const {
    return particle.energy();
  }
};

}  // namespace ActsFatras::Casts
