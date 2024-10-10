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

#include <torch/script.h>

namespace Ort {
class Env;
class Session;
class Value;
}  // namespace Ort

namespace Acts {

class OnnxMetricLearning final : public Acts::GraphConstructionBase {
 public:
  struct Config {
    std::string modelPath;
    int spacepointFeatures = 3;
    int embeddingDim = 8;
    float rVal = 1.6;
    int knnVal = 500;
  };

  OnnxMetricLearning(const Config& cfg, std::unique_ptr<const Logger> logger);
  ~OnnxMetricLearning();

  std::tuple<std::any, std::any> operator()(
      std::vector<float>& inputValues, std::size_t numNodes,
      torch::Device device = torch::Device(torch::kCPU)) override;

  Config config() const { return m_cfg; }
  torch::Device device() const override { return m_device; };

 private:
  void buildEdgesWrapper(std::vector<float>& embedFeatures,
                         std::vector<std::int64_t>& edgeList,
                         std::int64_t numSpacepoints,
                         const Logger& logger) const;

  std::unique_ptr<const Acts::Logger> m_logger;
  const auto& logger() const { return *m_logger; }

  Config m_cfg;
  torch::Device m_device;
  std::unique_ptr<Ort::Env> m_env;
  std::unique_ptr<Ort::Session> m_model;
};

}  // namespace Acts
