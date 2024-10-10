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

class OnnxEdgeClassifier final : public Acts::EdgeClassificationBase {
 public:
  struct Config {
    std::string modelPath;
    float cut = 0.21;
  };

  OnnxEdgeClassifier(const Config &cfg, std::unique_ptr<const Logger> logger);
  ~OnnxEdgeClassifier();

  std::tuple<std::any, std::any, std::any> operator()(
      std::any nodes, std::any edges,
      torch::Device device = torch::Device(torch::kCPU)) override;

  Config config() const { return m_cfg; }
  torch::Device device() const override { return m_device; };

 private:
  std::unique_ptr<const Acts::Logger> m_logger;
  const auto &logger() const { return *m_logger; }

  Config m_cfg;
  torch::Device m_device;
  std::unique_ptr<Ort::Env> m_env;
  std::unique_ptr<Ort::Session> m_model;

  std::string m_inputNameNodes;
  std::string m_inputNameEdges;
  std::string m_outputNameScores;
};

}  // namespace Acts
