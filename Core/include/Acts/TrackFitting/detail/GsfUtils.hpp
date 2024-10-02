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
#include "Acts/EventData/MultiComponentTrackParameters.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <map>
#include <numeric>
#include <ostream>
#include <tuple>
#include <vector>

namespace Acts {

/// The tolerated difference to 1 to accept weights as normalized
constexpr static double s_normalizationTolerance = 1.e-4;

namespace detail {

template <typename component_range_t, typename projector_t>
bool weightsAreNormalized(const component_range_t &cmps,
                          const projector_t &proj,
                          double tol = s_normalizationTolerance) {
  double sumOfWeights = 0.0;

  for (auto it = cmps.begin(); it != cmps.end(); ++it) {
    sumOfWeights += proj(*it);
  }

  return std::abs(sumOfWeights - 1.0) < tol;
}

template <typename component_range_t, typename projector_t>
void normalizeWeights(component_range_t &cmps, const projector_t &proj) {
  double sumOfWeights = 0.0;

  // we need decltype(auto) here to support proxy-types with reference
  // semantics, otherwise there is a `cannot bind ... to ...` error
  for (auto it = cmps.begin(); it != cmps.end(); ++it) {
    decltype(auto) cmp = *it;
    assert(std::isfinite(proj(cmp)) && "weight not finite in normalization");
    sumOfWeights += proj(cmp);
  }

  assert(sumOfWeights > 0 && "sum of weights is not > 0");

  for (auto it = cmps.begin(); it != cmps.end(); ++it) {
    decltype(auto) cmp = *it;
    proj(cmp) /= sumOfWeights;
  }
}

// A class that prints information about the state on construction and
// destruction, it also contains some assertions in the constructor and
// destructor. It can be removed without change of behaviour, since it only
// holds const references
template <typename propagator_state_t, typename stepper_t, typename navigator_t>
class ScopedGsfInfoPrinterAndChecker {
  const propagator_state_t &m_state;
  const stepper_t &m_stepper;
  const navigator_t &m_navigator;
  double m_p_initial;
  const Logger &m_logger;

  const Logger &logger() const { return m_logger; }

  void print_component_stats() const {
    std::size_t i = 0;
    for (auto cmp : m_stepper.constComponentIterable(m_state.stepping)) {
      auto getVector = [&](auto idx) {
        return cmp.pars().template segment<3>(idx).transpose();
      };
      ACTS_VERBOSE("  #" << i++ << " pos: " << getVector(eFreePos0) << ", dir: "
                         << getVector(eFreeDir0) << ", weight: " << cmp.weight()
                         << ", status: " << cmp.status()
                         << ", qop: " << cmp.pars()[eFreeQOverP]
                         << ", det(cov): " << cmp.cov().determinant());
    }
  }

  void checks(bool onStart) const {
    const auto cmps = m_stepper.constComponentIterable(m_state.stepping);
    [[maybe_unused]] const bool allFinite =
        std::all_of(cmps.begin(), cmps.end(),
                    [](auto cmp) { return std::isfinite(cmp.weight()); });
    [[maybe_unused]] const bool allNormalized = detail::weightsAreNormalized(
        cmps, [](const auto &cmp) { return cmp.weight(); });
    [[maybe_unused]] const bool zeroComponents =
        m_stepper.numberComponents(m_state.stepping) == 0;

    if (onStart) {
      assert(!zeroComponents && "no cmps at the start");
      assert(allFinite && "weights not finite at the start");
      assert(allNormalized && "not normalized at the start");
    } else {
      assert(!zeroComponents && "no cmps at the end");
      assert(allFinite && "weights not finite at the end");
      assert(allNormalized && "not normalized at the end");
    }
  }

 public:
  ScopedGsfInfoPrinterAndChecker(const propagator_state_t &state,
                                 const stepper_t &stepper,
                                 const navigator_t &navigator,
                                 const Logger &logger)
      : m_state(state),
        m_stepper(stepper),
        m_navigator(navigator),
        m_p_initial(stepper.absoluteMomentum(state.stepping)),
        m_logger{logger} {
    // Some initial printing
    checks(true);
    ACTS_VERBOSE("Gsf step "
                 << state.stepping.steps << " at mean position "
                 << stepper.position(state.stepping).transpose()
                 << " with direction "
                 << stepper.direction(state.stepping).transpose()
                 << " and momentum " << stepper.absoluteMomentum(state.stepping)
                 << " and charge " << stepper.charge(state.stepping));
    ACTS_VERBOSE("Propagation is in " << state.options.direction << " mode");
    print_component_stats();
  }

