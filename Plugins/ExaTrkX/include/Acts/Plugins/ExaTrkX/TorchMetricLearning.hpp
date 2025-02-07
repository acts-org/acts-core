// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
    std::vector<int> selectedFeatures = {};
    int embeddingDim = 8;
    float rVal = 1.6;
    int knnVal = 500;
    bool shuffleDirections = false;
    int deviceID = 0;  // default is the first GPU if available

    // For edge features
    float phiScale = 3.141592654;
  };

  TorchMetricLearning(const Config &cfg, std::unique_ptr<const Logger> logger);
  ~TorchMetricLearning();

  std::tuple<std::any, std::any, std::any> operator()(
      std::vector<float> &inputValues, std::size_t numNodes,
      const std::vector<std::uint64_t> &moduleIds,
      const ExecutionContext &execContext = {}) override;

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
