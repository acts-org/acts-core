// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/ExaTrkX/Stages.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>

#include <torch/script.h>

namespace torch::jit {
class Module;
}

namespace c10 {
enum class DeviceType : std::int8_t;
}

namespace Acts {

class TorchEdgeClassifier final : public Acts::EdgeClassificationBase {
 public:
  struct Config {
    std::string modelPath;
    std::vector<int> selectedFeatures = {};
    float cut = 0.21;
    int nChunks = 1;  // NOTE for GNN use 1
    bool undirected = false;
    int deviceID = 0;
    bool useEdgeFeatures = false;
  };

  TorchEdgeClassifier(const Config &cfg, std::unique_ptr<const Logger> logger);
  ~TorchEdgeClassifier();

  std::tuple<std::any, std::any, std::any, std::any> operator()(
      std::any nodeFeatures, std::any edgeIndex, std::any edgeFeatures = {},
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
