// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <memory>
#include <string>

namespace ActsExamples {
struct AlgorithmContext;

/// A mock service that changes the magnetic field scale for each event.
///
/// The magnetic field scaling changes as some function of the event number.
class ScalableBFieldService : public IContextDecorator {
 public:
  struct Config {
    /// Scaling factor. Unit value means the magnetic field is left unchanged.
    Acts::ActsScalar scalor = 1.25;
  };

  /// Construct the magnetic field service.
  ///
  /// @param cfg Configuration struct
  /// @param lvl Logging level
  ScalableBFieldService(const Config& cfg, Acts::Logging::Level lvl);

  /// The service name.
  const std::string& name() const override;

  /// Update the magnetic field context.
  ///
  /// @param ctx The per-event context
  ProcessCode decorate(AlgorithmContext& ctx) override;

 private:
  Config m_cfg;
  std::unique_ptr<const Acts::Logger> m_logger;

  const Acts::Logger& logger() const { return *m_logger; }
};

}  // namespace ActsExamples
