// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Material/MaterialSlab.hpp"

namespace Acts {

class Material;

/// Accumulate and average volume-based material properties.
///
/// This class is intended to be used during the mapping process.
class AccumulatedVolumeMaterial {
 public:
  /// Add one entry with the given material properties.
  void accumulate(const MaterialSlab& mat);

  /// Compute the average material collected so far.
  ///
  /// @returns Vacuum properties if no matter has been accumulated yet.
  const Material& average() { return m_average.material(); }

 private:
  MaterialSlab m_average;
};

}  // namespace Acts
