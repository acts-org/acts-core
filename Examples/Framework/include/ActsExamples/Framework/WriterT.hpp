// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IWriter.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include <Acts/Utilities/Logger.hpp>

#include <limits>
#include <memory>
#include <string>

namespace ActsExamples {

/// NaN values for TTree variables
constexpr double NaNdouble = std::numeric_limits<double>::quiet_NaN();
constexpr float NaNfloat = std::numeric_limits<float>::quiet_NaN();
constexpr float NaNint = std::numeric_limits<int>::quiet_NaN();

/// A helper class for users to implement framework writers.
///
/// @note This is not an additional interface class and should not be used as
///       such, e.g. as a constrained `IWriter` substitute. This class should
///       only be used as the base class for a concrete writer implementation.
///
/// @tparam T The object type read from the event store
///
/// This class can be used when a writer reads a single object from the event
/// store and writes it to file. Reading from the event store and casting
/// to the specified type is done automatically and the user only needs to
/// implement the type-specific write method.
///
/// Default no-op implementations for `initialize` and `finalize` are provided
/// but can be overridden by the user.
template <typename write_data_t>
class WriterT : public IWriter {
 public:
  /// @param objectName The object that should be read from the event store
  /// @param writerName The name of the writer, e.g. for logging output
  /// @param level The internal log level
  WriterT(std::string objectName, std::string writerName,
          Acts::Logging::Level level);

  /// Provide the name of the writer
  std::string name() const override;

  /// Read the object and call the type-specific member function.
  ProcessCode write(const AlgorithmContext& context) override;

  /// No-op default implementation.
  ProcessCode finalize() override;

 protected:
  /// Type-specific write function implementation
  /// this method is implemented in the user implementation
  /// @param [in] context is the algorithm context that guarantees event
  ///        consistency
  /// @tparam [in] is the templated collection to be written
  virtual ProcessCode writeT(const AlgorithmContext& context,
                             const write_data_t& t) = 0;

  const Acts::Logger& logger() const { return *m_logger; }

 private:
  std::string m_objectName;
  std::string m_writerName;
  std::unique_ptr<const Acts::Logger> m_logger;

  ReadDataHandle<write_data_t> m_inputHandle{this, "InputHandle"};
};

}  // namespace ActsExamples

template <typename write_data_t>
ActsExamples::WriterT<write_data_t>::WriterT(std::string objectName,
                                             std::string writerName,
                                             Acts::Logging::Level level)
    : m_objectName(std::move(objectName)),
      m_writerName(std::move(writerName)),
      m_logger(Acts::getDefaultLogger(m_writerName, level)) {
  if (m_objectName.empty()) {
    throw std::invalid_argument("Missing input collection");
  } else if (m_writerName.empty()) {
    throw std::invalid_argument("Missing writer name");
  }

  m_inputHandle.initialize(m_objectName);
}

template <typename write_data_t>
inline std::string ActsExamples::WriterT<write_data_t>::name() const {
  return m_writerName;
}

template <typename write_data_t>
inline ActsExamples::ProcessCode
ActsExamples::WriterT<write_data_t>::finalize() {
  return ProcessCode::SUCCESS;
}

template <typename write_data_t>
inline ActsExamples::ProcessCode ActsExamples::WriterT<write_data_t>::write(
    const AlgorithmContext& context) {
  return writeT(context, m_inputHandle(context));
}
