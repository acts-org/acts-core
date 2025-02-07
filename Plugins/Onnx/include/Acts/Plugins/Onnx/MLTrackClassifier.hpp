// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Onnx/OnnxRuntimeBase.hpp"

#include <vector>

namespace Acts {

// Specialized class that labels tracks as good/duplicate/fake using a
// deep neural network.
class MLTrackClassifier : public OnnxRuntimeBase {
  using OnnxRuntimeBase::OnnxRuntimeBase;

 public:
  /// @brief The labels for track quality
  enum class TrackLabels { eGood, eDuplicate, eFake };

  /// @brief Predict the track label
  ///
  /// @param inputFeatures The vector of input features for the trajectory to be
  /// classified
  /// @param decisionThreshProb The probability threshold used to predict the
  /// track label
  ///
  /// @return The predicted track label of the trajectory
  TrackLabels predictTrackLabel(std::vector<float>& inputFeatures,
                                double decisionThreshProb) const;

  /// @brief Check if the predicted track label is 'duplicate'
  ///
  /// @param inputFeatures The vector of input features for the trajectory to be
  /// classified
  /// @param decisionThreshProb The probability threshold used to predict the
  /// track label
  ///
  /// @return If the predicted track label is 'duplicate'
  bool isDuplicate(std::vector<float>& inputFeatures,
                   double decisionThreshProb) const;
};

}  // namespace Acts
