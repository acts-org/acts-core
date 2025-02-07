// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/ExaTrkX/Stages.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>

#include <torch/script.h>

namespace Acts {

class CudaTrackBuilding final : public Acts::TrackBuildingBase {
 public:
  struct Config {
    // nothing yet
  };

  CudaTrackBuilding(const Config &cfg, std::unique_ptr<const Logger> logger)
      : m_cfg(cfg),
        m_logger(std::move(logger)),
        m_device(torch::Device(torch::kCUDA)) {}

  std::vector<std::vector<int>> operator()(
      std::any nodes, std::any edges, std::any edge_weights,
      std::vector<int> &spacepointIDs,
      const ExecutionContext &execContext = {}) override;
  torch::Device device() const override { return m_device; };

 private:
  Config m_cfg;
  std::unique_ptr<const Acts::Logger> m_logger;
  torch::Device m_device;
  const auto &logger() const { return *m_logger; }
};

}  // namespace Acts
