// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
