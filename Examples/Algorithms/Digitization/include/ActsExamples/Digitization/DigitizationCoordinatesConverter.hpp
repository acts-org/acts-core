// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/Digitization/DigitizationAlgorithm.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"

namespace ActsExamples {

/// Utility that converts hit coordinates.
class DigitizationCoordinatesConverter final {
 public:
  /// Construct the converter
  ///
  /// @param config is the configuration
  explicit DigitizationCoordinatesConverter(
      DigitizationAlgorithm::Config config);

  /// Get const access to the config
  const DigitizationAlgorithm::Config& config() const { return m_cfg; }

  /// Convert the hit coordinates to the local frame.
  std::tuple<double, double> globalToLocal(std::uint64_t moduleId, double x,
                                           double y, double z) const;

  /// Convert the hit coordinates to the global frame.
  std::tuple<double, double, double> localToGlobal(std::uint64_t moduleId,
                                                   double x, double y) const;

 private:
  /// Configuration
  DigitizationAlgorithm::Config m_cfg;
};

}  // namespace ActsExamples
