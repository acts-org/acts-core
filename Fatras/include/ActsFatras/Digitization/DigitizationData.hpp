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

#include <unordered_set>
#include <utility>

namespace ActsFatras {

/// A single cell definition: index, cell central value
using Cell = std::pair<unsigned int, Acts::ActsScalar>;

/// A channel definition: Cell identification, readout word, links
///
/// @tparam signal_t Type of the signal, requires += operator
/// @tparam kSize Number of channel coordinates
template <typename signal_t, std::size_t kSize>
struct Channel {
  /// The cell identification in sizeof..(kParameters) dimensions
  std::array<Cell, kSize> cellId;
  /// The signal value, as complex as possible,
  /// but need += operator and double() cast for the weight
  signal_t value = 0;
  /// The potential (truth) links
  std::unordered_set<unsigned int> links = {};

  /// Channel constructor
  ///
  /// @param cellId_ The Cell idenficiation and position
  /// @param value_ The Cell value
  /// @param links_ The (optional) links to e.g. truth indices
  Channel(std::array<Cell, kSize> cellId_, signal_t value_,
          std::unordered_set<unsigned int> links_ = {})
      : cellId(cellId_), value(value_), links(std::move(links_)) {}

  Channel() = delete;
};

/// A (simulated) cluster with its constituents.
///
/// @tparam signal_t Type of the signal carried, see above
/// @tparam kSize Number of cluster coordinates
template <typename signal_t, std::size_t kSize>
struct Cluster {
  using Scalar = Acts::ActsScalar;
  using ParametersVector = Acts::ActsVector<kSize>;
  using CovarianceMatrix = Acts::ActsSquareMatrix<kSize>;

  /// Measured parameters.
  ParametersVector parameters = ParametersVector::Zero();
  /// Measurement covariance.
  CovarianceMatrix covariance = CovarianceMatrix::Zero();
  /// The resulting cluster size along each channel dimension.
  std::array<unsigned int, kSize> clusterSize;
  /// The constituating signal channels.
  std::vector<Channel<signal_t, kSize>> channels;

  /// Cluster constructor
  ///
  /// @param p Measured parameters
  /// @param c Measurement covariance
  /// @param cSize The cluster size definition
  /// @param cChannels The channel
  template <typename parameters_t, typename covariance_t>
  Cluster(const Eigen::MatrixBase<parameters_t>& p,
          const Eigen::MatrixBase<covariance_t>& c,
          std::array<unsigned int, kSize> cSize,
          std::vector<Channel<signal_t, kSize>> cChannels)
      : parameters(p), covariance(c), clusterSize(cSize), channels(cChannels) {}

  Cluster() = delete;
};

}  // namespace ActsFatras
