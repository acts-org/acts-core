// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
                                    const std::vector<std::uint64_t> &moduleIds,
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
