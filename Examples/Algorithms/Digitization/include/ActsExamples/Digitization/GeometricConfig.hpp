// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/Result.hpp"
#include "ActsExamples/Digitization/Smearers.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsFatras/Digitization/UncorrelatedHitSmearer.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

namespace ActsExamples {

/// Configuration struct for geometric digitization
///
/// If this is defined, then the geometric digitization
/// will create clusters with cells.
/// The BinUtility defines the segmentation and which parameters
/// are defined by this.
///
struct GeometricConfig {
  // The dimensions of the measurement
  std::vector<Acts::BoundIndices> indices = {};

  // The (multidimensional) binning definition for the segmentation of the
  // sensor
  Acts::BinUtility segmentation;

  // The thickness of the sensor
  double thickness = 0.;

  /// The charge smearer
  ActsFatras::SingleParameterSmearFunction<ActsExamples::RandomEngine>
      chargeSmearer = Digitization::Exact(0);

  // The threshold below a cell activation is ignored
  double threshold = 0.;

  // Whether to assume digital readout (activation is either 0 or 1)
  bool digital = false;

  // Flag as strip
  bool strip = false;

  /// The variances for this digitization
  std::map<Acts::BoundIndices, std::vector<double>> varianceMap = {};

  /// Charge generation (configurable via the chargeSmearer)
  double charge(double path, RandomEngine &rng) const {
    if (!chargeSmearer) {
      return path;
    }
    auto res = chargeSmearer(path, rng);
    if (res.ok()) {
      return std::max(0.0, res->first);
    } else {
      throw std::runtime_error(res.error().message());
    }
  }

  /// This generates the variances for a given cluster
  ///
  /// @note either the variances are directly taken from a pre-read
  /// variance map, or they are generated from the pitch size
  ///
  /// @param csizes is the cluster size in the different dimensions
  /// @param cmins is the cluster minimum in the different dimensions
  ///
  /// @return a vector of variances for the cluster
  std::vector<double> variances(const std::array<std::size_t, 2u> &csizes,
                                const std::array<std::size_t, 2u> &cmins) const;

  /// Drift generation (currently not implemented)
  /// Takes as an argument the position, and a random engine
  ///  @return drift direction in local 3D coordinates
  Acts::Vector3 drift(const Acts::Vector3 & /*position*/,
                      RandomEngine & /*rng*/) const {
    return Acts::Vector3(0., 0., 0.);
  };
};

}  // namespace ActsExamples
