// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
