// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Utilities/OptionsFwd.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace Acts {
class TrackingGeometry;
class IMaterialDecorator;
}  // namespace Acts

namespace ActsExamples {
class IContextDecorator;
}  // namespace ActsExamples

namespace ActsExamples {
class IBaseDetector {
 public:
  using ContextDecorators =
      std::vector<std::shared_ptr<ActsExamples::IContextDecorator>>;
  using TrackingGeometryPtr = std::shared_ptr<const Acts::TrackingGeometry>;

  virtual ~IBaseDetector() = default;
  virtual void addOptions(
      boost::program_options::options_description& opt) const = 0;

  virtual std::pair<TrackingGeometryPtr, ContextDecorators> finalize(
      const boost::program_options::variables_map& vm,
      std::shared_ptr<const Acts::IMaterialDecorator> mdecorator) = 0;
};
}  // namespace ActsExamples
