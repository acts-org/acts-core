// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/ExaTrkX/ExaTrkXPipeline.hpp"
#include "Acts/Plugins/ExaTrkX/detail/CantorEdge.hpp"
#include "Acts/Utilities/Logger.hpp"

namespace Acts {

class TorchTruthGraphMetricsHook : public ExaTrkXHook {
  std::unique_ptr<const Logger> m_logger;
  std::vector<detail::CantorEdge<std::int64_t>> m_truthGraphCantor;

  const Logger &logger() const { return *m_logger; }

 public:
  TorchTruthGraphMetricsHook(const std::vector<std::int64_t> &truthGraph,
                             std::unique_ptr<const Acts::Logger> l);
  ~TorchTruthGraphMetricsHook() override {}

  void operator()(const std::any &, const std::any &edges,
                  const std::any &) const override;
};

}  // namespace Acts
