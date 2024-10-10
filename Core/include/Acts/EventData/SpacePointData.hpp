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

/// @class SpacePointData
/// This class contains auxiliary data associated to the
/// external space points provided by the customers
/// These variables are used internally by the seeding algorithm, that
/// reads them
/// The variables collected here are also dynamic variables only present
/// for strip space points
class SpacePointData {
 public:
  /// @brief Default constructor
  SpacePointData() = default;

  /// No copies
  SpacePointData(const SpacePointData& other) = delete;
  SpacePointData& operator=(const SpacePointData& other) = delete;

  /// @brief Move operations
  SpacePointData(SpacePointData&& other) noexcept = default;
  SpacePointData& operator=(SpacePointData&& other) noexcept = default;

  /// @brief Destructor
  ~SpacePointData() = default;

  /// @brief Getters
  float x(const std::size_t idx) const;
  float y(const std::size_t idx) const;
  float z(const std::size_t idx) const;
  float radius(const std::size_t idx) const;
  float phi(const std::size_t idx) const;
  float varianceZ(const std::size_t idx) const;
  float varianceR(const std::size_t idx) const;

  /// @brief Setters
  void setX(const std::size_t idx, const float value);
  void setY(const std::size_t idx, const float value);
  void setZ(const std::size_t idx, const float value);
  void setRadius(const std::size_t idx, const float value);
  void setPhi(const std::size_t idx, const float value);
  void setVarianceZ(const std::size_t idx, const float value);
  void setVarianceR(const std::size_t idx, const float value);

  /// @brief Resize vectors
  void resize(const std::size_t n, bool resizeDynamic = false);

  /// @brief clear vectors
  void clear();

  ///
  bool hasDynamicVariable() const;

  const Acts::Vector3& topStripVector(const std::size_t idx) const;
  const Acts::Vector3& bottomStripVector(const std::size_t idx) const;
  const Acts::Vector3& stripCenterDistance(const std::size_t idx) const;
  const Acts::Vector3& topStripCenterPosition(const std::size_t idx) const;

  void setTopStripVector(const std::size_t idx, const Acts::Vector3& value);
  void setBottomStripVector(const std::size_t idx, const Acts::Vector3& value);
  void setStripCenterDistance(const std::size_t idx,
                              const Acts::Vector3& value);
  void setTopStripCenterPosition(const std::size_t idx,
                                 const Acts::Vector3& value);

 private:
  /// base variables
  std::vector<float> m_x{};
  std::vector<float> m_y{};
  std::vector<float> m_z{};
  std::vector<float> m_radius{};
  std::vector<float> m_phi{};
  std::vector<float> m_varianceR{};
  std::vector<float> m_varianceZ{};

  /// dynamic variables
  std::vector<Acts::Vector3> m_topStripVector{};
  std::vector<Acts::Vector3> m_bottomStripVector{};
  std::vector<Acts::Vector3> m_stripCenterDistance{};
  std::vector<Acts::Vector3> m_topStripCenterPosition{};
};

}  // namespace Acts

#include "Acts/EventData/SpacePointData.ipp"
