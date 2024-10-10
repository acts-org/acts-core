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

namespace Acts {

class CugraphTrackBuilding final : public Acts::TrackBuildingBase {
 public:
  CugraphTrackBuilding(std::unique_ptr<const Logger> logger)
      : m_logger(std::move(logger)), m_device(torch::Device(torch::kCPU)) {}

  std::vector<std::vector<int>> operator()(
      std::any nodes, std::any edges, std::any edge_weights,
      std::vector<int> &spacepointIDs,
      torch::Device device = torch::Device(torch::kCPU)) override;
  torch::Device device() const override { return m_device; };

 private:
  std::unique_ptr<const Acts::Logger> m_logger;
  torch::Device m_device;
  const auto &logger() const { return *m_logger; }
};

}  // namespace Acts
