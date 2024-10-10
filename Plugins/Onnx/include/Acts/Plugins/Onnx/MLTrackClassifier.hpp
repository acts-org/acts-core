// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
