// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/ExaTrkX/ExaTrkXPipeline.hpp"
#include "Acts/Plugins/ExaTrkX/detail/CantorEdge.hpp"
#include "Acts/Utilities/Logger.hpp"

namespace Acts {

class TorchGraphStoreHook : public ExaTrkXHook {
 public:
  using Graph = std::pair<std::vector<std::int64_t>, std::vector<float>>;

 private:
  std::unique_ptr<Graph> m_storedGraph;

 public:
  TorchGraphStoreHook();
  ~TorchGraphStoreHook() override {}

  void operator()(const std::any &, const std::any &edges,
                  const std::any &weights) const override;

  const Graph &storedGraph() const { return *m_storedGraph; }
};

}  // namespace Acts
