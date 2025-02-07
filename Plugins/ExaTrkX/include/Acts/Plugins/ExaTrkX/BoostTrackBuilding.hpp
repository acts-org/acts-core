// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/ExaTrkX/Stages.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>

#include <torch/script.h>

namespace Acts {

class BoostTrackBuilding final : public Acts::TrackBuildingBase {
 public:
  BoostTrackBuilding(std::unique_ptr<const Logger> logger)
      : m_logger(std::move(logger)), m_device(torch::Device(torch::kCPU)) {}

  std::vector<std::vector<int>> operator()(
      std::any nodes, std::any edges, std::any edge_weights,
      std::vector<int> &spacepointIDs,
      const ExecutionContext &execContext = {}) override;
  torch::Device device() const override { return m_device; };

 private:
  std::unique_ptr<const Acts::Logger> m_logger;
  torch::Device m_device;
  const auto &logger() const { return *m_logger; }
};

}  // namespace Acts
