// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/HashedString.hpp"

#include <any>
#include <limits>
#include <vector>

namespace Acts {

/// @class SpacePointMutableData
/// This class contains mutable data associated to the
/// external space points provided by the customers
/// These variables are used mainly internally by the seeding algorithm, that
/// reads and updates them for seed selection purposes.
/// The quality is also accessed after the seeding for an additional selection
/// round on the candidates
class SpacePointMutableData {
 public:
  /// @brief Default constructor
  SpacePointMutableData() = default;

  /// No copies
  SpacePointMutableData(const SpacePointMutableData& other) = delete;
  SpacePointMutableData& operator=(const SpacePointMutableData& other) = delete;

  /// @brief Move operations
  SpacePointMutableData(SpacePointMutableData&& other) noexcept = default;
  SpacePointMutableData& operator=(SpacePointMutableData&& other) noexcept =
      default;

  /// @brief Destructor
  ~SpacePointMutableData() = default;

  /// @brief Getters
  float quality(const std::size_t idx) const;
  float deltaR(const std::size_t idx) const;

  /// @brief Setters
  void setQuality(const std::size_t idx, const float value);
  void setDeltaR(const std::size_t idx, const float value);

  /// @brief Resize vectors
  void resize(const std::size_t n);

  /// @brief clear vectors
  void clear();

 private:
  /// Variables
  std::vector<float> m_quality{};
  std::vector<float> m_deltaR{};
};

}  // namespace Acts

#include "Acts/EventData/SpacePointMutableData.ipp"
