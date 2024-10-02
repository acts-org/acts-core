// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/ExaTrkX/Stages.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>

namespace torch::jit {
class Module;
}

namespace c10 {
enum class DeviceType : std::int8_t;
}

namespace Acts {

class TorchMetricLearning final : public Acts::GraphConstructionBase {
 public:
  struct Config {
    std::string modelPath;
    int numFeatures = 3;
    int embeddingDim = 8;
    float rVal = 1.6;
    int knnVal = 500;
    bool shuffleDirections = false;
    int deviceID = 0;  // default is the first GPU if available
  };

  TorchMetricLearning(const Config &cfg, std::unique_ptr<const Logger> logger);
  ~TorchMetricLearning();

  std::tuple<std::any, std::any> operator()(
      std::vector<float> &inputValues, std::size_t numNodes,
      torch::Device device = torch::Device(torch::kCPU)) override;

  Config config() const { return m_cfg; }
  torch::Device device() const override { return m_device; };

 private:
  std::unique_ptr<const Acts::Logger> m_logger;
  const auto &logger() const { return *m_logger; }

  Config m_cfg;
  c10::DeviceType m_deviceType;
  torch::Device m_device;
  std::unique_ptr<torch::jit::Module> m_model;
};

}  // namespace Acts
