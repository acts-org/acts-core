// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/SequenceElement.hpp"

#include <string>

namespace ActsExamples {

/// Event data writer interface.
///
/// Get data from the event store and write it to disk. The writer can have
/// internal state and implementations are responsible to handle concurrent
/// calls.
class IWriter : public SequenceElement {
 public:
  /// Write data from one event.
  virtual ProcessCode write(const AlgorithmContext& context) = 0;

  /// Internal execute method forwards to the write method as mutable
  /// @param context The algorithm context
  ProcessCode internalExecute(const AlgorithmContext& context) final {
    return write(context);
  }

  /// Fulfil the algorithm interface
  ProcessCode initialize() override { return ProcessCode::SUCCESS; }
};

}  // namespace ActsExamples
