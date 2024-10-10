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

#include <any>
#include <chrono>
#include <functional>
#include <memory>
#include <vector>

#include <boost/container/small_vector.hpp>

namespace Acts {

struct ExaTrkXTiming {
  using Duration = std::chrono::duration<float, std::milli>;

  Duration graphBuildingTime = Duration{0.f};
  boost::container::small_vector<Duration, 3> classifierTimes;
  Duration trackBuildingTime = Duration{0.f};
};

class ExaTrkXHook {
 public:
  virtual ~ExaTrkXHook() {}
  virtual void operator()(const std::any & /*nodes*/,
                          const std::any & /*edges*/,
                          const std::any & /*weights*/) const {};
};

class ExaTrkXPipeline {
 public:
  ExaTrkXPipeline(
      std::shared_ptr<GraphConstructionBase> graphConstructor,
      std::vector<std::shared_ptr<EdgeClassificationBase>> edgeClassifiers,
      std::shared_ptr<TrackBuildingBase> trackBuilder,
      std::unique_ptr<const Acts::Logger> logger);

  std::vector<std::vector<int>> run(std::vector<float> &features,
                                    std::vector<int> &spacepointIDs,
                                    const ExaTrkXHook &hook = {},
                                    ExaTrkXTiming *timing = nullptr) const;

 private:
  std::unique_ptr<const Acts::Logger> m_logger;

  std::shared_ptr<GraphConstructionBase> m_graphConstructor;
  std::vector<std::shared_ptr<EdgeClassificationBase>> m_edgeClassifiers;
  std::shared_ptr<TrackBuildingBase> m_trackBuilder;

  const Logger &logger() const { return *m_logger; }
};

}  // namespace Acts
