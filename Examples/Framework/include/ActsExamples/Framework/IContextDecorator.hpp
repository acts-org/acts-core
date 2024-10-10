// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <string>

namespace ActsExamples {

/// @brief Decorator for the AlgorithmContext with
/// additional event specific information
///
class IContextDecorator {
 public:
  /// Virtual destructor
  virtual ~IContextDecorator() = default;

  /// @brief decorates (adds, modifies) the AlgorithmContext
  /// with additional event specific information, it is attached
  /// to the Sequencer
  ///
  /// @note If decorators depend on each other, they have to be
  /// added in order.
  ///
  /// @param context the bare (or at least non-const) Event context
  virtual ProcessCode decorate(AlgorithmContext& context) = 0;

  /// @brief decorator name() for screen output
  virtual const std::string& name() const = 0;
};

}  // namespace ActsExamples
