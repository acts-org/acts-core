// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/TrackFinding/TrackParamsLookupTable.hpp"

namespace ActsExamples {

/// @brief Interface for reading track parameter lookup tables
class ITrackParamsLookupReader {
 public:
  /// Virtual Destructor
  virtual ~ITrackParamsLookupReader() = default;

  /// Reader method
  ///
  /// @param path the path to the file to read
  virtual TrackParamsLookup readLookup(const std::string& path) const = 0;
};

}  // namespace ActsExamples
