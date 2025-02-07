// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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

class OnnxEdgeClassifier final : public Acts::EdgeClassificationBase {
 public:
  struct Config {
    std::string modelPath;
    float cut = 0.21;
  };

  OnnxEdgeClassifier(const Config &cfg, std::unique_ptr<const Logger> logger);
  ~OnnxEdgeClassifier();

  std::tuple<std::any, std::any, std::any, std::any> operator()(
      std::any nodeFeatures, std::any edgeIndex, std::any edgeFeatures = {},
      const ExecutionContext &execContext = {}) override;

  Config config() const { return m_cfg; }
  torch::Device device() const override { return m_device; };

 private:
  std::unique_ptr<const Acts::Logger> m_logger;
  const auto &logger() const { return *m_logger; }

  Config m_cfg;
  torch::Device m_device;
  std::unique_ptr<Ort::Env> m_env;
  std::unique_ptr<Ort::Session> m_model;

  std::vector<std::string> m_inputNames;
  std::string m_outputName;
};

}  // namespace Acts
