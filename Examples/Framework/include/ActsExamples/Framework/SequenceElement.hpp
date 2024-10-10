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
#include <vector>

namespace ActsExamples {

class DataHandleBase;
struct AlgorithmContext;

/// Event processing interface.
///
class SequenceElement {
 public:
  virtual ~SequenceElement() = default;

  /// The algorithm name.
  virtual std::string name() const = 0;

  /// Initialize the algorithm
  virtual ProcessCode initialize() = 0;

  /// Finalize the algorithm
  virtual ProcessCode finalize() = 0;

  /// Internal method to execute the algorithm for one event.
  /// @note Usually, you should not override this method
  virtual ProcessCode internalExecute(const AlgorithmContext& context) = 0;

  const std::vector<const DataHandleBase*>& writeHandles() const;
  const std::vector<const DataHandleBase*>& readHandles() const;

 private:
  void registerWriteHandle(const DataHandleBase& handle);
  void registerReadHandle(const DataHandleBase& handle);

  template <typename T>
  friend class WriteDataHandle;

  template <typename T>
  friend class ReadDataHandle;

  std::vector<const DataHandleBase*> m_writeHandles;
  std::vector<const DataHandleBase*> m_readHandles;
};

}  // namespace ActsExamples
