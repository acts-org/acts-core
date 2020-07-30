// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// CUDA plugin include(s).
#include "Acts/Plugins/Cuda/Utilities/Arrays.hpp"
#include "Acts/Plugins/Cuda/Utilities/StreamWrapper.hpp"

namespace Acts {
namespace Cuda {

/// Helper type for holding a matrix as a variable array in device memory
template <typename T>
class DeviceMatrix {
 public:
  /// The variable type being used
  using Variable_t = T;
  /// Non-constant reference to an element of the matrix
  using element_reference = Variable_t&;
  /// Constant reference to an element of the matrix
  using element_const_reference = const Variable_t&;
  /// Non-constant pointer to (some part of) the matrix
  using pointer = Variable_t*;
  /// Constant pointer to (some part of) the matrix
  using const_pointer = const Variable_t*;

  /// Create a matrix in device memory
  DeviceMatrix(std::size_t nRows, std::size_t nCols);

  /// Get the number of rows in the matrix
  std::size_t nRows() const { return m_nRows; }
  /// Get the number of columns in the matrix
  std::size_t nCols() const { return m_nCols; }
  /// Get the total size of the matrix
  std::size_t size() const { return m_nRows * m_nCols; }

  /// Get a specific element of the matrix. (non-const)
  element_reference get(std::size_t row = 0, std::size_t col = 0);
  /// Get a specific element of the matrix. (const)
  element_const_reference get(std::size_t row = 0, std::size_t col = 0) const;

  /// Get a "pointer into the matrix" (non-const)
  pointer getPtr(std::size_t row = 0, std::size_t col = 0);
  /// Get a "pointer into the matrix" (const)
  const_pointer getPtr(std::size_t row = 0, std::size_t col = 0) const;

  /// Set a specific element of the matrix
  void set(std::size_t row, std::size_t col, Variable_t val);

  /// Copy memory from the host.
  void copyFrom(const_pointer hostPtr, std::size_t len, std::size_t offset);
  /// Copy memory from the host asynchronously.
  void copyFrom(const_pointer hostPtr, std::size_t len, std::size_t offset,
                const StreamWrapper& streamWrapper);

  /// Reset the matrix to all zeros
  void zeros();

 private:
  /// Rows in the matrix
  std::size_t m_nRows;
  /// Culumns in the matrix
  std::size_t m_nCols;
  /// Smart pointer managing the matrix's memory
  device_array<Variable_t> m_array;

};  // class DeviceMatrix

}  // namespace Cuda
}  // namespace Acts