  ~ScopedGsfInfoPrinterAndChecker() {
    if (m_navigator.currentSurface(m_state.navigation)) {
      const auto p_final = m_stepper.absoluteMomentum(m_state.stepping);
      ACTS_VERBOSE("Component status at end of step:");
      print_component_stats();
      ACTS_VERBOSE("Delta Momentum = " << std::setprecision(5)
                                       << p_final - m_p_initial);
    }
    checks(false);
  }
};

ActsScalar calculateDeterminant(
    const double *fullCalibratedCovariance,
    TrackStateTraits<MultiTrajectoryTraits::MeasurementSizeMax,
                     true>::Covariance predictedCovariance,
    TrackStateTraits<MultiTrajectoryTraits::MeasurementSizeMax, true>::Projector
        projector,
    unsigned int calibratedSize);

/// Reweight the components according to `R. Frühwirth, "Track fitting
/// with non-Gaussian noise"`. See also the implementation in Athena at
/// PosteriorWeightsCalculator.cxx
/// @note The weights are not renormalized!
template <typename traj_t>
void computePosteriorWeights(
    const traj_t &mt, const std::vector<MultiTrajectoryTraits::IndexType> &tips,
    std::map<MultiTrajectoryTraits::IndexType, double> &weights) {
  // Helper Function to compute detR

  // Find minChi2, this can be used to factor some things later in the
  // exponentiation
  const auto minChi2 =
      mt.getTrackState(*std::min_element(tips.begin(), tips.end(),
                                         [&](const auto &a, const auto &b) {
                                           return mt.getTrackState(a).chi2() <
                                                  mt.getTrackState(b).chi2();
                                         }))
          .chi2();

  // Loop over the tips and compute new weights
  for (auto tip : tips) {
    const auto state = mt.getTrackState(tip);
    const double chi2 = state.chi2() - minChi2;
    const double detR = calculateDeterminant(
        // This abuses an incorrectly sized vector / matrix to access the
        // data pointer! This works (don't use the matrix as is!), but be
        // careful!
        state
            .template calibratedCovariance<
                MultiTrajectoryTraits::MeasurementSizeMax>()
            .data(),
        state.predictedCovariance(), state.projector(), state.calibratedSize());

    const auto factor = std::sqrt(1. / detR) * safeExp(-0.5 * chi2);

    // If something is not finite here, just leave the weight as it is
    if (std::isfinite(factor)) {
      weights.at(tip) *= factor;
    }
  }
}

/// Enumeration type to allow templating on the state we want to project on with
/// a MultiTrajectory
enum class StatesType { ePredicted, eFiltered, eSmoothed };

inline std::ostream &operator<<(std::ostream &os, StatesType type) {
  constexpr static std::array names = {"predicted", "filtered", "smoothed"};
  os << names[static_cast<int>(type)];
  return os;
}

/// @brief Projector type which maps a MultiTrajectory-Index to a tuple of
/// [weight, parameters, covariance]. Therefore, it contains a MultiTrajectory
/// and for now a std::map for the weights
template <StatesType type, typename traj_t>
struct MultiTrajectoryProjector {
  const traj_t &mt;
  const std::map<MultiTrajectoryTraits::IndexType, double> &weights;

  auto operator()(MultiTrajectoryTraits::IndexType idx) const {
    const auto proxy = mt.getTrackState(idx);
    switch (type) {
      case StatesType::ePredicted:
        return std::make_tuple(weights.at(idx), proxy.predicted(),
                               proxy.predictedCovariance());
      case StatesType::eFiltered:
        return std::make_tuple(weights.at(idx), proxy.filtered(),
                               proxy.filteredCovariance());
      case StatesType::eSmoothed:
        return std::make_tuple(weights.at(idx), proxy.smoothed(),
                               proxy.smoothedCovariance());
      default:
        throw std::invalid_argument(
            "Incorrect StatesType, should be ePredicted"
            ", eFiltered, or eSmoothed.");
    }
  }
};

/// Small Helper class that allows to carry a temporary value until we decide to
/// update the actual value. The temporary value is deliberately only accessible
/// with a mutable reference
template <typename T>
class Updatable {
  T m_tmp{};
  T m_val{};

 public:
  Updatable() : m_tmp(0), m_val(0) {}

  T &tmp() { return m_tmp; }
  void update() { m_val = m_tmp; }

  const T &val() const { return m_val; }
};

}  // namespace detail
}  // namespace Acts
