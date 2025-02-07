// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/TrackFinding/TrackParamsLookupTable.hpp"

namespace ActsExamples {

/// @brief Interface for writing track parameter lookup tables
class ITrackParamsLookupWriter {
 public:
  /// Virtual Destructor
  virtual ~ITrackParamsLookupWriter() = default;

  /// Writer method
  ///
  /// @param lookup track lookup to write
  virtual void writeLookup(const TrackParamsLookup& lookup) const = 0;
};

}  // namespace ActsExamples
