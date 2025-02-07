// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/SequenceElement.hpp"
#include <Acts/Utilities/Logger.hpp>

#include <memory>
#include <string>

namespace ActsExamples {
struct AlgorithmContext;

/// Event processing algorithm interface.
///
/// This class provides default implementations for most interface methods and
/// and adds a default logger that can be used directly in subclasses.
/// Algorithm implementations only need to implement the `execute` method.
class IAlgorithm : public SequenceElement {
 public:
  /// Constructor
  ///
  /// @name The algorithm name
  /// @level The logging level for this algorithm
  IAlgorithm(std::string name,
             Acts::Logging::Level level = Acts::Logging::INFO);

  /// The algorithm name.
  std::string name() const override;

  /// Execute the algorithm for one event.
  ///
  /// This function must be implemented by subclasses.
  virtual ProcessCode execute(const AlgorithmContext& context) const = 0;

  /// Internal execute method forwards to the algorithm execute method as const
  /// @param context The algorithm context
  ProcessCode internalExecute(const AlgorithmContext& context) final {
    return execute(context);
  }

  /// Initialize the algorithm
  ProcessCode initialize() override { return ProcessCode::SUCCESS; }
  /// Finalize the algorithm
  ProcessCode finalize() override { return ProcessCode::SUCCESS; }

 protected:
  const Acts::Logger& logger() const { return *m_logger; }

 private:
  std::string m_name;
  std::unique_ptr<const Acts::Logger> m_logger;
};
}  // namespace ActsExamples
